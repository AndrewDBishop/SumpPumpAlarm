
#define DATA_INTERVAL 30  // time in seconds between sending status messages
#define MQTT_CLIENT_ID "sumpAlarm" // MQTT Unique Client ID can be changed if preferred
#define STATUS_TOPIC_ALARM "status/basement/sumpalarm/alarm"
#define STATUS_TOPIC_CONTROLLER "status/basement/sumpalarm//heartBeat" //MQTT State Topic to show if controller is on WIFI and connected to MQTT or not
#define STATUS_TOPIC_RSSI "status/basement/sumpalarm/rssi" //mqtt topic to send WIFI RSSI data to
#define SERIAL_BAUD 115200 //Serial Baud Rate for USB debugging
#define WATER_SENSOR_PIN 34
const char* VERSION = "2021-02-16-001";
const char* WIFI_SSID ="Skyrim_IoT"; // WIFI SSID
const char* WIFI_PASSWORD ="1Ring2RuleThemAll"; // WIFI Password
const char* MQTT_HOST ="192.168.2.10"; // IP Address of your OpenHab or other MQTT Server
const char* OTA_PASSWORD = "134679"; // Password for OTA updates
const char* OTA_HOSTNAME = "sumpAlarm"; // mDNS Hostname for OTA updates
