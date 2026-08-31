// MQTT Publish Data
// {
//   "voltage": ...,
//   "current": ...,
//   "x_freq": ...,
//   "y_freq": ...,
//   "z_freq": ...,
//   "state": ...
// }

#ifndef MQTT_CORE_H
#define MQTT_CORE_H

class mqtt_t {
private:
    WiFiClient wifi_client;
    PubSubClient mqtt_client;

    void connect_wifi() {
        Serial.print("Wi-Fi Connecting");

        WiFi.mode(WIFI_STA);
        WiFi.setTxPower(WIFI_POWER_8_5dBm);
        WiFi.begin(wifi_ssid, wifi_password);

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }

        Serial.println();
        Serial.print("Wi-Fi Connected : ");
        Serial.println(WiFi.localIP());
    }


    void connect_mqtt() {
        while (!mqtt_client.connected()) {
            Serial.print("MQTT Connecting...");

            if (mqtt_client.connect(
                mqtt_client_id,
                mqtt_user,
                mqtt_password
            )) {
                Serial.println("Connected");
            }
            else {
                Serial.print("Failed, rc=");
                Serial.println(mqtt_client.state());
                delay(2000);
            }
        }
    }


public:
    mqtt_t() : mqtt_client(wifi_client) {
    }


    void begin() {
        connect_wifi();

        mqtt_client.setServer(mqtt_server, mqtt_port);
        connect_mqtt();
    }


    void publish_data() {
        if (WiFi.status() != WL_CONNECTED) {
            connect_wifi();
        }

        if (!mqtt_client.connected()) {
            connect_mqtt();
        }

        mqtt_client.loop();


        // 아직 Fan State가 확정되지 않았다면 발행하지 않음
        String state = fan_state.get_state();

        if (state == "UNKNOWN") {
            Serial.println("MQTT Publish : Waiting for Fan State");
            return;
        }


        char payload[256];
        // MQTT 메시지 구조
        snprintf(
            payload,
            sizeof(payload),
            "{\"voltage\":%.3f,"
            "\"current\":%.2f,"
            "\"x_freq\":%.2f,"
            "\"y_freq\":%.2f,"
            "\"z_freq\":%.2f,"
            "\"state\":\"%s\"}",

            sensor_data.voltage,
            sensor_data.current,

            feature_data.x_freq,
            feature_data.y_freq,
            feature_data.z_freq,

            state.c_str()
        );


        bool result = mqtt_client.publish(
            mqtt_topic,
            payload
        );


        if (result) {
            Serial.print("MQTT Publish : ");
            Serial.println(payload);
        }
        else {
            Serial.println("MQTT Publish Failed");
        }
    }
};


mqtt_t mqtt;


#endif
