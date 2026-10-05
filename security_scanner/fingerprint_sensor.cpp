#include "fingerprint_sensor.hpp"



uint8_t calculateChecksum(const serial_package& package) {
    uint16_t sum = 0;
    for (size_t i = 0; i < sizeof(package.header); ++i) {
        sum += package.header[i];
    }
    for (size_t i = 0; i < sizeof(package.adder); ++i) {
        sum += package.adder[i];
    }
    sum += package.p_id;
    sum += package.dlc;
    for (const auto& byte : package.contents) {
        sum += byte;
    }
    return static_cast<uint8_t>(sum & 0xFF); // Return the least significant byte as the checksum
}

bool FingerprintSensor::initialize() {
    std::cout << "Initializing fingerprint sensor..." << std::endl;
    bool success = readFromStorage(&enrolledFingerprints); // Load enrolled fingerprints from storage
    if(!success) {
        std::cout << "Failed to read enrolled fingerprints from storage." << std::endl;
        return false;
    }
    port_ = openSerialPort("/dev/serial0", 57600); // Open the serial port for communication with the sensor
    if (port_ == -1) {
        std::cout << "Failed to open serial port for fingerprint sensor." << std::endl;
        return false;
    }
    std::cout << "Fingerprint sensor initialized successfully." << std::endl;
    return true; // Return true if initialization is successful
}

std::vector<uint8_t> FingerprintSensor::captureFingerprint() {
    std::vector<uint8_t> fingerprintData(256);
    for (size_t i = 0; i < fingerprintData.size(); ++i) {
        fingerprintData[i] = static_cast<uint8_t>(rand() % 256); // Fill with random data
    }
    return fingerprintData;
}

bool FingerprintSensor::enrollFingerprint(const std::vector<uint8_t>& fingerprintData) {
    // Simulate enrolling a fingerprint
    enrolledFingerprints.push_back({fingerprintData});
    std::cout << "Fingerprint enrolled successfully." << std::endl;
    return true; // Return true if enrollment is successful
}

bool FingerprintSensor::verifyFingerprint(const std::vector<uint8_t>& fingerprintData) {
    // Simulate verifying a fingerprint
    for (const auto& enrolled : enrolledFingerprints) {
        if (enrolled.data == fingerprintData) {
            std::cout << "Fingerprint verified successfully." << std::endl;
            return true; // Return true if verification is successful
        }
    }
    std::cout << "Fingerprint verification failed." << std::endl;
    return false; // Return false if verification fails
}

bool FingerprintSensor::writeToStorage(const std::vector<fingerprint_data>& data) {
    std::cout << "Writing enrolled fingerprints to storage..." << std::endl;
    
    return true; // Return true if write is successful
}

bool FingerprintSensor::readFromStorage(std::vector<fingerprint_data>* data) {
    std::cout << "Reading enrolled fingerprints from storage..." << std::endl;
    
    return true; // Return true if read is successful
}

void FingerprintSensor::shutdown() {
    std::cout << "Shutting down fingerprint sensor..." << std::endl;
    bool complete = writeToStorage(enrolledFingerprints); // Save enrolled fingerprints to storage before shutdown
    if(!complete) {
        std::cout << "Failed to write enrolled fingerprints to storage, shutdown with failure, bye bye fingerprints" << std::endl;
        return;
    }
    std::cout << "Fingerprint sensor shut down successfully." << std::endl;
}

bool FingerprintSensor::handleAcknowledgment(serial_package* response) {
    // Read the acknowledgment response from the sensor and process.
    return readSerialResponse(); // Return true if acknowledgment is handled successfully
}

bool FingerprintSensor::sendHandshake() {
    uint8_t base_dlc = 0x02; // We also add the checksum to this for some fucking reason
    // Send a handshake command to the sensor and wait for acknowledgment.
    serial_package handshakeCommand;
    handshakeCommand.header[0] = 0xEF; // Start of packet header
    handshakeCommand.header[1] = 0x01; // Start of packet header
    handshakeCommand.adder[0] = 0x01; // Command identifier for handshake
    handshakeCommand.adder[1] = 0x00; // Command identifier for handshake
    handshakeCommand.adder[2] = 0x00; // Command identifier for handshake
    handshakeCommand.adder[3] = 0x00; // Command identifier for handshake
    handshakeCommand.p_id = 0x01; 
    handshakeCommand.dlc = base_dlc + checksum_len; // Data length code (base DLC + checksum length)
    handshakeCommand.code = 0x17; // Instruction code for handshake
    handshakeCommand.contents = {0x00};
    handshakeCommand.checksum = calculateChecksum(handshakeCommand); // Calculate checksum for the command

    // Wait on serial ack and parse

    return true; // Return true if handshake is successful
}

// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

int openSerialPort(const char* device, int baud_rate) {
    /// Open the serial port and return the file descriptor
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("openSerialPort: Unable to open serial port");
        return -1;
    }
    fcntl(fd, F_SETFL, 0); // Set to blocking mode

    struct termios options;
    tcgetattr(fd, &options); // Get the current options for the port
    cfsetispeed(&options, baud_rate); // Set the baud rates
    cfsetospeed(&options, baud_rate);
    options.c_cflag |= (CLOCAL | CREAD); // Enable the receiver and set local mode
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE; // Mask the character size bits
    options.c_cflag |= CS8; // 8 data bits
    options.c_cflag &= ~CRTSCTS; // Disable hardware flow control
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input mode
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
    options.c_oflag &= ~OPOST; // Raw output mode
    options.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    options.c_cc[VMIN] = 0;
    tcsetattr(fd, TCSANOW, &options); // Set the new options for the port immediately
    return fd;
}

int pollSerialPort(int fd, uint8_t* buffer, size_t bufferSize) {
    // Poll serial line for incoming data and return the number of bytes available
    int bytes = read(fd, buffer, bufferSize);
    if (bytes < 0) {
        perror("pollSerialPort: Error reading from serial port");
        return -1;
    }
    return bytes;
}

int writeSerialMessage(int fd, const uint8_t* message, size_t messageSize) {
    // Write a message to the serial port and return the number of bytes written
    int bytes = write(fd, message, messageSize);
    if (bytes < 0) {
        perror("writeSerialMessage: Error writing to serial port");
        return -1;
    }
    return bytes;
}

void shutdownSerialPort(int fd) {
    // Close the serial port
    close(fd);
}

bool encodePacket(const serial_package& package, std::vector<uint8_t>* encodedPackage) {
    // Encode the serial package into a byte stream for transmission
    encodedPackage->clear();
    encodedPackage->insert(encodedPackage->end(), std::begin(package.header), std::end(package.header));
    encodedPackage->insert(encodedPackage->end(), std::begin(package.adder), std::end(package.adder));
    encodedPackage->push_back(static_cast<uint8_t>(package.p_id & 0xFF)); // Add lower byte of p_id
    encodedPackage->push_back(static_cast<uint8_t>((package.p_id >> 8) & 0xFF)); // Add upper byte of p_id
    encodedPackage->push_back(package.dlc);
    encodedPackage->push_back(package.code);
    encodedPackage->insert(encodedPackage->end(), package.contents.begin(), package.contents.end());
    uint16_t checksum = calculateChecksum(package);
    encodedPackage->push_back(static_cast<uint8_t>(checksum & 0xFF)); // Add lower byte of checksum
    encodedPackage->push_back(static_cast<uint8_t>((checksum >> 8) & 0xFF)); // Add upper byte of checksum
    return true; // Return true if encoding is successful
}