#include "DHTesp.h"
#include <WiFiManager.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "time.h"


/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPLREEV0yMB"
#define BLYNK_TEMPLATE_NAME         "Quickstart Device"
#define BLYNK_AUTH_TOKEN            "luRf7ofHZvpwFjQZIKcLy4Smn9wYyZmI"

#define BLYNK_PRINT Serial

#define DHTpin 15    //D15 of ESP32 DevKit
#define BUTTON_PIN 21
int moisture_pin = 36;
int light_pin = 34;
int pump_pin = 18;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;


const int OpenAirMoistureReading = 3500;
const int WaterReading = 1700;
const int darkestMes = 4095;
const int lightestMes = 0;
const int avgMesNum = 10; // na kolku merenja da se napravi avg
const int timeForMes = 2000L; // na kolku msekundi da se odviva sekoe merenje

DHTesp dht;

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
//const char* ssid = "Marko Martin";
//const char* password = "Poposki43";
//char ssid[] = "Marko Martin";
//char pass[] = "Poposki43";

String phoneNumber = "+38972530111";
String apiKey = "1012064";

void sendMessage(String message){

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}


BlynkTimer timer;

BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();

  // Update state
  Blynk.virtualWrite(V1, value);
}

BLYNK_CONNECTED()
{
  // Change Web Link Button message to "Congratulations!"
  Blynk.setProperty(V3, "offImageUrl", "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations.png");
  Blynk.setProperty(V3, "onImageUrl",  "https://static-image.nyc3.cdn.digitaloceanspaces.com/general/fte/congratulations_pressed.png");
  Blynk.setProperty(V3, "url", "https://docs.blynk.io/en/getting-started/what-do-i-need-to-blynk/how-quickstart-device-was-made");
}

String plantName;
String brightnessMin;
String brightnessMax;
String temperatureMin;
String temperatureMax;
String waterMin;
String waterMax;
String humidityMin;
String humidityMax;
double waterAvg = 0.0;
double lightAvg = 0.0;
double mesNum = 0.0;
double humidityAvg = 0.0;
double tempAvg = 0.0;

const char* PARAM_INPUT_1 = "name";
const char* PARAM_INPUT_2 = "brightnessMin";
const char* PARAM_INPUT_3 = "brightnessMax";
const char* PARAM_INPUT_4 = "temperatureMin";
const char* PARAM_INPUT_5 = "temperatureMax";
const char* PARAM_INPUT_6 = "waterMin";
const char* PARAM_INPUT_7 = "waterMax";
const char* PARAM_INPUT_8 = "humidityMin";
const char* PARAM_INPUT_9 = "humidityMax";
const char* PARAM_INPUT_10 = "phoneNumber";
const char* PARAM_INPUT_11 = "apiKey";

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-GLhlTQ8iRABdZLl6O3oVMWSktQOp6b7In1Zl3/Jr59b6EGGoI1aFkw7cmDA6j6gD" crossorigin="anonymous">
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/js/bootstrap.bundle.min.js" integrity="sha384-w76AqPfDkMBDXo30jS1Sgez6pr3x5MlQ1ZAGC+nuZB+EYdgRZgiwxhTBTkF7CXvN" crossorigin="anonymous"></script>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get" class="m-3">
  <div class="row g-3 m-3 align-items-center">
      <div><p>"If you want to recieve messages about your plant, please send WhatsApp message: 
      'I allow callmebot to send me messages' to +34 644 91 07 79 and type the code below."</p></div>
      <div class="col-auto">
        <label for="phoneNumber" class="form-label">Your phone number (ex. +38972530111)</label>
        <input type="text" class="form-control" name="phoneNumber" value="+38972530111">
      </div>
      <div class="col-auto">
        <label for="apiKey" class="form-label">Code you recieved</label>
        <input type="text" class="form-control" name="apiKey" value="1012064">
      </div>
    </div>
    <div class="row g-3 m-3 align-items-center">
      <div class="col-auto">
        <label for="name" class="form-label">Name of the plant</label>
        <input type="text" class="form-control" name="name">
      </div>
    </div>
    <div class="row g-3 m-3 align-items-center">
      <label for="brightnessMin" class="form-label">Brightness from</label>
      <div class="slidecontainer col-auto">
        <input type="range" name="brightnessMin" min="1" max="100" value="10" class="slider" id="brightnessMin">
      </div>
      <div class="slidecontainer col-auto">
       to <input type="range" name="brightnessMax" min="1" max="100" value="80" class="slider" id="brightnessMax">
      </div>
    </div>
    <div class="row g-3 m-3 align-items-center">
      <label for="temperatureMin" class="form-label">Temperature from</label>
      <div class="slidecontainer col-auto">
        <input type="range" name="temperatureMin" min="1" max="50" value="5" class="slider" id="temperatureMin">
      </div>
      <div class="slidecontainer col-auto">
        to <input type="range" name="temperatureMax" min="1" max="50" value="40" class="slider" id="temperatureMax">
      </div>
    </div>
    <div class="row g-3 m-3 align-items-center">
      <label for="waterMin" class="form-label">Water from</label>
      <div class="slidecontainer col-auto">
        <input type="range" name="waterMin" min="1" max="100" value="10" class="slider" id="waterMin">
      </div>
      <div class="slidecontainer col-auto">
        to <input type="range" name="waterMax" min="1" max="100" value="80" class="slider" id="waterMax">
      </div>
    </div>
    <div class="row g-3 m-3 align-items-center">
      <label for="humidityMin" class="form-label">Humidity from</label>
      <div class="slidecontainer col-auto">
        <input type="range" name="humidityMin" min="1" max="100" value="10" class="slider" id="humidityMin">
      </div>
      <div class="slidecontainer col-auto">
        to <input type="range" name="humidityMax" min="1" max="100" value="80" class="slider" id="humidityMax">
      </div>
    </div>
    
    <input type="submit" class="btn btn-primary m-3" value="Submit">
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

