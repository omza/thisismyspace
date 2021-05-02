// Example: storing JSON configuration file in flash file system
//
// Uses ArduinoJson library by Benoit Blanchon.
// https://github.com/bblanchon/ArduinoJson
//
// Created Aug 10, 2015 by Ivan Grokhotkov.
//
// This example code is in the public domain.

#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>


// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        5 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 60 // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Config Vars
struct Config {
  char Host[100];
  char Ssid[100];
  char Pass[100];
  char Broker[100];
  int Port;
  boolean Debug;
  };

struct Config Properties;

// Message Buffer and Event objects
struct ReceiveData {
  const char *Devices = "LD";
  boolean Raised;
  unsigned long ReceivedAt;
  char Payload[50];  
};

struct ReceiveData Rx;

struct Lightbulb {
  const char *Events = "OF";
  char Event[1];
  unsigned long From;
  unsigned long To;
  uint32_t Colour;
  int Brightness;   
};

// Event objects instantiate
// last come last serve
struct Lightbulb Light;


// routines

bool loadConfig() {

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return false;
  }
  
  File configFile = LittleFS.open("/properties.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<200> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  //Speichere Pointer to Struct Var
  strcpy(Properties.Host, doc["host"]);
  strcpy(Properties.Ssid, doc["ssid"]);
  strcpy(Properties.Pass, doc["pass"]);
  strcpy(Properties.Broker, doc["broker"]);
  Properties.Port = doc["port"];
  Properties.Debug = doc["debug"];
  return true;
}

WiFiClient espClient;
PubSubClient client(espClient);

long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect(Properties.Host)) {
    // Once connected, publish an announcement...
    //client.publish("outTopic","hello world");
    // ... and resubscribe
    //client.subscribe("inTopic");
    client.subscribe(Properties.Host);   
  }
  return client.connected();
}

void setup() {
  // put your setup code here, to run once:

  // BusyLight during Setup
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)  
  pixels.clear(); // Set all pixel colors to 'off'
  uint32_t magenta = pixels.Color(255, 0, 255);
  pixels.setBrightness(10);
  pixels.fill(magenta, 0, NUMPIXELS);
  pixels.show();   // Send the updated pixel colors to the hardware.
  
  // Serial monitor
  Serial.begin(115200);
  delay(1000);

  // Startup Sequence
  Serial.println("");
  Serial.println("This Is My Space Device Startup Sequence");
  Serial.println("----------------------------------------");

  if (!loadConfig()) {
    Serial.println("Failed to load config");
  } else {
     // Show Debug Information and Configuration
    if (Properties.Debug) {
      Serial.println("DEBUG  ---------------------------");
      printf("Host string : %s\n", Properties.Host);     
      printf("SSID string : %s\n", Properties.Ssid);
      printf("Key string : %s\n", Properties.Pass);     
      printf("Broker string : %s\n", Properties.Broker);
      printf("Port: %i\n", Properties.Port);
      printf("Debug-Mode: %s\n", "true");
      Serial.print("MAC Address: ");
      Serial.println(WiFi.macAddress());
      Serial.println("DEBUG  END -----------------------");    
    }    
  }

  //WiFi.mode(WIFI_STA);
  WiFi.begin(Properties.Ssid, Properties.Pass);
  Serial.print("Connecting to WiFi: ");
  Serial.println(Properties.Ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to the WiFi network with IP: ");
  Serial.println(WiFi.localIP());  

  client.setServer(Properties.Broker, Properties.Port);
  client.setCallback(callback);
  Serial.print("Connecting to MQTT: ");  
  Serial.print(Properties.Broker);
  Serial.print(":");
  Serial.println(Properties.Port); 
 
  while (!client.connected()) {
       
    if (client.connect(Properties.Host)) {
 
      Serial.print(".Connected to the MQTT Broker with Client ID: ");
      Serial.println(Properties.Host);   

    } else {
 
      Serial.print(".");
      Serial.print(client.state());
      delay(500);
 
    }
  }

  // Subscribe to Client/Topic
  // Prepare Rx and Tx
  Rx.Raised = false;
  client.subscribe(Properties.Host);

  // Setup finished
  pixels.clear(); // Set all pixel colors to 'off'  
  pixels.show();   // Send the updated pixel colors to the hardware.
    
}

void callback(char* topic, byte* payload, unsigned int length) {

  // Test length (minimum 4 char) and possible devices and fill Event 
  // further parsing takes part in loop top keep the callback as short as possible
  if (length >= 4) {
    if (strchr(Rx.Devices,(char)payload[0]) != NULL) {
      int i;
      for (i = 0; i < length; i++) {
        Rx.Payload[i] = (char)payload[i];
      }
      Rx.Payload[i] = 0;
      Rx.Raised = true;
      Rx.ReceivedAt = millis();
          
    }    
  }
}


void loop() {
  // put your main code here, to run repeatedly:

  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    
    // Incoming Event handler
    if (Rx.Raised) {
      
      if (Properties.Debug){
        Serial.println("----------------------------------------"); 
        Serial.print("Payload: ");
        Serial.println(Rx.Payload);
        Serial.print("Timestamp: ");
        Serial.println(Rx.ReceivedAt);        
      }

      // Parse Object 
      if (Rx.Payload[0] == 'L') {

        // test to known events
        if (strchr(Light.Events,Rx.Payload[1]) != NULL) {
          Light.Event[1] = Rx.Payload[1];
          Light.From = millis();
          Light.To = Light.From + 5000;
          Light.Colour = pixels.Color(0, 255, 0);
          Light.Brightness = 10;
          if (Properties.Debug){Serial.println("Lightbulb Event triggered");}
        }        
      }
      Rx.Raised = false;
    }

    // Lightbulb Device Events
    if (Light.Event[1] != 'X') {

      if (Light.Event[1] == 'O') {
        // Lights on
        unsigned long checktime = millis();
        if ((Light.From <= checktime) && (Light.To >= checktime)) {
          //Do Event                   
          pixels.setBrightness(Light.Brightness);
          pixels.fill(Light.Colour, 0, NUMPIXELS);
          pixels.show();   // Send the updated pixel colors to the hardware.          
        
        } else {
          // End of Event
          Light.Event[1] = 'X';
          pixels.clear(); // Set all pixel colors to 'off'  
          pixels.show();   // Send the updated pixel colors to the hardware. 
        }
      } else if (Light.Event[1] == 'F') {
          // Lights Off
          Light.Event[1] = 'X';          
          pixels.clear(); // Set all pixel colors to 'off'  
          pixels.show();   // Send the updated pixel colors to the hardware. 
          
      } else { 
        // Event unknown
        
        Light.Event[1] = 'X';
      }
    }
  
    // Outgoing Event handler

    //next message
    client.loop();    
    
  }
}
