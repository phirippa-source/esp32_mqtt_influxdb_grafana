#include <Wire.h>
#include <SPI.h>
#include <Adafruit_INA219.h>
#include <SparkFunLIS3DH.h>
#include <arduinoFFT.h>

// ============================================================
// 기본 설정
// ============================================================

// I2C - INA219
#define SDA_PIN               8
#define SCL_PIN               10

// SPI - LIS3DH
#define LIS3DH_CS             5
#define SPI_SCK               2
#define SPI_MISO              0
#define SPI_MOSI              4

// FFT
#define SAMPLES               256
#define SAMPLING_FREQUENCY    400
#define SAMPLE_INTERVAL_US    2500


// ============================================================
// 센서 객체
// ============================================================

Adafruit_INA219 ina219;
LIS3DH lis3dh(SPI_MODE, LIS3DH_CS);


// ============================================================
// 원시 가속도 데이터
// ============================================================

double xData[SAMPLES];
double yData[SAMPLES];
double zData[SAMPLES];


// FFT 계산용 공용 버퍼
double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT(
    vReal,
    vImag,
    SAMPLES,
    SAMPLING_FREQUENCY
);


// ============================================================
// FFT 결과 저장용
// PC에 전체 Spectrum을 보내기 위해 별도로 저장
// ============================================================

double xSpectrum[SAMPLES / 2];
double ySpectrum[SAMPLES / 2];
double zSpectrum[SAMPLES / 2];


// ============================================================
// 전압 / 전류 평균 측정
// ============================================================

void measurePower(float &voltage, float &current)
{
    const int NUM_READINGS = 10;

    float voltageSum = 0.0;
    float currentSum = 0.0;

    for (int i = 0; i < NUM_READINGS; i++)
    {
        voltageSum += ina219.getBusVoltage_V();
        currentSum += ina219.getCurrent_mA();

        delay(3);
    }

    voltage = voltageSum / NUM_READINGS;
    current = currentSum / NUM_READINGS;
}


// ============================================================
// X/Y/Z 가속도 동시 수집
// ============================================================

void collectAcceleration()
{
    unsigned long nextSampleTime = micros();

    for (int i = 0; i < SAMPLES; i++)
    {
        while ((long)(micros() - nextSampleTime) < 0)
        {
        }

        nextSampleTime += SAMPLE_INTERVAL_US;

        xData[i] = lis3dh.readFloatAccelX();
        yData[i] = lis3dh.readFloatAccelY();
        zData[i] = lis3dh.readFloatAccelZ();
    }
}


// ============================================================
// 한 축 FFT 수행
//
// source[]      : 원시 가속도 데이터
// spectrum[]    : FFT Magnitude 저장
// dominantFreq  : 최대 성분의 주파수
// dominantEnergy: 최대 성분의 에너지
// ============================================================

void processFFT(
    double source[],
    double spectrum[],
    double &dominantFreq,
    double &dominantEnergy
)
{
    // --------------------------------------------------------
    // 1. 원시 데이터를 FFT 버퍼로 복사
    // --------------------------------------------------------

    for (int i = 0; i < SAMPLES; i++)
    {
        vReal[i] = source[i];
        vImag[i] = 0.0;
    }


    // --------------------------------------------------------
    // 2. DC 제거
    // --------------------------------------------------------

    FFT.dcRemoval();


    // --------------------------------------------------------
    // 3. Hamming Window
    // --------------------------------------------------------

    FFT.windowing(
        FFTWindow::Hamming,
        FFTDirection::Forward
    );


    // --------------------------------------------------------
    // 4. FFT 수행
    // --------------------------------------------------------

    FFT.compute(
        FFTDirection::Forward
    );


    // --------------------------------------------------------
    // 5. Magnitude 계산
    // --------------------------------------------------------

    FFT.complexToMagnitude();


    // --------------------------------------------------------
    // 6. Spectrum 저장
    // --------------------------------------------------------

    for (int i = 0; i < SAMPLES / 2; i++)
    {
        spectrum[i] = vReal[i];
    }


    // --------------------------------------------------------
    // 7. Dominant Frequency 탐색
    // 0 Hz는 제외
    // --------------------------------------------------------

    int dominantIndex = 1;
    double maxMagnitude = spectrum[1];

    for (int i = 2; i < SAMPLES / 2; i++)
    {
        if (spectrum[i] > maxMagnitude)
        {
            maxMagnitude = spectrum[i];
            dominantIndex = i;
        }
    }


    dominantFreq =
        ((double)dominantIndex * SAMPLING_FREQUENCY)
        / SAMPLES;


    // --------------------------------------------------------
    // 8. Dominant Energy 계산
    //
    // 단측 spectrum 정규화
    // --------------------------------------------------------

    double dominantAmplitude =
        (2.0 * maxMagnitude) / SAMPLES;

    dominantEnergy =
        dominantAmplitude * dominantAmplitude;
}


