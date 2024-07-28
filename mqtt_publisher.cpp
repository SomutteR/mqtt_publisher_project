#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <unistd.h> // For sleep function
#include <ctime> // For time function
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <MQTTClient.h> // Include the MQTTClient header from the Paho MQTT library

#define ADDRESS    "tcp://192.168.57.102:1883"
#define CLIENTID   "rpi1"
#define AUTHMETHOD "rahul"
#define AUTHTOKEN  "rahul"
#define TOPIC      "sensor/data"
#define TIMEOUT    10000L

#define ADXL345_ADDR 0x53
#define CPU_TEMP "/sys/class/thermal/thermal_zone0/temp"

int fd;

float getCPUTemperature() {
    int cpuTemp;
    std::ifstream fs;
    fs.open(CPU_TEMP);
    fs >> cpuTemp;
    fs.close();
    return cpuTemp / 1000.0;
}

void read_accelerometer_data(int fd, int &x, int &y, int &z) {
    x = wiringPiI2CReadReg8(fd, 0x32) | (wiringPiI2CReadReg8(fd, 0x33) << 8);
    y = wiringPiI2CReadReg8(fd, 0x34) | (wiringPiI2CReadReg8(fd, 0x35) << 8);
    z = wiringPiI2CReadReg8(fd, 0x36) | (wiringPiI2CReadReg8(fd, 0x37) << 8);
}

void publish_sensor_data(MQTTClient client, int qos) {
    char payload[200];
    int x, y, z;
    float temp;
    char time_str[100];
    time_t rawtime;
    struct tm * timeinfo;

    while (true) {
        read_accelerometer_data(fd, x, y, z);
        temp = getCPUTemperature();
        
        // Get current time
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

        snprintf(payload, sizeof(payload), "{\"time\": \"%s\", \"x\": %d, \"y\": %d, \"z\": %d, \"temp\": %.2f}", time_str, x, y, z, temp);
        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        pubmsg.payload = payload;
        pubmsg.payloadlen = strlen(payload);
        pubmsg.qos = qos;
        pubmsg.retained = 0;
        MQTTClient_deliveryToken token;
        MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
        MQTTClient_waitForCompletion(client, token, TIMEOUT);
        std::cout << "Message with token " << token << " delivered.\n";
        sleep(1); // Publish every second
    }
}

int main() {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;

    // Set Last Will and Testament (LWT)
    will_opts.topicName = TOPIC;
    will_opts.message = "{\"status\": \"disconnected\"}";
    will_opts.qos = 1; // Change this to set the QoS level for the LWT message
    will_opts.retained = 0;
    opts.will = &will_opts;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    int rc;
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        std::cerr << "Failed to connect, return code " << rc << std::endl;
        return -1;
    }

    // Initialize wiringPi and I2C
    if (wiringPiSetup() == -1) {
        std::cerr << "wiringPi setup failed" << std::endl;
        return -1;
    }
    fd = wiringPiI2CSetup(ADXL345_ADDR);
    if (fd == -1) {
        std::cerr << "Failed to init I2C communication." << std::endl;
        return -1;
    }
    // Enable measurement mode
    wiringPiI2CWriteReg8(fd, 0x2D, 0x08);

    // Set QoS level here
    int qos = 1; // Change this value to 0, 1, or 2 to test different QoS levels

    // Publish sensor data
    publish_sensor_data(client, qos);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