int getLocalTimeHour(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return -1;
  }
  
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  String stringTimeHour = String(timeHour);
  return stringTimeHour.toInt();
}

void myTimerEvent()
{
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  int moisture = analogRead(moisture_pin);
  moisture = map(moisture, OpenAirMoistureReading, WaterReading, 0, 100);
  int light = analogRead(light_pin);
  light = map(light, darkestMes, lightestMes, 0, 100);
  bool pump = false;

  mesNum = mesNum + 1.0;
  waterAvg = (waterAvg + moisture);
  lightAvg = (lightAvg + light);
  humidityAvg = (humidityAvg + humidity);
  tempAvg = (tempAvg + temperature);
  
  if(mesNum >= avgMesNum) {
    waterAvg = waterAvg / mesNum;
    lightAvg = lightAvg / mesNum;
    humidityAvg = humidityAvg / mesNum;
    tempAvg = tempAvg / mesNum;
    
    if(waterAvg < waterMin.toInt()){
      pump = true;
      digitalWrite(pump_pin, HIGH);
      delay(5000);
      pump = false;
      digitalWrite(pump_pin, LOW);
    }
    if(waterAvg > waterMax.toInt()){
      //send notif for overwatering
      //sendMessage("Too much water in your plant!");
      Serial.println("Too much water!");
    }
    
    if(getLocalTimeHour() <= 18 && getLocalTimeHour() >= 6){
      if(lightAvg < brightnessMin.toInt()){
        //sendMessage("Put your plant in a brighter place!");
        Serial.println("Put your plant in a brighter place!");
    }
      if(lightAvg > brightnessMax.toInt()){
        //send notif for light
        //sendMessage("Too much light for your plant!");
        Serial.println("Too much light!");
      }
    }

    if(humidityAvg < humidityMin.toInt()){
      //sendMessage("Too dry air for your plant!");
      Serial.println("Too dry air for your plant!");
    }
    if(humidityAvg > humidityMax.toInt()){
      //sendMessage("Too humid for your plant!");
      Serial.println("Too humid for your plant!");
    }
    
    if(tempAvg < temperatureMin.toInt()){
      //sendMessage("Too hot for your plant!");
      Serial.println("Too cold for your plant!");
    }
    if(tempAvg > temperatureMax.toInt()){
      //sendMessage("Too hot for your plant!");
      Serial.println("Too hot for your plant!");
    }

    tempAvg = 0;
    humidityAvg = 0;
    waterAvg = 0;
    lightAvg = 0;
    mesNum = 0;
  }

  Serial.println("Set name: " + plantName);

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(moisture, 1);
  Serial.print("\t\t");
  Serial.print(light, 1);
  Serial.print("\t\t");
  Serial.print(pump, 1);
  Serial.println("");
  Serial.println("Status\tHumidity (%)\tTemperature (C)\tMoisture \tLight \tPump");
  Blynk.virtualWrite(V2, millis() / 1000);
  Blynk.virtualWrite(V4, moisture);
  Blynk.virtualWrite(V5, temperature);
  Blynk.virtualWrite(V6, light);
  Blynk.virtualWrite(V7, humidity);
}

