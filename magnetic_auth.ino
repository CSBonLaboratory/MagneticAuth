
#include <WiFiMulti.h>
#include "magnetic_influx.h"

#define DEVICE "ESP32"

#define BUTTON_PIN 34

#define WIFI_SSID "UPB-Guest"

#define WIFI_PASSWORD ""

#define TZ_INFO "WET0WEST,M3.5.0/1,M10.5.0"

#define ADD_USER 0

#define AUTH_USER 1

#define DEBOUNCE_DELAY 5000

WiFiMulti wifiMulti;

int configureUserId(){
  Serial.print("Input id:");
  bool isFound = false;
  int id;
  while(Serial.available() == 0);
  id = Serial.parseInt();
  Serial.println();
  return id;
}

// This is the sexiest piece of code I have ever written... so far 
void doUntilSerialInput(void(*task_functional)(void*), void *task_param){
  while(1){

    if(task_functional != NULL){
      task_functional(task_param);
    }

    if(Serial.available() > 0){
      String response = Serial.readString();
      response.trim();
      if(response == "y"){
        return;
      }
    }
  }
}

// And this one as well
void doUntilButtonPress(void(*task_functional)(void*), void *task_param){
  int buttonState = LOW;
  int lastDebounceTime = 0;

  while(1){

    if(task_functional != NULL){
      task_functional(task_param);
    }
    int reading = digitalRead(BUTTON_PIN);
    if(reading == 1)
      Serial.println(reading);

    if(reading != buttonState){
      lastDebounceTime = millis();
    }

    if((millis() - lastDebounceTime) > DEBOUNCE_DELAY){

      if(reading != buttonState){

        buttonState = reading;

        if(buttonState == LOW){
          return;
        }
      }
    }
  }
}

void sampleHallSensor(void* __unused__){
  int val = hallRead();
  Serial.println(val);
  writeFluxPoint(val);
  delay(50);
}

void startHallSession(int flag, int id){

  if(flag == ADD_USER){
    Serial.println("Starting adding user process.");
  }
  else{
    Serial.println("Starting authentication user process.");
  }

  Serial.println("Press 'y' to start. Press 'y' again to stop");
  doUntilSerialInput(NULL, NULL);

  unsigned long startTimeSerie = millis();
  initTimeSeries(id, flag, startTimeSerie);

  doUntilSerialInput(sampleHallSensor, NULL);
  Serial.println("Finished reading from hall sensor.");
  

  if(flag == AUTH_USER){
    Serial.print("Authenticating user ");
    Serial.print(id);
    Serial.print(" trying to get series from -");
    Serial.print(millis() - startTimeSerie);
    Serial.println("miliseconds from current time");
  }
  else{
    Serial.print("Added user ");
    Serial.println(id);
  }

}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  // Wifi connection
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check InfluxDB connection
  while(connectToBuckets() == -1){
    delay(1000);
  }

}

void loop() {

  int id = configureUserId();

  int isFound = userExists(id);

  switch(isFound){
    case -1:
      break;
    case 0:
      startHallSession(ADD_USER, id);
      startHallSession(AUTH_USER, id);
      break;
    case 1:
      startHallSession(AUTH_USER, id);
      break;
  }

  
 
}
