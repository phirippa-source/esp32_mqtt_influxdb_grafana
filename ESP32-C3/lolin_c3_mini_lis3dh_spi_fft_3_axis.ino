#include <SPI.h>
#include <SparkFunLIS3DH.h>
#include <arduinoFFT.h>

#define SAMPLES               256
#define SAMPLING_FREQUENCY    400
#define SAMPLE_INTERVAL_US    2500

// ----------------------------------------------------
// LIS3DH
// SPI CS = GPIO5
// ----------------------------------------------------
LIS3DH lis3dh(SPI_MODE, 5);


// ----------------------------------------------------
// 3축 원시 가속도 데이터 저장 배열
// ----------------------------------------------------
double xData[SAMPLES];
double yData[SAMPLES];
double zData[SAMPLES];


// ----------------------------------------------------
// FFT 계산용 배열
//
// X/Y/Z 각각 별도의 FFT 배열을 만들지 않고,
// 한 축씩 복사해서 반복 사용한다.
// ----------------------------------------------------
double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT(
    vReal,
    vImag,
    SAMPLES,
    SAMPLING_FREQUENCY
);


// ----------------------------------------------------
// 한 축의 데이터를 FFT 계산용 배열로 복사
// ----------------------------------------------------
void copyToFFTBuffer(double source[]) {

    for (int i = 0; i < SAMPLES; i++) {
        vReal[i] = source[i];
        vImag[i] = 0.0;
    }
}


// ----------------------------------------------------
// 한 축에 대해 FFT 수행 후 PC로 전송
// ----------------------------------------------------
void processFFT(double source[], const char* axisName) {

    // 원시 데이터를 FFT 버퍼에 복사
    copyToFFTBuffer(source);


    // ------------------------------------------------
    // 1. DC 성분 제거
    // ------------------------------------------------
    FFT.dcRemoval();


    // ------------------------------------------------
    // 2. Hamming Window 적용
    // ------------------------------------------------
    FFT.windowing(
        FFTWindow::Hamming,
        FFTDirection::Forward
    );


    // ------------------------------------------------
    // 3. FFT 수행
    // ------------------------------------------------
    FFT.compute(
        FFTDirection::Forward
    );


    // ------------------------------------------------
    // 4. 복소수 결과를 Magnitude로 변환
    // ------------------------------------------------
    FFT.complexToMagnitude();


    // ------------------------------------------------
    // 5. 해당 축 FFT 데이터 전송 시작
    //
    // 예:
    // FFT_X_BEGIN
    // ------------------------------------------------
    Serial.print("FFT_");
    Serial.print(axisName);
    Serial.println("_BEGIN");


    // ------------------------------------------------
    // 0 Hz(DC)는 제외
    //
    // SAMPLES = 256
    // Fs      = 400 Hz
    //
    // FFT 결과에서 1 ~ 127 bin 전송
    //
    // 주파수 해상도
    // = 400 / 256
    // = 1.5625 Hz
    // ------------------------------------------------
    for (int i = 1; i < SAMPLES / 2; i++) {

        double frequency =
            ((double)i * SAMPLING_FREQUENCY) / SAMPLES;

        Serial.print(frequency, 4);
        Serial.print(",");
        Serial.println(vReal[i], 6);
    }


    // ------------------------------------------------
    // 해당 축 FFT 데이터 전송 종료
    //
    // 예:
    // FFT_X_END
    // ------------------------------------------------
    Serial.print("FFT_");
    Serial.print(axisName);
    Serial.println("_END");
}


// ----------------------------------------------------
// Setup
// ----------------------------------------------------
void setup() {

    Serial.begin(230400);

    // SCK=2, MISO=0, MOSI=4, CS=5
    SPI.begin(2, 0, 4, 5);


    // ------------------------------------------------
    // LIS3DH 설정
    // ------------------------------------------------

    // Output Data Rate = 400 Hz
    lis3dh.settings.accelSampleRate = 400;

    // 측정 범위 ±2 g
    lis3dh.settings.accelRange = 2;

    // X/Y/Z 모두 사용
    lis3dh.settings.xAccelEnabled = 1;
    lis3dh.settings.yAccelEnabled = 1;
    lis3dh.settings.zAccelEnabled = 1;


    if (lis3dh.begin() != 0) {

        Serial.println("LIS3DH init failed");

        while (1) {
            delay(1000);
        }
    }


    delay(1000);

    Serial.println("3-Axis FFT Ready");
}


// ----------------------------------------------------
// Main Loop
// ----------------------------------------------------
void loop() {

    // ==================================================
    // 1. X/Y/Z 가속도 256개 수집
    //
    // Fs = 400 Hz
    // 256 samples / 400 Hz = 0.64초
    // ==================================================

    unsigned long nextSampleTime = micros();

    for (int i = 0; i < SAMPLES; i++) {

        // 다음 샘플링 시각까지 대기
        while ((long)(micros() - nextSampleTime) < 0) {
        }

        nextSampleTime += SAMPLE_INTERVAL_US;


        // ------------------------------------------------
        // 같은 sampling 시점에서 X/Y/Z 측정
        // ------------------------------------------------
        xData[i] = lis3dh.readFloatAccelX();
        yData[i] = lis3dh.readFloatAccelY();
        zData[i] = lis3dh.readFloatAccelZ();
    }


    // ==================================================
    // 2. 한 Frame 시작
    // ==================================================

    Serial.println("FRAME_BEGIN");


    // ==================================================
    // 3. X축 FFT
    // ==================================================

    processFFT(xData, "X");


    // ==================================================
    // 4. Y축 FFT
    // ==================================================

    processFFT(yData, "Y");


    // ==================================================
    // 5. Z축 FFT
    // ==================================================

    processFFT(zData, "Z");


    // ==================================================
    // 6. 한 Frame 종료
    // ==================================================

    Serial.println("FRAME_END");


    // 그래프 갱신 속도를 조금 여유 있게
    delay(300);
}
