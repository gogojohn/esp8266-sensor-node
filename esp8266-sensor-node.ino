#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#include "SemanticVersion.h"
#include "WiFiAuth.h"


#define DHTTYPE DHT22
#define DHTPIN 5
#define LED 0
#define LED_ON 0
#define LED_OFF 1

// Enables ESP8266 to read the module supply voltage (Vcc)
ADC_MODE(ADC_VCC);

char IP_ADDRESS[16];
char MAC_ADDRESS[18];
char DATE_TIME_START[21];
char DATE_TIME_CURRENT[21];

// Instantiates the ESP8266 Web Server which will handle incoming HTTP requests
ESP8266WebServer server(80);

// Instantiates the NTP client which will handle time synchronization
WiFiUDP ntpUDP;
static const char ntp_server_pool[] = "pool.ntp.org";
static const int offset_s = 0;
static const unsigned int interval_ms = 60000;
NTPClient timeClient(ntpUDP, ntp_server_pool, offset_s, interval_ms);

// Instantiates the DHT sensor which will perform measurements
DHT_Unified dht(DHTPIN, DHTTYPE);
float humidity, temp_c;             // values read from sensor
const long interval = 2000;         // interval at which to read from sensor
unsigned long previousMillis = 0;   // last time that sensor was read


void getISO8601DateTimeString(char *date_time_ptr) {
  /*
    Updates the value of the variable (pointed to by the date_time_ptr argument)
    where the current ISO 8601 formatted datetime string will be stored.
    
    example: 2021-01-01T03:36:48Z
  */
    
  unsigned long epoch_time;
      
  // Syncronizes the current time (if necessary), determines the epoch time.
  timeClient.update();
  epoch_time = timeClient.getEpochTime();

  if (epoch_time > 0) {
    TimeElements tm;
    breakTime(epoch_time, tm);
    sprintf(date_time_ptr, "%i-%02i-%02iT%02i:%02i:%02iZ",
      tmYearToCalendar(tm.Year),
      tm.Month,
      tm.Day,
      tm.Hour,
      tm.Minute,
      tm.Second);
  }  
}


void getMeasurements() {
  /*
    Attempts to acquire temperature and relative humidity measurements, and
    stores the values in for subsequent use.
  */
  
  unsigned long currentMillis = millis();
 
  /*
    Wait at least 2 seconds seconds between measurements. If the difference
    between the current time and last time you read the sensor is bigger than
    the interval you set, read the sensor. Works better than delay for things
    happening elsewhere also.
  */
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   

    // Gets the temperature event and stores its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      // Serial.print(F("Temperature: "));
      // Serial.print(event.temperature);
      // Serial.println(F("°C"));
      temp_c = event.temperature;
    }

    // Gets the humidity event and stores its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    }
    else {
      // Serial.print(F("Humidity: "));
      // Serial.print(event.relative_humidity);
      // Serial.println(F("%"));
      humidity = event.relative_humidity;
    }    
  }
}


