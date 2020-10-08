#include "ThingsBoard.h"
//
#include <WiFi.h>

#define WIFI_AP             "estheim@HUAWEI"
#define WIFI_PASSWORD       "1sampai100"

// See https://thingsboard.io/docs/getting-started-guides/helloworld/
// to understand how to obtain an access token
#define TOKEN               "BkrF79HjeWczA4tXP8FA"
#define THINGSBOARD_SERVER  "demo.thingsboard.io"
#define SensorPulsePin 4

// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;

int BPM =0;
int Signal;                // holds the incoming raw data. Signal value can range from 0-1024
int Threshold = 3450;            // Determine which Signal to “count as a beat”, and which to ingore.
//interupt timer
volatile int interruptCounter;
int totalCapturebeat;
 
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  BPM = totalCapturebeat*10;
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux); 
}


void setup() {
  // initialize serial for debugging
  Serial.begin(115200);
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  InitWiFi();
  // timer 1 sec
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 6000000, true);
  timerAlarmEnable(timer);

}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
  }

  if (!tb.connected()) {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }


  // Uploads new telemetry to ThingsBoard using MQTT.
  // See https://thingsboard.io/docs/reference/mqtt-api/#telemetry-upload-api
  // for more details
  
  Signal = analogRead(SensorPulsePin);
  if(Signal > Threshold){                          // If the signal is above threshold
    digitalWrite(22,HIGH);
    totalCapturebeat++;
    delay(200);
  }else {
    digitalWrite(22,LOW);                //  Else, the sigal must be below threshold
  }
  delay(10);
  
  if (interruptCounter > 0) {

    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
    Serial.println("Sending data...");
    Serial.print(BPM);
    tb.sendTelemetryInt("BPM", BPM);
    totalCapturebeat=0;
  }
  tb.loop();
}

void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void reconnect() {
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to AP");
  }
}
