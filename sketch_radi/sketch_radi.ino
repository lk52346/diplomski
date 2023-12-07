#include <Wire.h>
#include <MPU6050.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

MPU6050 mpu;

const char* ssid = "internetK";
const char* password = "10203040";

IPAddress localIP(192, 168, 1, 177);  // Prilagodite IP adresu vašoj mreži
unsigned int localPort = 8888;       // Prilagodite port prema vašim potrebama

IPAddress remoteIP; // Prilagodite IP adresu odredišta
unsigned int remotePort = 12345;      // Prilagodite port odredišta prema vašim potrebama

IPAddress broadcastAddress(192, 168, 100, 255);

char packetBuffer[255];

WiFiUDP udp;

// Gyroscope calibration offsets
float gyroXoffset = 0.0;
float gyroYoffset = 0.0;
float gyroZoffset = 0.0;

int lastPitch, lastRoll = 0;

// Filter parameters
float alpha = 0.02;  // Weight for gyroscope data (between 0 and 1)


struct Paket {
  int os;
  int vrijednost;
};



void calibrateGyro() {
  Serial.println("Gyroscope Calibration...");

  const int numCalibrationSamples = 1000;
  float gx, gy, gz;
  float gxSum = 0, gySum = 0, gzSum = 0;

  for (int i = 0; i < numCalibrationSamples; ++i) {
    gxSum += mpu.getRotationX();
    gySum += mpu.getRotationY();
    gzSum += mpu.getRotationZ();
    delay(5);
  }

  gyroXoffset = -gxSum / numCalibrationSamples;
  gyroYoffset = -gySum / numCalibrationSamples;
  gyroZoffset = -gzSum / numCalibrationSamples;

  Serial.println("Gyroscope Calibration Complete");
}

void setup() {
  Serial.begin(9600);

  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  calibrateGyro();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Povezivanje na WiFi...");
  }
  Serial.println("Povezano na WiFi.");

  udp.begin(localPort);

  Paket paket;
  paket.os = 0;
  paket.vrijednost = 1;

  StaticJsonDocument<256> jsonDocument;
  jsonDocument["Os"] = paket.os;
  jsonDocument["Vrijednost"] = paket.vrijednost;

  String jsonString;
  serializeJson(jsonDocument, jsonString);

  udp.beginPacket(broadcastAddress, remotePort);
  udp.print(jsonString);
  udp.endPacket();
  

  while(1){
    int packetSize = udp.parsePacket();
    if (packetSize) {

      Serial.print("Received packet of size ");

      Serial.println(packetSize);


      // read the packet into packetBufffer

      int len = udp.read(packetBuffer, 255);

      if (len > 0) {
        packetBuffer[len] = 0;
      }

      if(strcmp(packetBuffer, "pong")==0){
        remoteIP = udp.remoteIP();

        Serial.print(remoteIP);

        Serial.print(", port ");

        Serial.println(udp.remotePort());

        Serial.println("Contents:");

        Serial.println(packetBuffer);
        break;
      }
    }


    delay(50);
  }

  Serial.println("SPOJENO");
}

void loop() {

  float gyroX = (mpu.getRotationX() + gyroXoffset) / 131.0;
  float gyroY = (mpu.getRotationY() + gyroYoffset) / 131.0;
  float gyroZ = (mpu.getRotationZ() + gyroZoffset) / 131.0;

  float pitchAcc = atan2(mpu.getAccelerationY(), mpu.getAccelerationZ()) * 180.0 / PI;
  float rollAcc = atan2(-mpu.getAccelerationX(), sqrt(mpu.getAccelerationY() * mpu.getAccelerationY() + mpu.getAccelerationZ() * mpu.getAccelerationZ())) * 180.0 / PI;

  float pitch = alpha * (pitch + gyroX * 0.01) + (1 - alpha) * pitchAcc;
  float roll = alpha * (roll + gyroY * 0.01) + (1 - alpha) * rollAcc;
  float yaw = gyroZ;

  Serial.print("Yaw: ");
  Serial.print(yaw);
  Serial.print("\tPitch: ");
  Serial.print(pitch);
  Serial.print("\tRoll: ");
  Serial.println(roll);

  int pitchRound = -pitch / 10;
  int rollRound = -roll / 10;

  if(pitchRound != lastPitch){
    Paket pitchPaket;
    pitchPaket.os = 1;
    pitchPaket.vrijednost = pitchRound;
    lastPitch = pitchRound;
    sendUDP(pitchPaket);
  }

  if(rollRound != lastRoll){
    Paket rollPaket;
    rollPaket.os = 2;
    rollPaket.vrijednost = rollRound;
    lastRoll = rollRound;
    sendUDP(rollPaket);
  }

  delay(100);
}


void sendUDP(Paket paket) {
  StaticJsonDocument<256> jsonDocument;
  jsonDocument["Os"] = paket.os;
  jsonDocument["Vrijednost"] = paket.vrijednost;

  String jsonString;
  serializeJson(jsonDocument, jsonString);

  udp.beginPacket(remoteIP, remotePort);
  udp.print(jsonString);
  udp.endPacket();

  Serial.println("Sent UDP message: " + jsonString);
}