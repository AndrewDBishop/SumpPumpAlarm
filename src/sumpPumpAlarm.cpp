#include <EspMQTTClient.h>
#include <Wire.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ACROBOTIC_SSD1306.h>
#include <config.h>



//variables for Water Level Sensor
int waterLevel = 0;
boolean alarmTriggered;
unsigned long previousMillis = 0;

//WIFI and MQTT client
EspMQTTClient client(WIFI_SSID, WIFI_PASSWORD, MQTT_HOST, MQTT_CLIENT_ID, 1883);


/*********************Send HeartBeat*****************************/
void sendHeartBeat() {
  client.publish(STATUS_TOPIC_CONTROLLER, "ONLINE");
}

/**********************Send RSSi********************************/
void sendRSSi() {
    long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);
  client.publish (STATUS_TOPIC_RSSI, String(rssi));
}

/********************Send Alarm Active*************************/
void sendAlarmActiveMessage() {
  client.publish(STATUS_TOPIC_ALARM, "ACTIVE");
  
}
/******************Send Alarm clear **************************/
void sendAlarmClearMessage() {
  Serial.println("Alarm Cleared Sending Message...");
  client.publish(STATUS_TOPIC_ALARM, "CLEAR");
}

/*********************************** OTA SETUP****************/
void setupOTA()
{
  Serial.println("Starting setipOTA");
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(3232);
  Serial.println("Setting OTA onStart");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  Serial.println("Setting OTA onEnd");
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  Serial.println("Setting OTA onProgress");
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  Serial.println("Setting OTA onError");
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });
  Serial.println("Calling OTA begin");
  ArduinoOTA.begin();
  Serial.println("OTA Setup Finished");
}


void setupOled() {
//OLED setup
  Wire.begin();
  oled.init();                      // Initialze SSD1306 OLED display
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0, 0);             // Set cursor position, start of line 0
  //Show setup status
  oled.putString("Sump Pump Alarm");
  oled.setTextXY(1, 0);
  Serial.println(F("Sump Pump Alarm"));
  Serial.print("Version:");
  Serial.println(VERSION);
  oled.putString(VERSION);
}


void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  setupOled();
  Serial.println("Setup water sensor");
  //setup water sensor
  alarmTriggered = false;
  Serial.println("Set analogReadResolution");
  //analogReadResolution(9);
  Serial.println("Set analogSetAttenuation");
 //analogSetAttenuation(ADC_11db);
 //MQTT LastWill Message - used for HeartBeat
 Serial.println("Set LastWillMessage");
  client.enableLastWillMessage(STATUS_TOPIC_CONTROLLER, "OFFLINE");
  Serial.println("Main setup complete");
}

void onConnectionEstablished() {
  // Publish a message to Status topic for the controller
  client.publish(STATUS_TOPIC_CONTROLLER, "ONLINE");
 client.publish(STATUS_TOPIC_ALARM, "CLEAR");
 setupOTA();
}

void loop() {
  client.loop();
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= DATA_INTERVAL*1000) {
    sendHeartBeat();
    sendRSSi();
    waterLevel = analogRead(WATER_SENSOR_PIN);
    Serial.print("Waterlevel:");
    Serial.println(waterLevel);
    oled.setTextXY(2, 0);
    oled.putString("                ");
    oled.setTextXY(2, 0);
    oled.putString("Water Level:");
    oled.putNumber(waterLevel);
    oled.setTextXY(3, 0);
    oled.putString("IP:" + WiFi.localIP().toString());
    oled.setTextXY(4, 0);
    oled.putString("                ");
    oled.setTextXY(4, 0);
    if (client.isMqttConnected()) {
      oled.putString("MQTT UP");
    }
    if (!client.isMqttConnected()) {
      oled.putString("MQTT DOWN!!!");
    }
    if (alarmTriggered) {
      Serial.println("Alarm State active, checking if water level has dropped");
      waterLevel = analogRead(WATER_SENSOR_PIN);
      if (waterLevel > 500) {
        Serial.print("Alarm State cleared, water level is:");
        Serial.println(waterLevel);
        Serial.println("Sending Alarm clear Message");
        oled.setTextXY(5, 0);
        oled.putString("Alarm Clear");
        // Send Alarm Clear SMS
        sendAlarmClearMessage();
        alarmTriggered = false;
        delay(2000);
        oled.setTextXY(5, 0);
        oled.putString("                "); // clear the line
        oled.setTextXY(5, 0);
      }
    }
    if (waterLevel < 500) {
      Serial.println("ALARM Water Level HIGH sending ALARM Message!");
      oled.setTextXY(5, 0);
      oled.putString("ALARM!!");
      sendAlarmActiveMessage();
      alarmTriggered = true;
    }
    //fix to keep MQTT state in sync
    if (!alarmTriggered) {
      client.publish(STATUS_TOPIC_ALARM, "CLEAR");
    }
  }
}