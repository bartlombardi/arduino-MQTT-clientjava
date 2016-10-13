#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <string.h>
#include <stdio.h>

const int MPU = 0x68;  // I2C address of the MPU-6050
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
float gForceX, gForceY, gForceZ, rotX, rotY, rotZ;

// Update these with values suitable for your network.
byte mac[]= {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };

IPAddress ip;
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

IPAddress server(192, 168, 1, 100);
//const char* server = "iot.eclipse.org";

void dataReceiver(){
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,14,true);  // request a total of 14 registers
  AcX = Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY = Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  GyX = Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY = Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ = Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
	processData();
}

void processData(){
  gForceX = AcX / 16384.0;
  gForceY = AcY / 16384.0; 
  gForceZ = AcZ / 16384.0;
  
  rotX = GyX / 131.0;
  rotY = GyY / 131.0; 
  rotZ = GyZ / 131.0;
}

void debugFunction(int16_t AcX, int16_t AcY, int16_t AcZ, int16_t GyX, int16_t GyY, int16_t GyZ){
  Serial.print("Accelerometer: ");
  Serial.print("X="); Serial.print(gForceX);
  Serial.print("|Y="); Serial.print(gForceY);
  Serial.print("|Z="); Serial.println(gForceZ);  
  Serial.print("Gyroscope:");
  Serial.print("X="); Serial.print(rotX);
  Serial.print("|Y="); Serial.print(rotY);
  Serial.print("|Z="); Serial.println(rotZ);
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("arduinoClient")){
      Serial.println("connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
//      Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void setup(){
  Serial.begin(9600);
  
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  mqttClient.setServer(server, 1883);
  
  Ethernet.begin(mac);
  ip = Ethernet.localIP();
  
  Serial.println(ip);  
  Serial.println(server);
  //delay(1500);
}

char* init(float val){
  
  char buff[100];

  for (int i = 0; i < 100; i++) {
      dtostrf(val, 4, 2, buff);  //4 is mininum width, 6 is precision
  }
   return buff;

}

void dataAccProducer(){

  char concatenazione[100]= "X=";   
  strcat(concatenazione,init(gForceX));
  strcat(concatenazione,";Y=");
  strcat(concatenazione,init(gForceY));
  strcat(concatenazione,";Z=");
  strcat(concatenazione,init(gForceZ));
  
  mqttClient.publish("Acc", concatenazione);
}


void dataGyProducer(){

  char concatenazione[100]= "X=";
  strcat(concatenazione,init(rotX));
  strcat(concatenazione,";Y=");
  strcat(concatenazione,init(rotY));
  strcat(concatenazione,";Z=");
  strcat(concatenazione,init(rotZ));

  mqttClient.publish("Gy",concatenazione);
}

void loop(){
  dataReceiver();
  debugFunction(AcX,AcY,AcZ,GyX,GyY,GyZ);

  if (!mqttClient.connected()) {
    reconnect();
  }

  mqttClient.loop(); 

  dataAccProducer();
  dataGyProducer();

  delay(50);
}
