#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <ArduinoJson.h>

 // globals
const char *apssid = "thisismyspace";
const char *appassword = "";

bool loadConfig();
bool saveConfig();
bool changeApplicationMode = false;

AsyncWebServer server(80);

/*
 *  Application Modes are
 *  1 = Setup of WLAN Station Mode 
 *  2 = Setup of MQTT and tims Server & Account
 *  3 = Running
 * 
 */

struct Config {
  int ApplicationMode;
  char Host[100];
  char DeviceUuid[100];
  char Ssid[100];
  char Pass[100];
  char Broker[100];
  char User[100];
  char Password[100];
  int Port;
  boolean Debug;
  };

struct Config Properties;

String HtmlMessage;


/*
* Connect your controller to WiFi
*/
bool connectToWiFi() {
  
  //Connect to WiFi Network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to WiFi");
  Serial.println("...");
  WiFi.begin(Properties.Ssid, Properties.Pass);
  int retries = 0;
     
  while ((WiFi.status() != WL_CONNECTED) && (retries < 30)) {
     retries++;
     delay(500);
     Serial.print(".");
  }
    
  if (retries > 29) {
    Serial.println(F("WiFi connection FAILED"));
    return false;
    
  } else if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi connected!"));
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
    
  } else {
    Serial.println(F("WiFi not connected"));
    return false;
    
  }
}


// handlers

String processorNotFound(const String& var)
{
  if(var == "URI") {
    return "uri...";
    
  } else if (var == "METHOD") {
    return "methode post oder get";
  
  } else if (var == "ARGUMENTS") {
    return "arguments/params";
  }
  return String();
}

void handleNotFound(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/notfound.html", String(), false, processorNotFound);
}

String processorConfig(const String& var)
{
  if(var == "SSID") {
    return Properties.Ssid;
  } else if (var == "KEY") {
    return Properties.Pass;
  } else if (var == "HOSTNAME") {
    return Properties.Host;
  } else if (var == "MESSAGE") {
    return HtmlMessage;
  } else if (var == "BROKER") {
    return Properties.Broker;
  } else if (var == "PORT") {
    return String(Properties.Port);
  } else if (var == "USER") {
    return Properties.User;
  } else if (var == "PASSWORD") {
    return Properties.Password;
  }
  return String();
}

void handleRoot(AsyncWebServerRequest *request) {
  // setup assistent and config
  if (Properties.ApplicationMode == 1) {
    HtmlMessage = "";
    request->send(LittleFS, "/wlan.html", String(), false, processorConfig);
    
  } else if (Properties.ApplicationMode == 2) {
    HtmlMessage = WiFi.softAPIP().toString();
    request->send(LittleFS, "/account.html", String(), false, processorConfig);
    
  } else {
    HtmlMessage = WiFi.softAPIP().toString();
    request->send(LittleFS, "/account.html", String(), false, processorConfig);
    
  }
  
}


void handleSubmit(AsyncWebServerRequest *request) {

  String message;

  if (Properties.ApplicationMode == 1) {
    // wlan configuration
     
    if (request->hasParam("ssid", true)) {
      message = request->getParam("ssid", true)->value();
      strcpy(Properties.Ssid, message.c_str());
    } 

    if (request->hasParam("key", true)) {
      message = request->getParam("key", true)->value();
      strcpy(Properties.Pass, message.c_str());
    } 

    if (request->hasParam("host", true)) {
      message = request->getParam("host", true)->value();
      strcpy(Properties.Host, message.c_str());
    } 

    // Change to Account Registration/Setup when connection successful
    //if (connectToWiFi()) {
    Properties.ApplicationMode = 2;
    changeApplicationMode = true;
    //}
    
  } else if (Properties.ApplicationMode == 2) {

    
    // Account Config
    
    if (request->hasParam("broker", true)) {
      message = request->getParam("broker", true)->value();
      strcpy(Properties.Broker, message.c_str());
    } 

    if (request->hasParam("port", true)) {
      message = request->getParam("port", true)->value();
      Properties.Port = message.toInt();
    } 

    if (request->hasParam("user", true)) {
      message = request->getParam("user", true)->value();
      strcpy(Properties.User, message.c_str());
    } 

    if (request->hasParam("password", true)) {
      message = request->getParam("password", true)->value();
      strcpy(Properties.Password, message.c_str());
    }  
    
  }

  // Save Config 
  if (!saveConfig()) {
    Serial.println("Failed to save config");
  } else {
    Serial.println("config file was saved successfully");

  }  

  //redirect to index 
  showConfig();
  //printConfig();  
  request->redirect("/");
}


