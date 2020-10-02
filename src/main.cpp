#include <WiFiManager.h>
WiFiManager wm;
#define WEBSERVER_H
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_SPIDevice.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char *ssid = "ESP32AP";
const char *password = "devkit1234";
//"EFC5F27D674F";

const char *PARAM_TEMPERATURE = "temperature";
const char *PARAM_CADENCE_FREQ = "cadenceFreq";
const char *PARAM_CADENCE_DUREE = "cadenceDuree";
const char *PARAM_NOM = "nom";

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" charset="UTF-8" />
  <script>
  function submitMessage() {
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script>
</head>
<body style="font-family:Georgia, Times, serif;">
  <header style="height:350px; background-image: url(https://cutewallpaper.org/21/hd-aquarium-wallpapers/Aquarium-Desktop-Wallpaper-62-images-.jpg);">
    <div style="float:right; margin:25px 10px 0 0;">
      <a style="border-right: 2px solid white; padding-right:10px; font-weight: bold; color:white; cursor:pointer;">EN/FR</a>&nbsp;&nbsp;<a style="font-weight: bold; color:white; cursor:pointer;">Documentation</a>
    </div>
  </header>
  <div style="display:flex; margin-top:45px; color:#3e4444;">
    <div style="text-align:center;width:420px;">
      <h3 style="padding: 0px 50px 0 50px;">Changer la cadence de la pompe</h3>
      <div>
        <form action="/get" target="hidden-form">
          <h4>Cadence actuelle</h4>
          <span>À chaque : %cadenceFreq%h (%cadenceDuree% mins fixe)</span>
          <h4>Cadence désirée</h4>
          <select style="width:100px;text-align-last:center;" name="cadenceFreq" onchange="this.form.submit();submitMessage();">
            <option selected="selected" disabled>%cadenceFreq%</option>
            <option value="0.5">30m</option>
            <option value="1">1h</option>
            <option value="2">2h</option>
            <option value="4">4h</option>
            <option value="8">8h</option>
          </select>
          <h4>Durée désirée</h4>
          <input style="text-align:center;width:40px;" name="cadenceDuree" type="number"value="%cadenceDuree%"></input>
          <button type="submit" value="Submit" onclick="submitMessage();">Modifier</button>
        </form>
      </div>
    </div>
    <div style="text-align:center;width:420px;">
      <h3>Changer la température</h3>
      <div>
        <form action="/get" target="hidden-form">
          <h4>Température actuelle</h4>
          <span> 0°</span>
          <h4>Température désirée</h4>
          <select style="width:100px;text-align-last:center;" name="temperature" onchange="this.form.submit();submitMessage();">
            <option selected disabled>%temperature%°</option>
            <option value="23">23°</option>
            <option value="24">24°</option>
            <option value="25">25°</option>
            <option value="26">26°</option>
            <option value="27">27°</option>
            <option value="28">28°</option>
            <option value="29">29°</option>
            <option value="30">30°</option>
            <option value="31">31°</option>
            <option value="32">32°</option>
            <option value="33">33°</option>
            <option value="34">34°</option>
          </select>
          <p>Le pad chauffant commencera à chauffer WHEN (TempératureActuelle < (TempératureDésirée - 1.5°))</p>
        </form>
      </div>
    </div>
    <div style="text-align:center;width:420px;">
      <h3>Changer le nom de l'aquarium</h3>
      <div>
        <form action="/get" target="hidden-form">
          <h4>Nom Actuel</h4>
          <input style="text-align:center;" name="nom" type="text"value="%nom%"></input>
          <button type="submit" value="Submit" onclick="submitMessage();">Modifier</button>
        </form>
      </div>
    </div>
  </div>
  <footer style="text-align: center;border-top: 2px solid black; border-bottom: 2px solid black; padding: 35px 0 35px 0; margin-top:80px; font-style:italic; color:#3e4444;">
    <span>Réalisé par Mathieu Lepage</span>
  </footer>
  <iframe style="display:none" name="hidden-form"></iframe>
</body>
</html>
)rawliteral";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- write failed");
  }
}

String processor(const String &var)
{
  if (var == "temperature")
  {
    return readFile(SPIFFS, "/temperature.txt");
  }
  else if (var == "nom")
  {
    return readFile(SPIFFS, "/nom.txt");
  }
  else if (var == "cadenceFreq")
  {
    return readFile(SPIFFS, "/cadenceFreq.txt");
  }
  else if (var == "cadenceDuree")
  {
    return readFile(SPIFFS, "/cadenceDuree.txt");
  }
  return String();
}

void setup()
{
  //----------------------------------------------------Serial
  Serial.begin(9600);
  Serial.println("\n");

//----------------------------------------------------SPIFFS
#ifdef ESP32
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
#else
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
#endif

//------------------------------------------------------OLED

if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);}

  //----------------------------------------------------GPIO

  //----------------------------------------------------WIFI
  if (!wm.autoConnect(ssid, password))
  {
    Serial.println("Erreur de connexion.");
  }
  else
  {
    Serial.println("\n");
    Serial.println("Connexion etablie!");
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.localIP());
  }

  //----------------------------------------------------SERVER

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_NOM)) {
      inputMessage = request->getParam(PARAM_NOM)->value();
      writeFile(SPIFFS, "/nom.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_TEMPERATURE)) {
      inputMessage = request->getParam(PARAM_TEMPERATURE)->value();
      writeFile(SPIFFS, "/temperature.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_CADENCE_FREQ) ||request->hasParam(PARAM_CADENCE_DUREE)) {
    if(request->hasParam(PARAM_CADENCE_FREQ)){
    inputMessage = request->getParam(PARAM_CADENCE_FREQ)->value();
    writeFile(SPIFFS, "/cadenceFreq.txt", inputMessage.c_str());
    }
    if(request->hasParam(PARAM_CADENCE_DUREE)){
    inputMessage = request->getParam(PARAM_CADENCE_DUREE)->value();
    writeFile(SPIFFS, "/cadenceDuree.txt", inputMessage.c_str());
    }
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });

  server.onNotFound(notFound);
  Serial.println("Serveur actif!");
  server.begin();
}

void loop()
{
String yourNameString = readFile(SPIFFS, "/nom.txt");
Serial.print("*** Your inputNom: ");
Serial.println(yourNameString);

int yourTemperatureInt = readFile(SPIFFS, "/temperature.txt").toInt();
Serial.print("*** Your TemperatureInt: ");
Serial.println(yourTemperatureInt);

int yourCadenceFreqInt = readFile(SPIFFS, "/cadenceFreq.txt").toInt();
Serial.print("*** Your CadenceFreqInt: ");
Serial.println(yourCadenceFreqInt);

int yourCadenceDureeInt = readFile(SPIFFS, "/cadenceDuree.txt").toInt();
Serial.print("*** Your CadenceDureeInt: ");
Serial.println(yourCadenceDureeInt);
delay(5000);

// delay(2000);
// display.clearDisplay();
// display.setTextSize(1);
// display.setTextColor(WHITE);
// display.setCursor(0, 5);
// // Display static text
// display.println("Name : " + readFile(SPIFFS, "/nom.txt"));
// display.setCursor(0, 20);
// // Display static text
// display.println("Temp. Desire : " + readFile(SPIFFS, "/temperature.txt"));
// display.setCursor(0, 35);
// // Display static text
// display.println("IP : " + WiFi.localIP().toString());
// display.display(); 

// if(touchRead(T0) < 50)
//   {
//     Serial.println("Suppression des reglages et redemarrage...");
//     wm.resetSettings();
//     ESP.restart();
//   }
}