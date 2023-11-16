#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>
#include <VL53L0X.h>

VL53L0X sensor;



// #define SERVICE_UUID        "439a61ce-9270-48dd-8ab5-9ef92c3e5969"
// #define CHARACTERISTIC_UUID "ab90b28e-c31e-4bbe-b0a8-5673ab1c898d"
#define SERVICE_UUID        "30585118-5628-47bf-b50b-3b66189e194d"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_PIN             5

bool isConnected = false;
unsigned long lastGyroYTime = 0;
int haloCount = 0;
int countV = 0;
int countL = 0;
unsigned long lastCountV1Time = 0;


unsigned long startMillis, currentMillis = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        isConnected = true;
        digitalWrite(LED_PIN, HIGH);
    }

    void onDisconnect(BLEServer* pServer) {
        isConnected = false;
        digitalWrite(LED_PIN, LOW);
    }
};

Adafruit_MPU6050 mpu;
BLECharacteristic *pCharacteristic;

void setup() {
   pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);  // turn the LED on (HIGH is the voltage level)
  Serial.begin(115200);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found. Make sure the connections are correct.");
    while (1);
  }

  Serial.println("MPU6050 found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init("BleEsp322");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks);

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY
  );

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();


  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();
}

void loop() {
  startMillis = millis();
  // Membaca data akselerometer dan gyroscope
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Membuat objek JSON untuk data sensor
  DynamicJsonDocument jsonDocument(256);
  jsonDocument["accelY"] = String(a.acceleration.y);
  jsonDocument["accelX"] = String(a.acceleration.x);
  jsonDocument["accelZ"] = String(a.acceleration.z);
  jsonDocument["gyroX"] = String(g.gyro.x);
  jsonDocument["gyroY"] = String(g.gyro.y);
  jsonDocument["gyroZ"] = String(g.gyro.z);


  // float gyroX = g.gyro.x;
  // float gyroY = g.gyro.y;
  // float gyroZ = g.gyro.z;
  if(sensor.readRangeContinuousMillimeters() > 430){
    if (g.gyro.y > 1.5) {
      // Cek apakah sudah lebih dari 0.5 detik sejak cetak terakhir
      if (millis() - lastGyroYTime >= 200) {
        haloCount++; // Menambah jumlah "Halo"
        countV = 1;
        lastGyroYTime = millis();
        lastCountV1Time = millis(); // Setel waktu terakhir countV menjadi 1
      }else {
        // Jika waktu sejak countV terakhir menjadi 1 lebih dari 0.2 detik, set countV menjadi 0
        if (millis() - lastCountV1Time >= 200) {
          countV = 0;
        }
        countV = 0;
      }
    }else {
      // Jika nilai gyro y tidak memenuhi syarat, set countV menjadi 0
      countV = 0;
    }
  }else{
    if (g.gyro.y > 1.5) {
      // Cek apakah sudah lebih dari 0.5 detik sejak cetak terakhir
      if (millis() - lastGyroYTime >= 200) {
        haloCount++; // Menambah jumlah "Halo"
        countV = 2;
        lastGyroYTime = millis();
        lastCountV1Time = millis(); // Setel waktu terakhir countV menjadi 1
      }else {
        // Jika waktu sejak countV terakhir menjadi 1 lebih dari 0.2 detik, set countV menjadi 0
        if (millis() - lastCountV1Time >= 200) {
          countV = 0;
        }
        countV = 0;
      }
    }else {
      // Jika nilai gyro y tidak memenuhi syarat, set countV menjadi 0
      countV = 0;
    }
  }
  
  jsonDocument["Snare1"] = String(countV);
  jsonDocument["Bass"] = String(countL);

  
  Serial.println(countV);
  
  // Serialize JSON ke dalam string
  String jsonStr;
  char jsonStr2[1000];
  serializeJson(jsonDocument, jsonStr2);

  // Konversi String ke byte array
  uint8_t data[jsonStr.length()];
  jsonStr.getBytes(data, jsonStr.length());

  // Setel nilai karakteristik BLE
  // pCharacteristic->setValue(data, jsonStr.length());
  pCharacteristic->setValue(jsonStr2);
  pCharacteristic->notify();
  // Serial.println(jsonStr2);
  currentMillis = millis();
  // Serial.print("timer = ");
  // Serial.println(currentMillis - startMillis);
  delay(15);  // Sesuaikan delay untuk mengontrol frekuensi pembaruan
}

