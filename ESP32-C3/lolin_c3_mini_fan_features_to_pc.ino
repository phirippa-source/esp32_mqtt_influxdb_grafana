#include <Wire.h>
#include <SPI.h>
#include <Adafruit_INA219.h>
#include <SparkFunLIS3DH.h>
#include <arduinoFFT.h>

// ============================================================
// 기본 설정
// ============================================================

// I2C
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
// 가속도 데이터 저장 배열
// ============================================================

double xData[SAMPLES];
double yData[SAMPLES];
double zData[SAMPLES];


// FFT 계산용 버퍼
double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT(
    vReal,
    vImag,
    SAMPLES,
    SAMPLING_FREQUENCY
);


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
            // 샘플링 시각까지 대기
        }

        nextSampleTime += SAMPLE_INTERVAL_US;

        xData[i] = lis3dh.readFloatAccelX();
        yData[i] = lis3dh.readFloatAccelY();
        zData[i] = lis3dh.readFloatAccelZ();
    }
}


// ============================================================
// 한 축의 FFT 수행
//
// 반환값:
//   dominantFrequency : 지배 주파수
//   dominantEnergy    : 지배 주파수 성분의 에너지
// ============================================================

void extractFFTFeature(
    double source[],
    double &dominantFrequency,
    double &dominantEnergy)
{
    // --------------------------------------------------------
    // 1. FFT 버퍼로 복사
    // --------------------------------------------------------

    for (int i = 0; i < SAMPLES; i++)
    {
        vReal[i] = source[i];
        vImag[i] = 0.0;
    }


    // --------------------------------------------------------
    // 2. DC 성분 제거
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
    // 4. FFT
    // --------------------------------------------------------

    FFT.compute(
        FFTDirection::Forward
    );


    // --------------------------------------------------------
    // 5. Magnitude 계산
    // --------------------------------------------------------

    FFT.complexToMagnitude();


    // --------------------------------------------------------
    // 6. 최대 Magnitude를 가지는 주파수 Bin 검색
    //
    // 0 Hz는 제외
    // 실수 신호이므로 N/2까지만 검색
    // --------------------------------------------------------

    int dominantIndex = 1;
    double maxMagnitude = vReal[1];

    for (int i = 2; i < SAMPLES / 2; i++)
    {
        if (vReal[i] > maxMagnitude)
        {
            maxMagnitude = vReal[i];
            dominantIndex = i;
        }
    }


    // --------------------------------------------------------
    // 7. Dominant Frequency 계산
    //
    // Frequency resolution
    // = Fs / N
    // = 400 / 256
    // = 1.5625 Hz
    // --------------------------------------------------------

    dominantFrequency =
        ((double)dominantIndex * SAMPLING_FREQUENCY)
        / SAMPLES;


    // --------------------------------------------------------
    // 8. FFT Magnitude 정규화
    //
    // 단측 Spectrum 기준
    // --------------------------------------------------------

    double dominantAmplitude =
        (2.0 * maxMagnitude) / SAMPLES;


    // --------------------------------------------------------
    // 9. Dominant Energy
    //
    // 프로젝트에서는
    //
    // Energy = Dominant Amplitude²
    //
    // 로 정의
    //
    // 절대적인 물리 에너지라기보다는
    // AI 입력용 상대적 진동 Feature로 사용
    // --------------------------------------------------------

    dominantEnergy =
        dominantAmplitude * dominantAmplitude;
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
    // LIS3DH SPI
    // --------------------------------------------------------

    SPI.begin(
        SPI_SCK,
        SPI_MISO,
        SPI_MOSI,
        LIS3DH_CS
    );


    // ODR = 400 Hz
    lis3dh.settings.accelSampleRate = 400;

    // ±2 g
    lis3dh.settings.accelRange = 2;

    // X/Y/Z 모두 사용
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


    // --------------------------------------------------------
    // CSV Header
    // --------------------------------------------------------

    Serial.println(
        "Voltage,Current,"
        "X_Freq,X_Energy,"
        "Y_Freq,Y_Energy,"
        "Z_Freq,Z_Energy"
    );
}


// ============================================================
// loop()
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // Feature 저장 변수
    // --------------------------------------------------------

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
    // 256 / 400 Hz = 0.64초
    // ========================================================

    collectAcceleration();


    // ========================================================
    // 3. X축 FFT
    // ========================================================

    extractFFTFeature(
        xData,
        xFreq,
        xEnergy
    );


    // ========================================================
    // 4. Y축 FFT
    // ========================================================

    extractFFTFeature(
        yData,
        yFreq,
        yEnergy
    );


    // ========================================================
    // 5. Z축 FFT
    // ========================================================

    extractFFTFeature(
        zData,
        zFreq,
        zEnergy
    );


    // ========================================================
    // 6. PC로 Feature 전송
    //
    // 한 줄 = 학습 데이터 한 Sample 후보
    // ========================================================

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
