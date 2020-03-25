// from https://learn.adafruit.com/pm25-air-quality-sensor/arduino-code
//works in getting PM2.5 data from PMS5003
////https://www.instructables.com/id/Arduino-Esp8266-Post-Data-to-Website/

// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (TX pin on sensor), leave pin #3 disconnected
// comment these two lines if using hardware serial
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid     = "Fios-86TE6";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "pad6707plea6697jar";     // The password of the Wi-Fi network
String server = "http://cs3551-airquality.herokuapp.com"; // www.example.com
String uri = "/readings";// our example is /esppost.php
String pm1, pm25, postData;

SoftwareSerial pmsSerial(2, 3);

void reset() {
pmsSerial.println("AT+RST");
delay(1000);
if(pmsSerial.find("OK") ) Serial.println("Module Reset");
}
 
void setup() {
  // our debugging output
  Serial.begin(9600);
 
  // sensor baud rate is 9600
  pmsSerial.begin(9600);

  delay(10);
  Serial.println('\n');
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  reset();
}
 
struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
 
struct pms5003data data;
    
void loop() {
  HTTPClient http;    //Declare object of class HTTPClient
  WiFiClient client; //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/examples/PostHttpClient/PostHttpClient.ino
  
  if (readPMSdata(&pmsSerial)) {
    // reading data was successful!
    Serial.println();
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (standard)");
    Serial.println("PM 1.0: "); Serial.print(data.pm10_standard);
    Serial.println("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
    //Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
    //Serial.println("---------------------------------------");
    //Serial.println("Concentration Units (environmental)");
    //Serial.print("PM 1.0: "); Serial.print(data.pm10_env);
    //Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_env);
    //Serial.print("\t\tPM 10: "); Serial.println(data.pm100_env);
    //Serial.println("---------------------------------------");
    //Serial.print("Particles > 0.3um / 0.1L air:"); Serial.println(data.particles_03um);
    //Serial.print("Particles > 0.5um / 0.1L air:"); Serial.println(data.particles_05um);
    //Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
    //Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
    //Serial.print("Particles > 5.0um / 0.1L air:"); Serial.println(data.particles_50um);
    //Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
    //Serial.println("---------------------------------------");
    pm1 = String(data.pm10_standard);
    pm25 = String(data.pm25_standard);
    postData = "{\"device_id\":\"99\",\"time_cast\":\"23:59\",\"reading_1\":\"" + pm1 + "\",\"reading_2\":\"" + pm25 +"\"}";
    http.begin(client, "http://cs3551-airquality.herokuapp.com/readings");
    http.addHeader("Content-Type", "application/json");    //Specify content-type header
    int httpCode = http.POST(postData);   //Send the request
    String payload = http.getString();    //Get the response payload
 
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
 
  http.end();  //Close connection
    delay(10000);
    delay(10000);
  }
}
 
boolean readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }
  
  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }
 
  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }
    
  uint8_t buffer[32];    
  uint16_t sum = 0;
  s->readBytes(buffer, 32);
 
  // get checksum ready
  for (uint8_t i=0; i<30; i++) {
    sum += buffer[i];
  }
 
  /* debugging
  for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
  }
  Serial.println();
  */
  
  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i=0; i<15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }
 
  // put it into a nice struct :)
  memcpy((void *)&data, (void *)buffer_u16, 30);
 
  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}
