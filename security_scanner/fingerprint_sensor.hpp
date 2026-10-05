#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <thread>

enum class AckCode : uint8_t {
    SUCCESS = 0x00,
    FAILURE = 0x01,
    NO_FINGER = 0x02,
    ENROLL_FAILURE = 0x03,
    GENERATE_FAILURE_DISORDERLY = 0x06,
    GENERATE_FAILURE_LOW_QUALITY = 0x07,
    FINGER_NO_MATCH = 0x08,
    FIND_FAILURE = 0x09,
    COMBINE_FAILURE = 0x0A,
    PAGE_ID_OUT_OF_RANGE = 0x0B,
    TEMPLATE_READ_ERROR = 0x0C,
    TEMPLATE_UPLOAD_ERROR = 0x0D,
    RECEIVE_ERROR = 0x0E,
    UPLOAD_ERROR = 0x0F,
    TEMPLATE_DELETE_ERROR = 0x10,
    CLEAR_FAILURE = 0x11,
    IMAGE_GENERATE_FAILURE = 0x15,
    WRITE_FAILURE = 0x18,
    NO_DEF_ERROR = 0x19,
    INVALID_REGISTER = 0x1A,
    INCORRECT_CONFIGURATION = 0x1B,
    WRONG_PAGE_ERROR = 0x1C,
    COMMS_FAILED = 0x1D,
};

struct fingerprint_data {
    std::vector<uint8_t> data;
    // Additional metadata can be added here (e.g., timestamp, user ID, etc.)
};

struct serial_package {
    uint8_t header[2]; // Start of packet header
    uint8_t adder[4]; // Command identifier
    uint16_t p_id; // Length of the data payload
    uint8_t dlc; // Data length code
    uint8_t code; // Instruction/Acknowledge code for the package
    std::vector<uint8_t> contents; // Data payload
    uint16_t checksum; // Checksum for error detection
};

class FingerprintSensor {
public:
    bool initialize();
    std::vector<uint8_t> captureFingerprint();
    bool enrollFingerprint(const std::vector<uint8_t>& fingerprintData);
    bool verifyFingerprint(const std::vector<uint8_t>& fingerprintData);
    void shutdown();
    bool writeToStorage(const std::vector<fingerprint_data>& data);
    bool readFromStorage(std::vector<fingerprint_data>* data);

    // Low level driver functions
    bool handleAcknowledgment(serial_package* response);
    bool sendHandshake();
    bool setAddress(uint32_t address);
    bool setSysParam(uint8_t param, uint8_t value);
    bool readTemplateNum(uint16_t* templateCount);
    bool genImg();
    bool upImg();
    bool downImg(const std::vector<uint8_t>& imageData);
    bool img2Tz(uint8_t slot);
    bool regModel();
    bool upChar(uint8_t slot, std::vector<uint8_t>* charData);
    bool downChar(uint8_t slot, const std::vector<uint8_t>& charData);
    bool store(uint16_t pageId, uint8_t slot);
    bool loadChar(uint16_t pageId, uint8_t slot);
    bool deleteChar(uint16_t pageId);
    bool empty();
    bool match(uint8_t* score);
    bool search(uint16_t* pageId, uint8_t* score);
    bool getRandomCode(uint8_t* randomCode);
    bool writeNotepad(uint8_t pageId, const std::vector<uint8_t>& data);
    bool readNotepad(uint8_t pageId, std::vector<uint8_t>* data);

private:
    std::vector<fingerprint_data> enrolledFingerprints; // Store enrolled fingerprints
    int accessAttempts; // Track the number of access attempts
    int baudRate; // Store the baud rate for communication with the sensor
    int chip_addr; // Store the chip address for communication with the sensor
    uint8_t checksum_len{2}; // Length of the checksum for serial communication (We add this to package dlc)
    int port_; // File descriptor for the serial port

    // Serial Handlers
    int openSerialPort(const char* device, int baud_rate);
    int pollSerialPort(int fd, uint8_t* buffer, size_t bufferSize);
    int writeSerialMessage(int fd, const uint8_t* message, size_t messageSize);
    int readSerialResponse(uint8_t* buffer, size_t bufferSize);
    void shutdownSerialPort(int fd);
    bool encodePacket(const serial_package& package, std::vector<uint8_t>* encodedPackage);
};