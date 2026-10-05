#include "fingerprint_sensor.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <memory.h>

#define POLLING_INTERVAL_MS 10

int openSerialPort(const char* device, int baud_rate);
int pollSerialPort(int fd, uint8_t* buffer, size_t bufferSize);
int writeSerialMessage(int fd, const uint8_t* message, size_t messageSize);
void shutdownSerialPort(int fd);

int main(int argc, char* argv[]) {
    FingerprintSensor sensor;
    if (!sensor.initialize()) {
        return -1;
    }

    int fd = openSerialPort("/dev/serial0", 57600);
    if (fd == -1) {
        return -1;
    }

    std::cout << "Place your finger on the sensor..." << std::endl;

    while (true) {
        int bytes = 0;
        uint8_t buffer[256];
        bytes = pollSerialPort(fd, buffer, sizeof(buffer));
        if (bytes > 0) {
            std::vector<uint8_t> fingerprintData(buffer, buffer + bytes);
            if (sensor.verifyFingerprint(fingerprintData)) {
                std::cout << "Access granted." << std::endl;
            } else {
                std::cout << "Access denied." << std::endl;
            }
        }
        // Clear the buffer for the next read
        memset(buffer, 0, sizeof(buffer));
        // Polling delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS)); // Polling delay
    }

    shutdownSerialPort(fd);
    return 0;
}