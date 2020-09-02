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
  int i;
  const char *charbuffer = doc["host"]; 
  for(i=0; i<strlen(charbuffer); i++)
  {
    Properties.Host[i] =  charbuffer[i];
  }
  Properties.Host[i+1] = 0;
  
  charbuffer = doc["ssid"]; 
  for(i=0; i<strlen(charbuffer); i++)
  {
    Properties.Ssid[i] =  charbuffer[i];
  }
  Properties.Ssid[i+1] = 0;

  charbuffer = doc["pass"]; 
  for(i=0; i<strlen(charbuffer); i++)
  {
    Properties.Pass[i] =  charbuffer[i];
  }
  Properties.Pass[i+1] = 0;  
  
  charbuffer = doc["broker"]; 
  for(i=0; i<strlen(charbuffer); i++)
  {
    Properties.Broker[i] =  charbuffer[i];
  }
  Properties.Broker[i+1] = 0;  

  Properties.Port = doc["port"];
  Properties.Debug = doc["debug"];

  return true;
}

WiFiClient espClient;
PubSubClient client(espClient);

long lastReconnectAttempt = 0;
char hellomessage[] = "BusyLightAdvancedDevice online";


boolean reconnect() {
  if (client.connect(Properties.Host)) {
    // Once connected, publish an announcement...
    //client.publish("outTopic","hello world");
    // ... and resubscribe
    //client.subscribe("inTopic");
    client.subscribe(Properties.Host); 
    client.publish(Properties.Host, hellomessage);    
  }
  return client.connected();
}

void setup() {
  // put your setup code here, to run once:
  
  // Serial monitor
  Serial.begin(115200);
  delay(1000);

  // Startup Sequence
  Serial.println("");
  Serial.println("BusyLightAdvancedDevice Startup Sequence");
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

  client.setServer("test.mosquitto.org", 1883);
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
      //Serial.print(client.state());
      delay(1000);
 
    }
  }

  client.subscribe(Properties.Host); 
  client.publish(Properties.Host, hellomessage);

    
}

void callback(char* topic, byte* payload, unsigned int length) {
  
  Serial.println("----------------------------------------"); 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("----------------------------------------");
 
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

    client.loop();
  }
}
