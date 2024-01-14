#include <WiFi.h>         
#include <Firebase_ESP_Client.h>                        
#include <Arduino.h> 
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <string>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ssid = "Christian";
const char* password = "Lyoko2004";

#define SERVER "https://motion-dection-real-default-rtdb.firebaseio.com/"
#define AUTH "AIzaSyCe28xHGLr-vcPCOJ5HrkwZ-5MCRF1zwYU"


const int MOTION_SENSOR = 27;

const int RELAY_PIN = 4;

int currentState = LOW;
int previousState = LOW;

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

int count_home, count_not_home = 1;

const int CUTOFF = -80;

#define WIFILED 19
#define FBLED 18

bool lightOn = false;




void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);
  pinMode(WIFILED, OUTPUT);
  pinMode(FBLED, OUTPUT);
  WiFi.begin(ssid, password);

  Serial.print(F("Connecting"));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F("Connected"));
  digitalWrite(WIFILED, HIGH);
  

  config.api_key = AUTH;
  config.database_url = SERVER;

  pinMode(MOTION_SENSOR, INPUT);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.print(F("OK"));
    digitalWrite(FBLED, HIGH);
    signupOK = true;
  }

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  BLEDevice::init("");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (lightOn) {
    checkBt();
  }
  bool home = false;
  if (motionDetected() && Firebase.ready() && signupOK) {

    std::string convertedString;
    if (Firebase.RTDB.getString(&fbdo, "/Dorm/Allowed_Devices")) {
      String allowedDevicesStr = fbdo.stringData();
      convertedString = allowedDevicesStr.c_str();
    }
    //stringToList(allowedDevicesStr);
    BLEScan *scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    BLEScanResults results = scan->start(1);

    for (int i = 0; i < results.getCount(); i++) {
      BLEAdvertisedDevice device = results.getDevice(i);
      std::string deviceAddress = device.getAddress().toString();
      Serial.println(deviceAddress.c_str());
      if (convertedString.find(deviceAddress) != std::string::npos) {
       home = true;
     }
    }
    //Firebase.setBool("/Dorm/Alarm", true);
    if (home) {
      if (Firebase.RTDB.setInt(&fbdo, "Dorm/Alarm/Home", count_home)){
        //This command will be executed even if you dont serial print but we will make sure its working
        Serial.println(F("Movement"));
        if (Firebase.RTDB.getString(&fbdo, "/Dorm/Lights_Home")) {
          String lights_home = fbdo.stringData();
          if (lights_home == "1") {
            digitalWrite(RELAY_PIN, LOW);
            lightOn = true;
          } 
        }
        
      }
      count_home++;
    } else {
      if (Firebase.RTDB.setInt(&fbdo, "Dorm/Alarm/Not_Home", count_not_home)){
         //This command will be executed even if you dont serial print but we will make sure its working
        Serial.println(F("Movement"));
        if (Firebase.RTDB.getString(&fbdo, "/Dorm/Lights_NotHome")) {
          String lights_not_home = fbdo.stringData();
          if (lights_not_home == "1") {
            digitalWrite(RELAY_PIN, LOW);
          } 
        }
      }
      count_not_home++;
    }
    
  } else {
    delay(1000);
  }
}

bool motionDetected() {
  previousState = currentState;
  currentState = digitalRead(MOTION_SENSOR);
  if (currentState == HIGH) {
    return true;
  } else {
    return false;
  }
}

void checkBt() {
  std::string convertedString;
  if (Firebase.RTDB.getString(&fbdo, "/Dorm/Allowed_Devices")) {
    String allowedDevicesStr = fbdo.stringData();
    convertedString = allowedDevicesStr.c_str();
  }
  BLEScan *scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  BLEScanResults results = scan->start(1);

  for (int i = 0; i < results.getCount(); i++) {
    BLEAdvertisedDevice device = results.getDevice(i);
     std::string deviceAddress = device.getAddress().toString();
    //Serial.println(deviceAddress.c_str());
    if (convertedString.find(deviceAddress) != std::string::npos) {
     int rssi = device.getRSSI();
      if (rssi <= CUTOFF) {
      digitalWrite(RELAY_PIN, HIGH);
      lightOn = false;
      Serial.println(rssi);
     } 
    }
  }
}
