#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <DHT.h>
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h" // Include the UI lib
#include "images.h" // Include custom images
#include <ArduinoOTA.h>

#define wifi_ssid "WiFi SSID"
#define wifi_password "WiFi Password"
#define mqtt_server "MQTT IP Address"

#define humidity_topic "home/sensor/humidity"
#define temperature_topic "home/sensor/temperature"
#define heatindex_topic "home/sensor/heatindex"
#define motion_topic "home/sensor/motion"
#define ota_topic "home/sensor/ota"

#define DHTTYPE DHT22
#define DHTPIN  D6

SSD1306  display(0x3c, D2, D1);
OLEDDisplayUi ui     ( &display );

int counter = 0;
int previousReading = LOW;
int PIRPIN = D5;

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float hic=0.0;
float diff = 0.1;
boolean motion=false;

boolean otaflag=false;

WiFiClient espClient;
PubSubClient client(espClient);
//DHT dht(D1, DHTTYPE, 11); // 11 works fine for ESP8266
DHT dht(DHTPIN, DHTTYPE); // 11 works fine for ESP8266

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis())); yield();
}

void drawFrame0(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String ipAddress = IPAddress2String(WiFi.localIP());
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "Sensor 2"); yield();
  display->drawString(64 + x, 17 + y, ipAddress); yield();
  if(motion){
    display->drawString(64 + x, 34 + y, "Motion : Y"); yield();
  }else{
    display->drawString(64 + x, 34 + y, "Motion : N"); yield();
  }
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "Sensor 2"); yield();
  display->drawString(64 + x, 17 + y, "Temp: " + String(temp) + " C"); yield();
  display->drawString(64 + x, 34 + y, "Hum : " + String(hum) + " %"); yield();
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame0, drawFrame1 };

// how many frames are there?
int frameCount = 2;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;
//  char* cpmOTATopic="home/sensor/ota";

  if(strcmp(topic,"home/sensor/ota")==0 and message == "1") {
    otaflag=true;
  } else {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);
  }
}

void setup() {
  //Wire.begin(); 
  Serial.begin(115200);
  pinMode(DHTPIN, INPUT_PULLUP);
  pinMode(PIRPIN, INPUT);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 
  
  ArduinoOTA.setHostname("Sensor 2");
  
  ArduinoOTA.onStart([]() {
    digitalWrite(BUILTIN_LED, LOW);
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    digitalWrite(BUILTIN_LED, HIGH);
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

  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  //ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();
  ////////// End of Display //////////////
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1500);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("sensor")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("home/sensor/status","Sensor 2 Connected");
      client.subscribe("home/sensor/status");
      client.subscribe("home/sensor/ota");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

void loop() {
    int remainingTimeBudget = ui.update(); yield();
    
    if (remainingTimeBudget > 0) {
      // You can do some work here
      // Don't do stuff if you are below your
      // time budget.
      // delay(remainingTimeBudget);
      if (otaflag) {
        Serial.println("OTA MODE");
        ArduinoOTA.handle();
        delay(200);
      }else{
        if (!client.connected()) {
          reconnect();
        }
        client.loop();
        int reading = digitalRead(PIRPIN); yield();
        long now = millis();
      //  Serial.println(now - lastMsg);
        if (now - lastMsg > 10000) {
          lastMsg = now;
      
          float newTemp = dht.readTemperature(); yield();
          float newHum = dht.readHumidity(); yield();
          float newHic = dht.computeHeatIndex(newTemp, newHum, false); yield();
      
          if (checkBound(newTemp, temp, diff)) {
            temp = newTemp;
            //Serial.print("New temperature:");
            //Serial.println(String(temp).c_str());
            client.publish(temperature_topic, String(temp).c_str(), true); yield();
          }
          Serial.print(String(newTemp).c_str());
          Serial.print(" ");
          if (checkBound(newHum, hum, diff)) {
            hum = newHum;
            //Serial.print("New humidity:");
            //Serial.println(String(hum).c_str());
            client.publish(humidity_topic, String(hum).c_str(), true); yield();
          }
          Serial.print(String(newHum).c_str());
          Serial.print(" ");
          if (checkBound(newHic, hic, diff)) {
            hic = newHic;
            //Serial.print("New Heat Index:");
            //Serial.println(String(hic).c_str());
            client.publish(heatindex_topic, String(hic).c_str(), true); yield();
          }
          Serial.println(String(newHic).c_str());
        }
      
        Serial.println(reading);
        if (previousReading == LOW && reading == HIGH) {
          counter++;
          client.publish(motion_topic, "ON");  yield();
          client.publish(motion_topic, "1");   yield();
          Serial.print("Triggered ");
          Serial.print(counter);
          Serial.println("x Times! ");
          motion=true;
        } else if (previousReading == HIGH && reading == LOW){
          client.publish(motion_topic, "OFF");  yield();
          client.publish(motion_topic, "0");  yield();
          motion=false;
        }
        previousReading = reading;
    }
    
    //delay(1000);
  }
}

String IPAddress2String(IPAddress address)
{
 return "IP:" + 
        String(address[0]) + "." + 
        String(address[1]) + "." + 
        String(address[2]) + "." + 
        String(address[3]);
}
