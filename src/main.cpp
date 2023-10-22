#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "25AE1441-05D3-4C5B-8281-93D4E07420CF" // UART service UUID
#define CHARACTERISTIC_UUID_INPUT "00002a22-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_PID_CONST "00002a28-0000-1000-8000-00805f9b34fb"

BLECharacteristic *pCharacteristic;
BLEServer *pServer;
bool deviceConnected = false;

int sensors[17] = {0};

// this lamnda expression returns the size of an arry of any type <T>;
// it is used to calculate the size of the sensors array
template<typename T, size_t N>
size_t getArraySize(T (&array)[N]) {
    return N;
}

class MyServerCallbacks : public BLEServerCallbacks{
  void onConnect(BLEServer *pServer){
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer){
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string readValue = pCharacteristic->getValue();

    if (readValue.length() > 0){
      Serial.println("*********");
      Serial.print("Received Value: ");
      Serial.println(readValue.c_str());
      Serial.println("*********");
    }
  }
};

/**
 * Esta función genera números aleatorios entre 0 y 1000 para simular los valores
*/
void generateRandomValues() {
  for (int i = 0; i < 17; i++) {
    sensors[i] = random(100, 1000);
  }
}

/**
 * Esta función se encarga de enviar la velcididad de los motores al dispositivo
 * así como los valores de los 16 sensores. 
 * @return Una cadena de texto con el formato:
 * rmp;s1;s2;s3;s4;s5;s6;s7;s8;s9;s10;s11;s12;s13;s14;s15;s16
*/

std::string getOutputString() {
  std::string outputString = "";
  for (int i = 0; i < 17; i++) {
    outputString += std::to_string(sensors[i]);
    if (i < 16) {
      outputString += ";";
    }
  }
  return outputString;
}

void setup(){
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32 UART Test"); // Give it a name

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_INPUT,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_PID_CONST,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());
  

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  generateRandomValues();
}

void loop(){
  if (deviceConnected){
    generateRandomValues();
    auto outputString = getOutputString();
    pCharacteristic->setValue(outputString.c_str());

    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent Value: ");
    Serial.print(outputString.c_str());
    Serial.println(" ***");
  } else {
    delay(500);
    pServer->getAdvertising()->start();

  }
  delay(100); // bluetooth stack will go into congestion, if too many packets are sent
}
