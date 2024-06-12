#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <vector> // Include for std::vector
#include <sstream>

#define UART_SERVICE_UUID "25AE1441-05D3-4C5B-8281-93D4E07420CF"
#define UART_INPUT_CHARACTERISTIC_UUID "00002a22-0000-1000-8000-00805f9b34fb"
#define PID_CONTROL_CHARACTERISTIC_UUID "00002a28-0000-1000-8000-00805f9b34fb"

BLECharacteristic *uartInputCharacteristic;
BLEServer *bleServer;
bool deviceConnected = false;
std::vector<int> sensorValues(16, 0); // Vector with 16 integers initialized to 0

class BLEConnectionHandler : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* server) override {
        deviceConnected = false;
    }
};

class UARTDataHandler : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        std::string value = characteristic->getValue();
        if (!value.empty()) {
           std::stringstream msg;
            msg << "Received data: ";
            for (size_t i = 0; i < value.length(); i++) {
                msg << std::hex << (int)value[i] << " ";
            }
            Serial.println(msg.str().c_str());
        }
    }
};

void generateSensorValues() {
    for (int& value : sensorValues) {
        value = random(100, 1000);
    }
}

std::string createSensorDataString() {
    std::stringstream dataString;
    for (size_t i = 0; i < sensorValues.size(); i++) {
        // Encode each sensor value as two bytes
        int value = sensorValues[i];
        dataString << (char)(value & 0xFF);
        dataString << (char)((value >> 8) & 0xFF);
    }
    return dataString.str();
}

void setup() {
    Serial.begin(115200);
    BLEDevice::init("LF-Speedy");

    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new BLEConnectionHandler());

    BLEService* uartService = bleServer->createService(UART_SERVICE_UUID);

    uartInputCharacteristic = uartService->createCharacteristic(
        UART_INPUT_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    uartInputCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic* pidControlCharacteristic = uartService->createCharacteristic(
        PID_CONTROL_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE
    );
    pidControlCharacteristic->setCallbacks(new UARTDataHandler());

    uartService->start();
    bleServer->getAdvertising()->start();
    Serial.println("Waiting for client connection to notify...");

    generateSensorValues();
}

void loop() {
    if (deviceConnected) {
        generateSensorValues();
        std::string sensorData = createSensorDataString();
        uartInputCharacteristic->setValue(sensorData);
        uartInputCharacteristic->notify();
    } else {
        delay(500);
        bleServer->getAdvertising()->start();
    }
    delay(100);
}