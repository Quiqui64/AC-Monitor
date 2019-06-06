// Using esp8266 node mcu, a AM2302 Temp Humidity sensor
// The AM2302 uses 3.3-5.5V DC Data Sheet link https://cdn-shop.adafruit.com/datasheets/Digital+humidity+and+temperature+sensor+AM2302.pdf
// Here is the info for the current porstion https://www.hackster.io/whatnick/esp8266-iot-energy-monitor-b199ed
// Things to do
// 1. Send temp and hum when change ed by 1 and after certain amount of time
// 2. calibrate amps with new coil
/* Comment this out to disable prints and save space */
#define DHTPIN D7        // What digital pin we're connected to AM2302 3.3-5.5V DC
#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321

#include <DHT.h> // For DHT22 Temp Humity sensor
#include <Wire.h>
#include <Adafruit_ADS1015.h> // For analog to digital card
#include <ESP8266WiFi.h> // For OTA
#include <ESP8266mDNS.h> // For OTA
#include <WiFiUdp.h> // For OTA
#include <ArduinoOTA.h> // For OTA
#include <ESP8266HTTPClient.h>

// Define Variables
// Your WiFi credentials.
// Set password to "" for open networks.
const char* ssid = "????"; // your router login mame
const char* password = "???"; // your router password
// Hubitat hub information
static String hubIp = "???.???.?.???"; // i.e. 192.168.1.180
const unsigned int hubPort = 39501; // Hubitat hub port, don't change this
static String accessToken = "???"; // You get this form the Hubitat Maker API, needs to be inside " " i.e. "e1d2888d-c8d3-4828-b314-6551e6b67115"
String appID = "???"; // Enter your MakerAPI app number " ", i.e. "875"
String deviceID = "???"; // Enter your MakerAPI device ID number " ", i.e. "579"
// End of Hubitat information
// Below you can adjust how often you want the program to report to Hubitat, be careful and don't set it up toreport to often you may flood Hubitat with data
const unsigned long tempHumSampleTime = 60000; // You can set how often the temature and humity is checked, i.e. 60000ms = 60 seconds
const unsigned long currentSampleTime = 1000; // You can set how often the current is sent to Hubitat, i.e. 1000ms = 1 seconds
const double currentSampleRate = .1; // you can set the amount of change in amperage is needed before a report is to Hubitat i.e. .1 = 1/10 of a amp
// below is are the variables for the program
String hubitatData; // The data being sent to hubitat
String command; // holds the command data
String secondaryValue; // holds secondary value i.e.tempature, humidity values.
double lastCurrent; // stores the last amp reading sent to Hubitat 
int iterations = 5;
unsigned long previousMillisTempHum = millis();// Keep track of last time temp and humidity was checked.
unsigned long previousMillisCurrent = millis();// Keep track of last time current was checked.
const unsigned int serverPort = 8090; // port to run the http server on esp8266, for hubitat hub
WiFiServer server(serverPort); //server
WiFiClient client; //client
DHT dht(DHTPIN, DHTTYPE);//For temp sensor
Adafruit_ADS1115 ads(0x48);  /* Use this for the 16-bit version */

void setup()
{
  WiFi.hostname("Matt's AC Monitor"); // Name displayed on router
  // Debug console
  Serial.begin(115200);
  dht.begin();
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
   // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("Matt's AC Monitor");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Start the server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.println(WiFi.localIP());  
}

void loop()
{
   ArduinoOTA.handle();

  if((millis() - previousMillisTempHum) > tempHumSampleTime)// you can change how ofted the temp and humity is checked 60,000 milli secounds = 1 minute
           {
             previousMillisTempHum = millis(); // get ready for next notificationTime
             Serial.println("Checking temp");   
             sendTempHum();
           }
  double current = calcIrms(256) * .1426025; // calcIrms(number of samples) * cal factor
//    if((current - lastCurrent) > currentSampleRate || (lastCurrent - current) > currentSampleRate ){
    if(((current - lastCurrent) > currentSampleRate || (lastCurrent - current) > currentSampleRate) && ((millis() - previousMillisCurrent) > currentSampleTime)){
      lastCurrent = current;
      String stringCurrent = String(current); //convert to string
      secondaryValue = String("/" + stringCurrent); // used to store temp
//      Serial.println(secondaryValue);
      command = "setCurrentMeter";
      sendData1();
      Serial.println(current);
  }
}


void sendData1(){
    
//if ((WiFiMulti.run() == WL_CONNECTED)) {
if (client.connect(hubIp, hubPort)){
    WiFiClient client;

    HTTPClient http;
    hubitatData = String("http://" + hubIp + "/apps/api/" + appID + "/devices/" + deviceID + "/" + command + secondaryValue + "?access_token=" + accessToken);
//                         http://192.168.1.180/apps/api/875/devices/[Device ID]/[Command]/[Secondary value]?access_token=e1d2777d-
//    Serial.print("[HTTP] begin...\n");
     if (http.begin(client, hubitatData)) {  // HTTP

//      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
//        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
//          Serial.println(payload);
        }
      } else {
//        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
//      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  client.stop();
}

void sendTempHum()
{
  float hum;    // Stores humidity value in percent
  float temp;   // Stores temperature value in Celcius
  static int lastTempF; // Stores the last sent temperature value in F
  static int LastHumR;  // Stores the last sent humidity value in percent
  hum = dht.readHumidity();  // Get Humidity value
  temp = dht.readTemperature();  // Get Temperature value
  int tempF = ((1.8 * temp) + 32);
  int humR = round(hum);
  if(humR != LastHumR){
    LastHumR = humR;
    String stringHum = String(humR); //convert to string
    secondaryValue = String("/" + stringHum); // used to store hum
    command = "setHumidity"; // the command being sent to Hubitat maker API
    sendData1();
  }
  if(tempF != lastTempF){
    lastTempF = tempF;
    String stringTempF = String(tempF); //convert to string
    secondaryValue = String("/" + stringTempF); // used to store temp
    command = "setTemperature"; // the command being sent to Hubitat maker API
    sendData1();
  }
}

double calcIrms(unsigned int Number_of_Samples)
{
  /* Be sure to update this value based on the IC and the gain settings! */
  float multiplier = 0.125F;    /* ADS1115 @ +/- 4.096V gain (16-bit results) */
  double offsetI;
  double filteredI;
  double sqI,sumI;
  int16_t sampleI;
  double Irms;
  for (unsigned int n = 0; n < Number_of_Samples; n++)
  {
    sampleI = ads.readADC_Differential_0_1();
    // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset, 
    //  then subtract this - signal is now centered on 0 counts.
    offsetI = (offsetI + (sampleI-offsetI)/1024);
    filteredI = sampleI - offsetI;
    // Root-mean-square method current
    // 1) square current values
    sqI = filteredI * filteredI;
    // 2) sum 
    sumI += sqI;
  }  
  Irms = squareRoot(sumI / Number_of_Samples)*multiplier; 
  //Reset accumulators
  sumI = 0;
  return Irms;
}
double squareRoot(double fg)  
{
  double n = fg / 2.0;
  double lstX = 0.0;
  while (n != lstX)
  {
    lstX = n;
    n = (n + fg / n) / 2.0;
  }
  return n;
}