void getWiFiIPAddress(){
    /*
      Reads the device's assigned IP address, and constructs a string
      representation of it which is assigned to the IP_ADDRESS global variable.
    
      The string is formatted as 4 octets of decimal digits.
    
      example: 192.168.0.1
    */
    
    IPAddress ip = WiFi.localIP();
    sprintf(IP_ADDRESS, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    
}


void getWiFiMACAddress(){
  /*
    Reads the device's Wi-Fi network interface controller (NIC) media access
    control (MAC) address, and constructs a string representation of it which
    is assigned to the MAC_ADDRESS global variable.
    
    The string is formatted as six groups of two zero-padded hexdecimal digits,
    separated by colons, in transmission order.
    
    example: 01:23:45:67:89:AB
  */

  byte mac[6];
  
  WiFi.macAddress(mac);
  sprintf(MAC_ADDRESS, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


long getWiFiRSSI(){
  /*
    Reads the received signal strength indicator (RSSI), for the wireless access
    point that the device's Wi-Fi interface is currently associated with, and
    returns it.
  */

  return WiFi.RSSI();
}


void handleRoot() {
  /*
    Site root request handler: returns an HTTP response, bearing details for
    how to interact with the sensor node.
  */
  
  digitalWrite(LED, LED_ON);
  String message = "<!DOCTYPE html>";
  message += "<html>";
  message += "<body>";
  message += "<h1>My First Heading</h1>";
  message += "<p>My first paragraph.</p>";
  message += "<script>alert('hello, you!');</script>";
  message += "</body>";
  message += "</html>";
  server.send(200, "text/html", message);
  digitalWrite(LED, LED_OFF);
}


void handleHealth() {
  /*
    REST API health endpoint request handler: returns an HTTP response, bearing
    a JSON-formatted string that contains the following elements:
    
    (1) general health status
    (2) uptime status
    (3) memory utilization
    (4) module supply voltage (Vcc)
  */
    
  char message[1024];
  
  // Turns the activity LED on (briefly), while handling request.
  digitalWrite(LED, LED_ON);
  
  // Syncronizes the current time (if necessary), and builds the JSON message
  // payload for the HTTP response.
  timeClient.update();
  getISO8601DateTimeString(DATE_TIME_CURRENT);
  
  
  // Acquires the current ESP8266 module supply voltage (Vcc) and then
  // converts the float value to a string, as the Arduino implementation of
  // sprintf() doesn't work with float values.
  float rawVcc = ((float)ESP.getVcc())/1024;
  char convertedVcc[6];
  dtostrf(rawVcc, 4, 3, convertedVcc);
  
    
  sprintf(message,
    "{\n"
      
    "  \"status\": \"pass\",\n"
    "  \"version\": \"1\",\n"
    "  \"releaseID\": \"%s\",\n"                    // FIRMWARE_SEMVER
    "  \"notes\": [\"\"],\n"
    "  \"output\": \"\",\n"
    "  \"serviceID\": \"\",\n"
    "  \"description\": \"health of remote sensor node\",\n"
    
    "  \"checks\": {\n"
    "    \"uptime\": [\n"
    "      {\n"
    "        \"componentType\": \"system\",\n"
    "        \"observedValue\": %lu,\n"             // millis()
    "        \"observedUnit\": \"ms\",\n"
    "        \"status\": \"pass\",\n"
    "        \"time\": \"%s\"\n"                    // DATE_TIME_CURRENT
    "      }\n"
    "    ],\n"
      
    "    \"memory:utilization\": [\n"
    "      {\n"
    "        \"componentId\": \"\",\n"
    "        \"componentType\": \"system\",\n"
    "        \"observedValue\": %u,\n"              // ESP.getFreeHeap()
    "        \"observedUnit\": \"B\",\n"
    "        \"status\": \"pass\",\n"
    "        \"time\": \"%s\",\n"                   // DATE_TIME_CURRENT
    "        \"output\": \"\"\n"
    "      }\n"
    "    ],\n"
        
    "    \"power:supply-voltage\": [\n"
    "      {\n"
    "        \"componentId\": \"\",\n"
    "        \"componentType\": \"system\",\n"
    "        \"observedValue\": %s,\n"              // convertedVcc
    "        \"observedUnit\": \"V\",\n"
    "        \"status\": \"pass\",\n"
    "        \"time\": \"%s\",\n"                   // DATE_TIME_CURRENT
    "        \"output\": \"\"\n"
    "      }\n"
    "    ]\n" 
    "  }\n"
      
    "}",
    FIRMWARE_SEMVER,
    millis(),
    DATE_TIME_CURRENT,
    ESP.getFreeHeap(),
    DATE_TIME_CURRENT,
    convertedVcc,
    DATE_TIME_CURRENT
  );
    
  // Sends the HTTP response, containing the constructed message payload.
  server.send(200, "application/health+json", message);

  // Turns the activity LED off, upon completion of request handling.
  digitalWrite(LED, LED_OFF);  
}


void handleMeasurements() {
  /*
    REST API measurement data request handler: returns an HTTP response, bearing
    a JSON-formatted string that contains the following elements:
     
    (1) MAC address of sensor node's Wi-Fi network interface controller
    (2) received signal strength indicator (RSSI) measurement, of the Wi-Fi
        access point that the sensor node is connected to, and units (in dBm)
    (3) temperature measurement value, and units (in degrees Celcius)
    (4) relative humidity value, and units (in %)
  */
    
  char message[1024];
  char temperature[7];
  char relative_humidity[7];
        
  // Turns the activity LED on (briefly), while handling request.
  digitalWrite(LED, LED_ON);
  
  // Acquires the temperature and humidity measurement data, from the sensor.
  getMeasurements();
  
  // Converts the float values to a string, as the Arduino implementation of
  // sprintf() doesn't work with float values.
  dtostrf(temp_c, 4, 1, temperature);
  dtostrf(humidity, 4, 1, relative_humidity);
  
  // Constructs the JSON-formatted message, to return in the HTTP response.
  sprintf(message,
    "{\n"
    "  \"MAC address\": \"%s\",\n"
    "  \"RSSI\": {\"value\": %ld, \"units\": \"dBm\"},\n"
    "  \"temperature\": {\"value\": %s, \"units\": \"degrees C\"},\n"
    "  \"relative humidity\": {\"value\": %s, \"units\": \"%%\"}\n"
    "}",
    MAC_ADDRESS,
    getWiFiRSSI(),
    temperature,
    relative_humidity
    );
  
  // Sends the HTTP response, containing the constructed message payload.
  server.send(200, "application/json", message);
  
  // Turns the activity LED off, upon completion of request handling.
  digitalWrite(LED, LED_OFF);
}


void handleNotFound(){
  /*
    Not found request handler: returns an HTTP response, for URLs that do not
    exist (e.g. 404: NOT FOUND).
  */
  
  digitalWrite(LED, LED_ON);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED, LED_OFF);
}


void registerHTTPEndpointHandlers(){
  /*
    Registers each of the HTTP endpoint handlers with the HTTP server.
  */
  
  // Registers the site root handler.
  server.on("/", handleRoot);

  // Registers the /health RESP API endpoint handler.
  server.on("/health", handleHealth);

  // Registers the /measurements REST API endpoint handler.
  server.on("/measurements", handleMeasurements);

  // Registers the not found (404) handler.
  server.onNotFound(handleNotFound);
}


void startHTTPServer(){
  /*
    Starts the HTTP server.
  */
  
  server.begin();
  timeClient.update();
  getISO8601DateTimeString(DATE_TIME_CURRENT);
  Serial.printf_P(
      PSTR("HTTP server started: %s\n"),
      DATE_TIME_CURRENT);  
}

void startMDNSResponder(){
  /*
    Starts the Multicast DNS (MDNS) responder.
  */
  
  if (MDNS.begin("esp8266")) {
    timeClient.update();
    getISO8601DateTimeString(DATE_TIME_CURRENT);
    Serial.printf_P(
        PSTR("MDNS responder started: %s\n"),
        DATE_TIME_CURRENT);
  }
}


void startNTPClient(){
  /*
    Starts the NTP client, synchronizes the time, and then stores the start
    time for the purpose of knowing how long the node has been running for
    since it was last (re)started.
  */

  // Starts the NTP client, and synchronizes the time.
  timeClient.begin();
  timeClient.update();
  getISO8601DateTimeString(DATE_TIME_START);
  Serial.printf_P(
      PSTR("NTP client started: %s\n"),
      DATE_TIME_START);
}


void setup(){
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LED_OFF);
  
  // Starts the serial port object, then prints the firmware version to the
  // serial debug port.
  Serial.begin(115200);
  Serial.println(F(""));
  Serial.printf_P(
      PSTR("ESP8266 Sensor Node firmware version: %s\n"),
      FIRMWARE_SEMVER);
  
  // Starts the DHT sensor object.
  dht.begin();
  
  // Starts the Wifi object, then waits for connection to Wi-Fi access point to
  // be established, and flashes activity LED while waiting.
  WiFi.begin(SSID, PASSWORD);
  Serial.print(F("Attempting to connect to WiFi"));
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, LED_ON);
    delay(125);
    digitalWrite(LED, LED_OFF);
    delay(125);
    digitalWrite(LED, LED_ON);
    delay(125);
    digitalWrite(LED, LED_OFF);
    delay(250);
    Serial.print(F("."));
  }

  // Stores the string representation of the MAC address.
  getWiFiMACAddress();

  // Stores the string representation of the assigned IP address.
  getWiFiIPAddress();
  
  // Displays the Wi-Fi connection details, on the serial debug monitor.
  Serial.printf_P(PSTR("\nConnected to: %s\n"), SSID);
  Serial.printf_P(PSTR("MAC address: %s\n"), MAC_ADDRESS);
  Serial.printf_P(PSTR("IP address: %s\n"), IP_ADDRESS);
  
  // Starts the NTP client, and synchronizes the time.  
  startNTPClient();
      
  // Starts the multicast DNS responder.
  startMDNSResponder();

  // Registers the HTTP endpoint handlers.
  registerHTTPEndpointHandlers();

  // Starts the HTTP server.
  startHTTPServer();
}


void loop(){
  server.handleClient();
}