char* string_to_char(String stringInput) {
  int str_len = stringInput.length() + 1;        // convert string to chat
  char char_arr[str_len];  
  stringInput.toCharArray(char_arr, str_len); 
  return char_arr;
}

void setup() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("SETUP!!");
    
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    if(digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("BUTTON PRESSED!!!");
      wm.resetSettings();
    }

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    //wm.setSTAStaticIPConfig(IPAddress(192,168,100,128), IPAddress(192,168,100,1), IPAddress(255,255,0,0));
    res = wm.autoConnect("PlantAssistant","password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected to :)");
        Serial.println(WiFi.SSID());
       //Serial.println(WiFi.psk());
    
    
  Blynk.begin(BLYNK_AUTH_TOKEN, string_to_char(WiFi.SSID()), string_to_char(WiFi.psk()));
  // You can also specify server:
  Blynk.begin(BLYNK_AUTH_TOKEN, string_to_char(WiFi.SSID()), string_to_char(WiFi.psk()), "blynk.cloud", 80);
  //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, IPAddress(192,168,1,100), 8080);

  // Setup a function to be called every second
  timer.setInterval(timeForMes, myTimerEvent);

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println(PARAM_INPUT_2);
  Serial.println(PARAM_INPUT_3);

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        
    plantName = request->getParam(PARAM_INPUT_1)->value();
    brightnessMin = request->getParam(PARAM_INPUT_2)->value();
    brightnessMax = request->getParam(PARAM_INPUT_3)->value();
    temperatureMin = request->getParam(PARAM_INPUT_4)->value();
    temperatureMax = request->getParam(PARAM_INPUT_5)->value();
    waterMin = request->getParam(PARAM_INPUT_6)->value();
    waterMax = request->getParam(PARAM_INPUT_7)->value();
    humidityMin = request->getParam(PARAM_INPUT_8)->value();
    humidityMax = request->getParam(PARAM_INPUT_9)->value();
    phoneNumber = request->getParam(PARAM_INPUT_10)->value();
    apiKey = request->getParam(PARAM_INPUT_11)->value();
    
    request->send(200, "text/html", "Device set for the plant: " 
                                     + plantName + " with values: <br> " 
                                     + "Brightness: " + brightnessMin + " - "
                                     + brightnessMax + " <br> "
                                     + "Temperature: " + temperatureMin + " - "
                                     + temperatureMax + " <br> "
                                     + "Water level: " + waterMin + " - "
                                     + waterMax + " <br> "
                                     + "Humidity: " + humidityMin + " - "
                                     + humidityMax + " <br> " +
                                     "Phone number: " + phoneNumber +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();

  
  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (C)\tMoisture \tLight");
    }
  dht.setup(DHTpin, DHTesp::DHT11); //for DHT11 Connect DHT sensor to GPIO 17
  pinMode(moisture_pin, INPUT);
  pinMode(light_pin, INPUT);
  pinMode(pump_pin, OUTPUT);
}

void loop() {
  Blynk.run();
  timer.run();
}
