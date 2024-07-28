#include <iostream>
#include <string>
#include <wiringPi.h>
#include <mqtt/client.h>
#include <nlohmann/json.hpp>

// Define constants
const std::string ADDRESS = "tcp://192.168.57.102:1883";
const std::string CLIENTID = "rpi_led_subscriber";
const std::string TOPIC = "sensor/data";
const int QOS = 1;
const int LED_PIN = 0; // GPIO17

// Create a class that implements the callback interface
class MessageCallback : public virtual mqtt::callback {
    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "Message arrived" << std::endl;
        std::cout << "     topic: " << msg->get_topic() << std::endl;
        std::cout << "   message: " << msg->to_string() << std::endl;

        try {
            // Parse JSON message
            auto json_msg = nlohmann::json::parse(msg->to_string());
            if (json_msg.contains("temp")) {
                float temp_value = json_msg["temp"];
                std::cout << "   temperature: " << temp_value << std::endl;
                if (temp_value > 50.0) { // Threshold value
                    std::cout << "Temperature greater than 50.0, turning LED ON" << std::endl;
                    digitalWrite(LED_PIN, HIGH);
                } else {
                    std::cout << "Temperature less than or equal to 50.0, turning LED OFF" << std::endl;
                    digitalWrite(LED_PIN, LOW);
                }
            } else {
                std::cerr << "No temperature field in message" << std::endl;
            }
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    }

    void connected(const std::string&) override {
        std::cout << "Connected to the broker" << std::endl;
    }

    void connection_lost(const std::string&) override {
        std::cerr << "Connection lost" << std::endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr) override {
        // Optional: Implement if you need to handle delivery confirmations
    }
};

int main() {
    wiringPiSetup();
    pinMode(LED_PIN, OUTPUT);

    mqtt::client client(ADDRESS, CLIENTID);
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    // Set credentials if the broker requires authentication
    connOpts.set_user_name("rahul");
    connOpts.set_password("rahul");

    MessageCallback callback;
    client.set_callback(callback);

    try {
        std::cout << "Connecting to the MQTT server..." << std::endl;
        client.connect(connOpts);
        std::cout << "Connected" << std::endl;

        client.subscribe(TOPIC, QOS);

        while (true) {
            // Keep the client alive
        }
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT error: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
