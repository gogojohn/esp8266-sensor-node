#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT.h>
#include "WiFiAuth.h"

#define DHTTYPE DHT22
#define DHTPIN  4
#define LED 0
#define LED_ON 0
#define LED_OFF 1

char MAC_ADDRESS[17];
ESP8266WebServer server(80);


// Initialize DHT sensor 
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an
// ESP8266, you need to increase the threshold for cycle counts considered a 1
// or 0. You can do this by passing a 3rd parameter for this threshold.  It's a
// bit of fiddling to find the right value, but in general the faster the CPU
// the higher the value.
//
// The default for a 16mhz AVR is a value of 6. For an Arduino Due that runs at
// 84mhz a value of 30 works. This is for the ESP8266 processor on ESP-01 
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266
 
float humidity, temp_c;             // values read from sensor
const long interval = 2000;         // interval at which to read from sensor
unsigned long previousMillis = 0;   // last time that sensor was read


void getMeasurements() {
  /*
    Wait at least 2 seconds seconds between measurements. If the difference
    between the current time and last time you read the sensor is bigger than
    the interval you set, read the sensor. Works better than delay for things
    happening elsewhere also.
  */
  
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   
 
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old'
    // (it's a very slow sensor)
    humidity = dht.readHumidity();      // Read humidity (percent)
    temp_c = dht.readTemperature();     // Read temperature as Celcius
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_c)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}


void getWiFiMacAddress(void){
  /*
    Reads the device's Wi-Fi network interface controller (NIC) media access
    control (MAC) address, and constructs a string representation of it which is
    assigned to the MAC_ADDRESS global variable.
    
    The string is formatted as six groups of two zero-padded hexdecimal digits,
    separated by colons, in transmission order.
    
    example: 01:23:45:67:89:AB
  */

  byte mac[6];
  
  WiFi.macAddress(mac);
  sprintf(MAC_ADDRESS, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


long getWiFiRSSI(void){
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
    "\"MAC address\": \"%s\",\n"
    "\"RSSI\": {\"value\": %ld, \"units\": \"dBm\"},\n"
    "\"temperature\": {\"value\": %s, \"units\": \"degrees C\"},\n"
    "\"relative humidity\": {\"value\": %s, \"units\": \"%%\"}\n"
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


void setup(void){
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LED_OFF);
  Serial.begin(115200); 
  WiFi.begin(SSID, PASSWORD);
  getWiFiMacAddress();
  Serial.println("");

  // Waits for connection to Wi-Fi access point to be established, and flash
  // activity LED while waiting.
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, LED_ON);
    delay(125);
    digitalWrite(LED, LED_OFF);
    delay(125);
    digitalWrite(LED, LED_ON);
    delay(125);
    digitalWrite(LED, LED_OFF);
    delay(250);
    Serial.print(".");
  }
  
  // Displays the Wi-Fi connection details, on the serial debug monitor.
  Serial.println("");
  Serial.print("Connected to: ");
  Serial.println(SSID);
  Serial.print("MAC address: ");
  Serial.println(MAC_ADDRESS);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // Registers the site root handler.
  server.on("/", handleRoot);

  // Registers the /measurements REST API endpoint handler.
  server.on("/measurements", handleMeasurements);

  // Registers the not found (404) handler.
  server.onNotFound(handleNotFound);

  // Starts the HTTP server.
  server.begin();
  Serial.println("HTTP server started");
}


void loop(void){
  server.handleClient();
}

