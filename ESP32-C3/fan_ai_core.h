#ifndef FAN_AI_CORE_H
#define FAN_AI_CORE_H


// ============================================================
// 내부 설정
// ============================================================

const unsigned long sample_interval_us =
    1000000UL / sampling_frequency;


// ============================================================
// 센서 데이터
// ============================================================

struct sensor_data_t {
    float voltage;
    float current;

    double x[samples];
    double y[samples];
    double z[samples];
};


// ============================================================
// AI Feature
// ============================================================

struct feature_data_t {
    float x_freq;
    float x_energy;

    float y_freq;
    float y_energy;

    float z_freq;
    float z_energy;
};


sensor_data_t sensor_data;
feature_data_t feature_data;


// ============================================================
// Sensor
// ============================================================

class sensor_t {
private:
    Adafruit_INA219 ina219;
    LIS3DH lis3dh = LIS3DH(SPI_MODE, lis3dh_cs);

    void read_power() {
        const int read_count = 10;

        float voltage_sum = 0.0;
        float current_sum = 0.0;

        for (int i = 0; i < read_count; i++) {
            voltage_sum += ina219.getBusVoltage_V();
            current_sum += ina219.getCurrent_mA();
            delay(5);
        }

        sensor_data.voltage = voltage_sum / read_count;
        sensor_data.current = current_sum / read_count;
    }

    void read_acceleration() {
        for (int i = 0; i < samples; i++) {
            unsigned long start_time = micros();

            sensor_data.x[i] = lis3dh.readFloatAccelX();
            sensor_data.y[i] = lis3dh.readFloatAccelY();
            sensor_data.z[i] = lis3dh.readFloatAccelZ();

            while (micros() - start_time < sample_interval_us) {
            }
        }
    }

public:
    void begin() {
        Wire.begin(i2c_sda, i2c_scl);
        Wire.setClock(400000);

        if (!ina219.begin()) {
            Serial.println("INA219 init failed");

            while (1) {
                delay(1000);
            }
        }

        SPI.begin(
            lis3dh_sck,
            lis3dh_miso,
            lis3dh_mosi,
            lis3dh_cs
        );

        lis3dh.settings.accelSampleRate = 400;
        lis3dh.settings.accelRange = 2;

        lis3dh.settings.xAccelEnabled = 1;
        lis3dh.settings.yAccelEnabled = 1;
        lis3dh.settings.zAccelEnabled = 1;

        if (lis3dh.begin() != 0) {
            Serial.println("LIS3DH init failed");

            while (1) {
                delay(1000);
            }
        }
    }

    void read_data() {
        read_power();
        read_acceleration();
    }
};


// ============================================================
// FFT
// ============================================================

class fft_t {
private:
    double v_real[samples];
    double v_imag[samples];

    ArduinoFFT<double> fft = ArduinoFFT<double>(
        v_real,
        v_imag,
        samples,
        sampling_frequency
    );

    void make_feature(
        double axis_data[],
        float &dominant_freq,
        float &dominant_energy
    ) {
        for (int i = 0; i < samples; i++) {
            v_real[i] = axis_data[i];
            v_imag[i] = 0.0;
        }

        fft.dcRemoval();

        fft.windowing(
            FFTWindow::Hamming,
            FFTDirection::Forward
        );

        fft.compute(FFTDirection::Forward);
        fft.complexToMagnitude();

        int max_index = 1;
        double max_magnitude = v_real[1];

        for (int i = 2; i < samples / 2; i++) {
            if (v_real[i] > max_magnitude) {
                max_magnitude = v_real[i];
                max_index = i;
            }
        }

        dominant_freq =
            max_index * sampling_frequency / samples;

        double amplitude =
            2.0 * max_magnitude / samples;

        dominant_energy =
            amplitude * amplitude;
    }

public:
    void make_features() {
        make_feature(
            sensor_data.x,
            feature_data.x_freq,
            feature_data.x_energy
        );

        make_feature(
            sensor_data.y,
            feature_data.y_freq,
            feature_data.y_energy
        );

        make_feature(
            sensor_data.z,
            feature_data.z_freq,
            feature_data.z_energy
        );
    }
};


// ============================================================
// Decision Tree
// ============================================================

class dt_t {
private:
    const char *prediction = "";

public:
    void predict_state() {
        prediction = predictFanState(
            sensor_data.voltage,
            sensor_data.current,

            feature_data.x_freq,
            feature_data.x_energy,

            feature_data.y_freq,
            feature_data.y_energy,

            feature_data.z_freq,
            feature_data.z_energy
        );
    }

    const char *get_prediction() {
        return prediction;
    }
};


// ============================================================
// Fan State
//
// 동일한 AI Prediction이 confirm_count회 연속되면
// Now Fan State를 변경
// ============================================================

class fan_state_t {
private:
    String last_prediction = "";
    String current_state = "UNKNOWN";

    int prediction_count = 0;

public:
    void update(const char *prediction_value) {
        String prediction = String(prediction_value);

        if (prediction == last_prediction) {
            if (prediction_count < confirm_count) {
                prediction_count++;
            }
        }
        else {
            last_prediction = prediction;
            prediction_count = 1;
        }

        if (prediction_count >= confirm_count) {
            current_state = prediction;
        }

        Serial.print("AI Prediction : ");
        Serial.print(prediction);
        Serial.print(" (");
        Serial.print(prediction_count);
        Serial.print("/");
        Serial.print(confirm_count);
        Serial.println(")");

        Serial.print("Now Fan State : ");
        Serial.println(current_state);
    }

    String get_state() {
        return current_state;
    }
};


// ============================================================
// Feature 출력
// ============================================================

void print_features() {
    Serial.println("========================================");

    Serial.printf("Voltage : %.3f V\n", sensor_data.voltage);
    Serial.printf("Current : %.2f mA\n", sensor_data.current);

    Serial.println();

    Serial.printf(
        "X : %7.2f Hz   Energy = %.6f\n",
        feature_data.x_freq,
        feature_data.x_energy
    );

    Serial.printf(
        "Y : %7.2f Hz   Energy = %.6f\n",
        feature_data.y_freq,
        feature_data.y_energy
    );

    Serial.printf(
        "Z : %7.2f Hz   Energy = %.6f\n",
        feature_data.z_freq,
        feature_data.z_energy
    );

    Serial.println();
}


// ============================================================
// 객체 생성
// ============================================================

sensor_t sensor;
fft_t fft;
dt_t dt;
fan_state_t fan_state;


#endif
