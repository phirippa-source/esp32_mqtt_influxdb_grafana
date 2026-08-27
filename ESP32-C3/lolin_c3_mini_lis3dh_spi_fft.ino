#include <SPI.h>
#include <SparkFunLIS3DH.h>
#include <arduinoFFT.h>

#define SAMPLES               256
#define SAMPLING_FREQUENCY    400
#define SAMPLE_INTERVAL_US    2500

LIS3DH lis3dh(SPI_MODE, 5);

double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT(
    vReal,
    vImag,
    SAMPLES,
    SAMPLING_FREQUENCY
);

void setup() {
    Serial.begin(230400);

    SPI.begin(2, 0, 4, 5);   // SCK=2, MISO=0, MOSI=4, CS=5

    // LIS3DH 설정
    lis3dh.settings.accelSampleRate = 400;   // ODR = 400 Hz
    lis3dh.settings.accelRange = 2;          // ±2 g

    lis3dh.settings.xAccelEnabled = 1;
    lis3dh.settings.yAccelEnabled = 1;
    lis3dh.settings.zAccelEnabled = 1;

    if (lis3dh.begin() != 0) {
        Serial.println("LIS3DH init failed");
        while (1);
    }

    delay(1000);
}

void loop() {

    // ------------------------------------------------
    // 1. X축 가속도 256개를 400 Hz로 수집
    // ------------------------------------------------
    unsigned long nextSampleTime = micros();

    for (int i = 0; i < SAMPLES; i++) {

        while ((long)(micros() - nextSampleTime) < 0) {
            // 다음 샘플링 시각까지 대기
        }

        nextSampleTime += SAMPLE_INTERVAL_US;

        vReal[i] = lis3dh.readFloatAccelX();
        vImag[i] = 0.0;
    }


    // ------------------------------------------------
    // 2. FFT
    // ------------------------------------------------

    // DC 성분 제거
    FFT.dcRemoval();

    // Hamming Window
    FFT.windowing(
        FFTWindow::Hamming,
        FFTDirection::Forward
    );

    // FFT 계산
    FFT.compute(FFTDirection::Forward);

    // Magnitude 계산
    FFT.complexToMagnitude();


    // ------------------------------------------------
    // 3. FFT 결과를 PC로 전송
    // ------------------------------------------------

    Serial.println("FFT_BEGIN");

    // 0 Hz(DC)는 제외
    for (int i = 1; i < SAMPLES / 2; i++) {

        double frequency =
            ((double)i * SAMPLING_FREQUENCY) / SAMPLES;

        Serial.print(frequency, 4);
        Serial.print(",");
        Serial.println(vReal[i], 6);
    }

    Serial.println("FFT_END");

    delay(500);
}
