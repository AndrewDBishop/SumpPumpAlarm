#include <EspMQTTClient.h>
#include <Wire.h>
#include <Arduino.h>
#include <ESP32_MailClient.h>
#include <ArduinoOTA.h>
#include <ACROBOTIC_SSD1306.h>
#include <config.h>

//for email
String MAIL_USER = "rivrsdhubfam@gmail.com"; 
String MAIL_PASSWORD = "bzidydrgtgrwvinw";
String MAIL_SENDER = "rivrsdhubfam@gmail.com";
String MAIL_TO = "4169708324@txt.bell.ca";

//variables for Water Level Sensor
int waterLevel = 0;
boolean alarmTriggered;
String message = "";
boolean emailSent;

//WIFI and MQTT client
EspMQTTClient client(WIFI_SSID, WIFI_PASSWORD, MQTT_HOST, MQTT_CLIENT_ID, 1883);
//The Email Sending data object contains config and data to send
SMTPData smtpData;

// HW Timer
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

/*********************Send HeartBeat******************/
void sendHeartBeat() {
  client.publish(STATUS_TOPIC_CONTROLLER, "ONLINE");
}

/**********************Send RSSi**************************************/
void sendRSSi() {
    long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);
  client.publish (STATUS_TOPIC_RSSI, String(rssi));
}

//Callback function to get the Email sending status
void sendCallback(SendStatus msg)
{
  //Print the current status
  Serial.println(msg.info());

  //Do something when complete
  if (msg.success())
  {
    Serial.println("----------------");
  }
}

void sendAlarmActiveEmail() {
  client.publish(STATUS_TOPIC_ALARM, "ACTIVE");
  Serial.println("Alarm Active Sending email...");
  //Check if email sent already
 if (!emailsent) {
  smtpData.setDebug(true);
  //Set the Email host, port, account and password
  smtpData.setLogin(MAIL_HOST, MAIL_PORT, MAIL_USER, MAIL_PASSWORD);
  smtpData.setSTARTTLS(true);
  //Set the sender name and Email
  smtpData.setSender("Riverside SUMP Pump", MAIL_SENDER);
  //Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");
  //Set the subject
  smtpData.setSubject("ALARM SUMP PUMP WATER LEVEL IS HIGH!!!");
  //Set the message
  String messageText = "ALARM! Sump Pump Water level is HIGH.Water Level is: ";
  String message = messageText + waterLevel;
  smtpData.setMessage(message, false);
  //Add recipients, can add more than one recipient
  smtpData.addRecipient(MAIL_TO);
  smtpData.setSendCallback(sendCallback);
  //Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
  } else { emailSent = true; }
  //Clear all data from Email object to free memory
  smtpData.empty();
 } 
}

void sendAlarmClearEmail() {
  Serial.println("Alarm Cleared Sending email...");
  client.publish(STATUS_TOPIC_ALARM, "CLEAR");
  //Setup email sender
  smtpData.setDebug(true);
  //Set the Email host, port, account and password
  smtpData.setLogin(MAIL_HOST, MAIL_PORT, MAIL_USER, MAIL_PASSWORD);
  smtpData.setSTARTTLS(true);
  //Set the sender name and Email
  smtpData.setSender("Riverside SUMP Pump", MAIL_SENDER);
  //Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");
  //Set the subject
  smtpData.setSubject("ALARM CLEARED SUMP PUMP WATER LEVEL IS DROPPING");
  //Set the message - normal text or html format
  String messageText = "ALARM! Sump Pump water level is dropping. Water level is: ";
  String message = messageText + waterLevel;
  Serial.println("Message text for email is:");
  Serial.println(message);
  smtpData.setMessage(message, false);
  //Add recipients, can add more than one recipient
  smtpData.addRecipient(MAIL_TO);
  smtpData.setSendCallback(sendCallback);

  //Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
  }
  //Clear all data from Email object to free memory
  smtpData.empty();
}



/*********************************** OTA SETUP******************************************/
void setupOTA() {
  Serial.println("WIFI is up, starting OTA Setup");
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("OTA Setup finished");
}

/************************Setup HearBeat****************************************/
void setupHeartBeat() {
  //Timer for Heartbeat to OpenHab via MQTT
   timerSemaphore = xSemaphoreCreateBinary();
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, DATA_INTERVAL*1000000, true);
  timerAlarmEnable(timer);
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
  setupHeartBeat();
  Serial.println("Setup water sensor");
  //setup water sensor
  alarmTriggered = false;
  emailSent = false ;
  Serial.println("Set analogReadResolution");
  analogReadResolution(9);
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
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
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
        Serial.println("Sending Alarm clear SMS");
        oled.setTextXY(5, 0);
        oled.putString("Alarm Clear");
        // Send Alarm Clear SMS
        sendAlarmClearEmail();
        alarmTriggered = false;
        delay(2000);
        oled.setTextXY(5, 0);
        oled.putString("                "); // clear the line
        oled.setTextXY(5, 0);
      }
    }
    if (waterLevel < 500) {
      Serial.println("ALARM Water Level HIGH sending SMS ALERT!");
      oled.setTextXY(5, 0);
      oled.putString("ALARM!!");
      sendAlarmActiveEmail();
      alarmTriggered = true;
    }
    //fix to keep MQTT state in sync
    if (!alarmTriggered) {
      client.publish(STATUS_TOPIC_ALARM, "CLEAR");
    }
  }
}