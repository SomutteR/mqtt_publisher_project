#include <iostream>
#include <fstream>
#include <string>
#include <mqtt/client.h>
#include <nlohmann/json.hpp>

// Define constants
const std::string ADDRESS = "tcp://192.168.57.102:1883";
const std::string CLIENTID = "rpi_log_subscriber";
const std::string TOPIC = "sensor/data";
const int QOS = 1;
const std::string LOGFILE = "sensor_data.log";

// Create a class that implements the callback interface
class MessageCallback : public virtual mqtt::callback {
    std::ofstream logFile;

public:
    MessageCallback() {
        // Open log file in append mode
        logFile.open(LOGFILE, std::ios_base::app);
        if (!logFile.is_open()) {
            std::cerr << "Unable to open log file" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    ~MessageCallback() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "Message arrived" << std::endl;
        std::cout << "     topic: " << msg->get_topic() << std::endl;
        std::cout << "   message: " << msg->to_string() << std::endl;

        try {
            // Parse JSON message
            auto json_msg = nlohmann::json::parse(msg->to_string());
            logFile << "Message arrived" << std::endl;
            logFile << "     topic: " << msg->get_topic() << std::endl;
            logFile << "   message: " << msg->to_string() << std::endl;

            if (json_msg.contains("temp")) {
                float temp_value = json_msg["temp"];
                logFile << "   temperature: " << temp_value << std::endl;
            }
            logFile << std::endl;
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            logFile << "JSON parse error: " << e.what() << std::endl;
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