// ============================================================
// Feature 한 줄 전송
// ============================================================

void sendFeatures(
    float voltage,
    float current,
    double xFreq,
    double xEnergy,
    double yFreq,
    double yEnergy,
    double zFreq,
    double zEnergy
)
{
    Serial.print("FEATURE,");

    Serial.print(voltage, 3);
    Serial.print(",");

    Serial.print(current, 2);
    Serial.print(",");

    Serial.print(xFreq, 4);
    Serial.print(",");

    Serial.print(xEnergy, 8);
    Serial.print(",");

    Serial.print(yFreq, 4);
    Serial.print(",");

    Serial.print(yEnergy, 8);
    Serial.print(",");

    Serial.print(zFreq, 4);
    Serial.print(",");

    Serial.println(zEnergy, 8);
}


// ============================================================
// 한 축의 전체 Spectrum 전송
// ============================================================

void sendSpectrum(
    const char* axisName,
    double spectrum[]
)
{
    Serial.print("FFT_");
    Serial.print(axisName);
    Serial.println("_BEGIN");


    // 0 Hz는 제외
    for (int i = 1; i < SAMPLES / 2; i++)
    {
        double frequency =
            ((double)i * SAMPLING_FREQUENCY)
            / SAMPLES;

        Serial.print(frequency, 4);
        Serial.print(",");
        Serial.println(spectrum[i], 6);
    }


    Serial.print("FFT_");
    Serial.print(axisName);
    Serial.println("_END");
}


// ============================================================
// setup()
// ============================================================

void setup()
{
    Serial.begin(230400);

    delay(1000);


    // --------------------------------------------------------
    // INA219
    // --------------------------------------------------------

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    if (!ina219.begin())
    {
        Serial.println("INA219 init failed");

        while (1)
        {
            delay(1000);
        }
    }


    // --------------------------------------------------------
    // LIS3DH
    // --------------------------------------------------------

    SPI.begin(
        SPI_SCK,
        SPI_MISO,
        SPI_MOSI,
        LIS3DH_CS
    );


    lis3dh.settings.accelSampleRate = 400;
    lis3dh.settings.accelRange = 2;

    lis3dh.settings.xAccelEnabled = 1;
    lis3dh.settings.yAccelEnabled = 1;
    lis3dh.settings.zAccelEnabled = 1;


    if (lis3dh.begin() != 0)
    {
        Serial.println("LIS3DH init failed");

        while (1)
        {
            delay(1000);
        }
    }


    delay(1000);

    Serial.println("READY");
}


// ============================================================
// loop()
// ============================================================

void loop()
{
    float voltage;
    float current;

    double xFreq;
    double xEnergy;

    double yFreq;
    double yEnergy;

    double zFreq;
    double zEnergy;


    // ========================================================
    // 1. 전압 / 전류 측정
    // ========================================================

    measurePower(
        voltage,
        current
    );


    // ========================================================
    // 2. X/Y/Z 가속도 256개 수집
    //
    // 256 / 400 = 0.64초
    // ========================================================

    collectAcceleration();


    // ========================================================
    // 3. X/Y/Z FFT
    // ========================================================

    processFFT(
        xData,
        xSpectrum,
        xFreq,
        xEnergy
    );

    processFFT(
        yData,
        ySpectrum,
        yFreq,
        yEnergy
    );

    processFFT(
        zData,
        zSpectrum,
        zFreq,
        zEnergy
    );


    // ========================================================
    // 4. 한 Frame 시작
    // ========================================================

    Serial.println("FRAME_BEGIN");


    // ========================================================
    // 5. Feature 전송
    // ========================================================

    sendFeatures(
        voltage,
        current,
        xFreq,
        xEnergy,
        yFreq,
        yEnergy,
        zFreq,
        zEnergy
    );


    // ========================================================
    // 6. X/Y/Z 전체 Spectrum 전송
    // ========================================================

    sendSpectrum(
        "X",
        xSpectrum
    );

    sendSpectrum(
        "Y",
        ySpectrum
    );

    sendSpectrum(
        "Z",
        zSpectrum
    );


    // ========================================================
    // 7. 한 Frame 종료
    // ========================================================

    Serial.println("FRAME_END");


    // 약간의 여유
    delay(200);
}