// Functions to handle Config File
bool loadConfig() {

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

  StaticJsonDocument<256> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    configFile.close();
    return false;
  }

  //Speichere Pointer to Struct Var
  Properties.Debug = doc["debug"];  
  Properties.ApplicationMode = doc["applicationmode"];
  strcpy(Properties.Host, doc["host"]);
  strcpy(Properties.DeviceUuid, doc["deviceuuid"]);
  strcpy(Properties.Ssid, doc["ssid"]);
  strcpy(Properties.Pass, doc["pass"]);
  strcpy(Properties.Broker, doc["broker"]);
  Properties.Port = doc["port"];  
  strcpy(Properties.User, doc["user"]);
  strcpy(Properties.Password , doc["password"]);

  //Close and return
  configFile.close();
  return true;
}

bool saveConfig() {

  File configFile = LittleFS.open("/properties.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }  

  // Serialize Config struct
  StaticJsonDocument<256> doc;  
 
  doc["debug"] = Properties.Debug;
  doc["applicationmode"] = Properties.ApplicationMode;
  doc["host"] = Properties.Host;
  doc["deviceuuid"] = Properties.DeviceUuid;
  doc["ssid"] = Properties.Ssid;
  doc["pass"] = Properties.Pass;
  doc["broker"] = Properties.Broker;
  doc["port"] = Properties.Port;
  doc["user"] = Properties.User;
  doc["password"] = Properties.Password;
  
  auto error = serializeJson(doc, configFile);
  if (error) {
    Serial.println("Failed to parse to config file");
    configFile.close();
    return false;
  }
 
  // close Config File
  configFile.close();

  return true;
}

void showConfig() {

   // Show Debug Information and Configuration
  if (Properties.Debug) {
    Serial.println("CONFIG  ---------------------------");
    printf("Application Mode: %i\n", Properties.ApplicationMode);
    printf("Device Name : %s\n", Properties.Host);
    printf("Device UUID : %s\n", Properties.DeviceUuid);     
    printf("SSID string : %s\n", Properties.Ssid);
    printf("Key string : %s\n", Properties.Pass);     
    printf("Broker string : %s\n", Properties.Broker);
    printf("Port: %i\n", Properties.Port);
    printf("User : %s\n", Properties.User);
    printf("Password : %s\n", Properties.Password);
    printf("Debug-Mode: %s\n", "true");
    Serial.println("CONFIG  END -----------------------");    
  } 
}


void printConfig() {
  // print config file to console
  File configFile = LittleFS.open("/properties.json", "r");
  if (configFile) {
   {
      // read from the file until there's nothing else in it:
      while (configFile.available())
      {
         Serial.write(configFile.read());
      }                               
      // close the file:             
      configFile.close();
   }
  }
}


void setup() {
  
   // Startup Sequence
  Serial.begin(115200);
  delay(500);

  Serial.println("");
  Serial.println("This Is My Space Device Startup Sequence");
  Serial.println("----------------------------------------");

  // Init Little FS Filesystem
  Serial.print(F("Inizializing FS..."));
  if (LittleFS.begin()){
      Serial.println(F("done."));

      //printConfig();

      if (!loadConfig()) {
        Serial.println("Failed to load config");
        
      } else {
        showConfig();   
      }
      
  }else{
      Serial.println(F("fail."));
  }

  // Determine ApplicationMode
  if (strlen(Properties.Host) == 0) {

      String DeviceName = "tims_";
      DeviceName += String(ESP.getChipId());
    
      strcpy(Properties.Host, DeviceName.c_str()); 
      printf("New Device Name : %s\n", Properties.Host);

      if (!saveConfig()) {
        Serial.println("Failed to save config");
      } else {
        Serial.println("config file was saved successfully");
      } 
      
  }

  // Init Soft Access Point for Device configuration
  WiFi.softAP(apssid, appassword);
  WiFi.hostname(Properties.Host);
 
  Serial.println();
  Serial.print("Server IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Server MAC address: ");
  Serial.println(WiFi.softAPmacAddress());

  // Init Wlan 
  if (Properties.ApplicationMode > 1) { 

    // Connect to wifi network  
    if (!connectToWiFi()) {
      Properties.ApplicationMode = 1;

      // Save Config 
      if (!saveConfig()) {
        Serial.println("Failed to save config");
      } else {
        Serial.println("config file was saved successfully");
      } 
      
    }
    
  }
  
  // Init Webserver
  server.serveStatic("/Roboto.css", LittleFS, "/Roboto.css");
  server.serveStatic("/tims.css", LittleFS, "/tims.css");
  
  server.on("/", handleRoot);
  server.on("/submit",HTTP_POST, handleSubmit);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Server listening");
  
  
}
 
void loop() {

  //
  if (changeApplicationMode) {

    if (Properties.ApplicationMode == 2) {
      
      // Connect to wifi network  
      if (!connectToWiFi()) {
        Properties.ApplicationMode = 1;

        // Save Config 
        if (!saveConfig()) {
          Serial.println("Failed to save config");
        } else {
          Serial.println("config file was saved successfully");
        } 
      }

      changeApplicationMode = false;
    }
    
  }
  
}
 
