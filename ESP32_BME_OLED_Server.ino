//  ESP32 with BME280 and OLED creates web page using WebServer

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ESPmDNS.h"

// COMFIGURE BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>   // include Adafruit BME280 library
Adafruit_BME280 bme;           // initialize Adafruit BME280 library
#define BME280_I2C_ADDRESS  0x76  // define device I2C address: 0x76 or 0x77
//BME280  Pin SDA   >>>>>   ESP32  Pin=GPIO 21 *****  ESP8266  Pin=GPIO4(D2) *****
//BME280  Pin SCL   >>>>>   ESP32  Pin=GPIO 22 *****  ESP8266  Pin=GPIO5(D1) *****
#define SEALEVELPRESSURE_HPA (1013.25)
// My elevation = 44m/144.4 ft  =  + 5.41 Hpa
// For Elevation and Location, Goto: https://www.freemaptools.com/elevation-finder.htm
float temperature, humidity, pressure, altitude;

// CONFIGURE U8x8lib OLED DRIVER
#include "U8x8lib.h"           //  https://github.com/olikraus/U8g2_Arduino
// I2C OLED Display works with SSD1306 driver U8x8lib.h (8 lines of text)
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
String Display_text;

// COMFIGURE WiFi NETWORK
const char*         ssid = "NETGEAR46";         // "Your Network SSID (Name)"
//const char*       ssid = "NETGEAR46_EXT";     // "Your Network SSID (Name)"
//const char*       ssid = "NETGEAR46_2GEXT";   // "Your Network SSID (Name)"
const char*     password = "icysea351";         // "Your Network Password"

WebServer server(80);
String local_hwaddr;                    // WiFi local hardware Address
String local_swaddr;                    // WiFi local software Address
const char* ServerName = "mancave";     // Location of Sensor
#define LED_BUILTIN 2

String Filename() {
  return String(__FILE__).substring(String(__FILE__).lastIndexOf("\\") + 1);
}

//-----------------------------

void setup() {
  Serial.begin(115200);
  delay(4000);         // Time to press "Clear output" in Serial Monitor
  Serial.println("\nProgram ~ " + Filename() + "\n");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);      // OFF

  // SET UP OLED Display
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  // Examples of Display Codes
  //u8x8.clearDisplay();                    // Clear the OLED display buffer.
  //u8x8.drawString(0, 0, "STARTING UP");   // Display on OLED  Normal Size
  //u8x8.draw2x2String(1, 1, "RUNNING");    // Display on OLED  Double Size
  //u8x8.setInverseFont(1);                 // Mode 1= ON 0=OFF
  //u8x8.setFlipMode(1);                    // Mode 1= ON 0=OFF
  //u8x8.setPowerSave(0);                   // Mode 1= ON 0=OFF

  // SET UP BME280 Sensor
  if (!bme.begin(0x76)) {   // Initialize the BME280 define I2C address: 0x76
    Serial.printf("Could not find a valid BMP280 sensor, check wiring!\n");
    while (1)  delay(1000);          // Stay here
  }
  Serial.printf("BMP280 sensor ~ Connected OK\n");

  // CONNECT to WiFi network:
  Serial.printf("WiFi Connecting to %s ", String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.printf(".");
  }
  Serial.printf(" ~ Connected OK\n");

  // PRINT the Signal Strength:
  long rssi = WiFi.RSSI() + 100;
  Serial.printf("Signal Strength ~ %s", String(rssi));
  if (rssi > 50)  Serial.printf("   (>50 - Good)\n");  else   Serial.printf("   (Could be Better)\n");

  if (!MDNS.begin(ServerName)) {   // Start mDNS with ServerName
    Serial.printf("Error setting up MDNS responder!\n");
    while (1)  delay(1000);        // Stay here
  }
  Serial.printf("\nTo Access Web Page :\n");
  local_hwaddr = WiFi.localIP().toString();
  Serial.printf("MDNS started ~\tUse IP Address\t: %s\n", local_hwaddr);
  local_swaddr = "http://" + String(ServerName) + ".local";
  Serial.println("\t\tOr Use Address\t: " + String(local_swaddr));

  // DISPLAY on the OLED
  u8x8.clearDisplay();                  // Clear the OLED display
  u8x8.drawString(0, 0, "WiFi CONNECTED");      // On OLED
  Display_text = String(ssid);
  u8x8.drawString(0, 1, Display_text.c_str());  // On OLED
  Display_text = "Strength = " + String(rssi);
  u8x8.drawString(0, 3, Display_text.c_str());  // On OLED
  Display_text = "IP " + local_hwaddr;
  u8x8.drawString(0, 4, Display_text.c_str());  // On OLED
  Display_text = String(ServerName) + ".local";
  u8x8.drawString(0, 5, Display_text.c_str());  // On OLED
  Display_text = "BMP280 ~ OK";
  u8x8.drawString(1, 7, Display_text.c_str());  // On OLED
  
  delay(8000);                       // 8 Sec Delay to Read
  u8x8.clearDisplay();               // Clear the OLED display
  OLED_Display();                    // Display Parameters On OLED

  Serial.printf("\nTemperature and Pressure Readings\n");
  Serial.printf("\tT: %s  deg F\n", String(temperature * 1.8 + 32));
  Serial.printf("\tT: %s deg C\n", String(temperature));
  Serial.printf("\tH: %s %\n", String(humidity));
  Serial.printf("\tP: %s hPa\n", String(pressure));
  Serial.printf("\tA: %s ft\n", String(altitude * 3.281));
  Serial.printf("\tA: %s m\n", String(altitude));
  
  server.on("/", Handle_Connect);             // Handles Connection
  server.begin();                             // Server Start

  Serial.printf(" ~ HTTP server started\n");
  Serial.printf(" ~ Set Up Complete\n");
  Serial.printf("***********************\n");
}
//-----------------------------

