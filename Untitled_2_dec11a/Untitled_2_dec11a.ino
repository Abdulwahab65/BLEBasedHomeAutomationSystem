#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include "thingProperties.h"
#include "arduino_secrets.h"


BLECharacteristic *pCharacteristicTemp; 
BLECharacteristic *pCharacteristicHumidity; 
BLECharacteristic *pCharacteristicLux;

bool deviceConnected = false;
float txValue = 0;  
float hxValue = 0;  
float luxValue = 0; 
bool manualControl = false;


const int relay = 26;
#define DHTPIN 5
#define DHTTYPE DHT11


BH1750 lightMeter;
DHT dht(DHTPIN, DHTTYPE);

#define SERVICE_UUID           "df9b6ad3-8c86-4c75-8036-9ed2f039a3b0"
#define CHARACTERISTIC_UUID_RX "ebe63d68-1e27-4f2a-9bd3-7f4be4e9119c"
#define CHARACTERISTIC_UUID_TX_TEMP "366945dc-c10e-4bdd-a45e-918dbf9f81e7"
#define CHARACTERISTIC_UUID_TX_HUMIDITY "d5ec7c69-1f45-46c4-9c3b-713ac73ea2a7"
#define CHARACTERISTIC_UUID_TX_LUX "1a0a813e-81e4-4aa9-b682-9cdbea2c9359"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            //manualControl = true;
            if (rxValue == "A") { 
                digitalWrite(relay, HIGH);
                lamp = true; 
            }
            else if (rxValue == "B") {
                digitalWrite(relay, LOW);
                lamp = false; 
            }
        }
    }
};

void setup() {
    Serial.begin(115200);

    Wire.begin();
    lightMeter.begin();
    dht.begin();
    pinMode(relay, OUTPUT);
    digitalWrite(relay, LOW);

    BLEDevice::init("ESP32");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristicTemp = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_TX_TEMP,
                            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                          );
    pCharacteristicTemp->addDescriptor(new BLE2902());

   
    pCharacteristicHumidity = pService->createCharacteristic(
                                CHARACTERISTIC_UUID_TX_HUMIDITY,
                                BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                              );
    pCharacteristicHumidity->addDescriptor(new BLE2902());

    pCharacteristicLux = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_TX_LUX,
                            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                         );
    pCharacteristicLux->addDescriptor(new BLE2902());

    BLECharacteristic *pCharacteristicRX = pService->createCharacteristic(
                                              CHARACTERISTIC_UUID_RX,
                                              BLECharacteristic::PROPERTY_WRITE
                                           );
    pCharacteristicRX->setCallbacks(new MyCallbacks());

    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("Waiting for BLE client connection...");

    initProperties();
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    setDebugMessageLevel(2);
    ArduinoCloud.printDebugInfo();
}

void loop() {
    ArduinoCloud.update();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    luxValue = lightMeter.readLightLevel();

    temperature = t;
    humidity = h;
    intensity = luxValue;

    if (!manualControl) { 
        if (luxValue < 2) {
            digitalWrite(relay, HIGH);
            lamp = true;
        } else {
            digitalWrite(relay, LOW);
            lamp = false;

        }
    } else {
        if (luxValue < 2 && !lamp) { 
            manualControl = false;
        }
    }

    ArduinoCloud.update();

    if (deviceConnected) {
        txValue = t;
        hxValue = h;

        char tempString[8];
        dtostrf(txValue, 1, 2, tempString);
        pCharacteristicTemp->setValue(tempString);
        pCharacteristicTemp->notify();

        char humidityString[8];
        dtostrf(hxValue, 1, 2, humidityString);
        pCharacteristicHumidity->setValue(humidityString);
        pCharacteristicHumidity->notify();

        char luxString[8];
        dtostrf(luxValue, 1, 2, luxString);
        pCharacteristicLux->setValue(luxString);
        pCharacteristicLux->notify();
      
    }

}

void onLampChange() {
    //manualControl = true;
    if (lamp) {
        digitalWrite(relay, HIGH);
    } else {
        digitalWrite(relay, LOW); 
    }
}