void loop() {                    // State Example
  static unsigned long DelayTime = millis();
  server.handleClient();         // Handles Connection ~ First
  // Update OLED every 5 Seconds
  if (millis() - DelayTime > 5000)  {
    OLED_Display();              // Display Parameters On OLED
    DelayTime = millis();        // Reset delaytime
  }
}
//-----------------------------

void Handle_Connect() {     // Handles Connection
  digitalWrite(LED_BUILTIN, HIGH);     // ON
  Serial.printf("New Connection\n");
  String html_code = Web_Page();
  server.send(200, "text/html", html_code);
  digitalWrite(LED_BUILTIN, LOW);      // OFF
}

void OLED_Display() {       // Display on SSD1306 OLED display
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  //altitude = bme.readAltitude(1013.25);       // Altitude (from Standard Sea Level)
  altitude = bme.readAltitude(pressure + 5.41); // Altitude (this should be adjusted to your elevation)

  // Display On OLED
  u8x8.drawString(2, 0, "Readings:");            // On OLED
  Display_text = "T: " + String(temperature * 1.8 + 32) + " deg F";
  u8x8.drawString(0, 1, Display_text.c_str());   // On OLED
  Display_text = "T: " + String(temperature) + " deg C";
  u8x8.drawString(0, 2, Display_text.c_str());   // On OLED
  Display_text = "H: " + String(humidity) + " %";
  u8x8.drawString(0, 3, Display_text.c_str());   // On OLED
  Display_text = "P: " + String(pressure) + " hPa";
  u8x8.drawString(0, 4, Display_text.c_str());   // On OLED
  Display_text = "A: " + String(altitude * 3.281) + " Ft";
  u8x8.drawString(0, 6, Display_text.c_str());   // On OLED
  Display_text = "A: " + String(altitude) + " m";
  u8x8.drawString(0, 7, Display_text.c_str());   // On OLED
}

String Web_Page() {            // Creates Code
  String header = ServerName;
  String color;
  if (temperature <= 0 ) color = "<FONT COLOR = 'Blue'>"; else color = "";
  if (temperature >= 32.2 ) color = "<FONT COLOR = 'Red'>"; else color = "";
  String html_code = "<!DOCTYPE html><html><HEAD><meta name =\'viewport\' content=\'width=device-width, initial-scale=1.0, user-scalable=no\'>";
  //html_code += "<meta http-equiv='refresh' content='10'>";      // Refresh every 10 sec
  html_code += "<TITLE>ESP32 Station</TITLE>";
  html_code += "<STYLE>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: left;}</STYLE></HEAD>";
  html_code += "<BODY><CENTER><h2>" + header + "</h2>";
  html_code += "Connected to : " + String(ssid) + "<BR>";
  html_code += "IP Address : " + local_hwaddr + "<BR>";
  html_code += "Url Address : " + local_swaddr + "<BR>";
  html_code += "<TABLE BORDER='2' CELLPADDING='5'><FONT SIZE=' + 1'>";
  html_code += "<TR><TD><FONT COLOR='Cyan'>Temperature:</FONT></TD><TD>" + color + String(temperature * 1.8 + 32) + " &deg;F</TD></TR>";
  html_code += "<TR><TD><FONT COLOR='Blue'>Temperature:</FONT></TD><TD>" + color +  String(temperature) + " &deg;C</TD></TR>";
  html_code += "<TR><TD><FONT COLOR='Green'>Humidity:</FONT></TD><TD>" + String(humidity) + " %</TD></TR>";
  html_code += "<TR><TD><FONT COLOR='Magenta'>Pressure:</FONT></TD><TD>" + String(pressure) + " hPa</TD></TR>";
  html_code += "<TR><TD><FONT COLOR='Orange'>Pressure Altitude:</FONT></TD><TD>" + String(altitude * 3.281) + " Ft</TD></TR></TABLE>";
  // html_code += "<BR><IMG SRC ='https://media.giphy.com/media/5upWi4wtfGc7u/giphy.gif'>";    // Cat
  html_code += "<BR><IMG SRC ='https://media.giphy.com/media/MViYNpI0wx69zX7j7w/giphy.gif' width='300' height='300' frameBorder='0'>";  // Fire Works
  html_code += "</CENTER></BODY></HTML>";
  return html_code;
}
