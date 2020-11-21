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
#include <ArduinoJson.h>
#include <DallasTemperature.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Declaration d'un array JSON
String jsonresponse;
StaticJsonDocument<200> doc;

const char *ssid = "ESP32AP";
const char *password = "devkit1234"; //"EFC5F27D674F";

const int LED = 2;        //La PIN GPIO de la LED
const int oneWireBus = 4; //La pin GPIO de connexion
const int Pompe = 26;     //La pin GPIO de connexion
bool ok;                  //Flag de verif pour le range de température
float timerFreq;          //Le timer qui mesure le temps restant avant le prochain cycle
float timerDuree;         //Le timer qui mesure le temps du cycle de la pompe

// Initialisation d'une instance onewire
OneWire oneWire(oneWireBus);
// Passer la référence au sensor Dallas Temperature
DallasTemperature sensors(&oneWire);

const char *PARAM_TEMPERATURE = "temperature";
const char *PARAM_CADENCE_FREQ = "cadenceFreq";
const char *PARAM_CADENCE_DUREE = "cadenceDuree";
const char *PARAM_NOM = "nom";

AsyncWebServer server(80);

#pragma region InterfaceWEB
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
      <a id="translate" style="border-right: 2px solid white; padding-right:10px; font-weight: bold; color:white; cursor:pointer;">EN/FR</a>&nbsp;&nbsp;<a style="font-weight: bold; color:white; cursor:pointer;">Documentation</a>
      <div id="google_translate_element" style="display:none;"></div>
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
            <option value="0.5">test</option>
            <option value="30">30m</option>
            <option value="60">1h</option>
            <option value="120">2h</option>
            <option value="240">4h</option>
            <option value="480">8h</option>
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
  <script type="text/javascript">
function googleTranslateElementInit() {
  new google.translate.TranslateElement({pageLanguage: 'fr'}, 'google_translate_element');
}
</script>
<script type="text/javascript" src="//translate.google.com/translate_a/element.js?cb=googleTranslateElementInit"></script>
<script>
  document.getElementById("translate").addEventListener("click", function() {
  if(document.getElementById("google_translate_element").style.display == "none")
  {
    document.getElementById("google_translate_element").style.display = "block";
  }
  else
  {
    document.getElementById("google_translate_element").style.display = "none";
  }
});
</script>
</html>
)rawliteral";
#pragma endregion InterfaceWEB

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char *path)
{
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  //Serial.println(fileContent);
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

//Fonction qui permet d'aller toujours chercher la dernière Température Max à jour
float getTempMax()
{
  float temperature = readFile(SPIFFS, "/temperature.txt").toFloat();
  return temperature;
}

//Fonction qui permet d'aller chercher la fréquence de la pompe en minute
float obtenirPompeFreq()
{
  float freq = readFile(SPIFFS, "/cadenceFreq.txt").toFloat();
  return (freq * 60) / 3;
}

//Fonction qui permet d'aller chercher la durée de cycle de la pompe en minute
float obtenirPompeDuree()
{
  float duree = readFile(SPIFFS, "/cadenceDuree.txt").toFloat();
  return (duree * 60) / 3;
}

// Fonction qui permet d'aller chercher une température avec le senseur
float demanderTemperature()
{
  // On appelle la requète de température sensors.requestTemperatures()
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

// Fonction qui permet d'afficher les informations sur l'OLED
void afficherOLED()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);
  display.println("Name : " + readFile(SPIFFS, "/nom.txt"));
  display.setCursor(0, 20);
  display.println("Temp. Desire : " + readFile(SPIFFS, "/temperature.txt"));
  display.setCursor(0, 30);
  display.print("Temp. actuel : ");
  display.println(demanderTemperature());
  display.setCursor(0, 40);
  if (digitalRead(Pompe) == HIGH)
    display.println("Etat pompe : OFF ");
  else
    display.println("Etat pompe : ON ");
  display.setCursor(0, 55);
  display.println("IP : " + WiFi.localIP().toString());
  display.display();
}

// Vérifie si la température est dans le range permis : IF TRUE; LED: OFF ELSE; LED ON
void verifierTemperature()
{
  if (demanderTemperature() <= getTempMax() - 1)
  {
    ok = true;
  }

  if (demanderTemperature() <= getTempMax() && ok)
  {
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
    ok = false;
  }

  // while (demanderTemperature() > getTempMax())
  // {
  //   digitalWrite(LED, HIGH);
  //   delay(300);
  //   digitalWrite(LED, LOW);
  //   delay(700);
  //   afficherOLED();
  //   Serial.print("Celsius temperature: ");
  //   Serial.println(demanderTemperature());
  // }
}

// Vérifie si la pompe est en opération/fermée
void verifierEtatPompe()
{
  //IF la pompe est OFF => GO dans un waiting cycle THEN TURN ON IF le timer de la fréquence atteint ZÉRO
  if (digitalRead(Pompe) == HIGH)
  {
    timerFreq--;
    Serial.println(timerFreq);
    Serial.println(timerDuree);
    if (timerFreq == 0 && timerDuree != 0)
    {
      digitalWrite(Pompe, LOW);
      timerFreq = obtenirPompeFreq();
      timerDuree = obtenirPompeDuree();
    }
  }
  //IF la pompe est ON => Surveille la durée restante et OFF lorsqu'on atteint ZÉRO
  else if (digitalRead(Pompe) == LOW)
  {
    timerDuree--;
    Serial.println(timerFreq);
    Serial.println(timerDuree);
    if (timerDuree == 0)
    {
      digitalWrite(Pompe, HIGH);
      timerDuree = obtenirPompeDuree();
      timerFreq = obtenirPompeFreq();
    }
  }
}

// Permet de factory reset les paramètres de l'ESP32
// void resetSetting()
// {
//   Serial.println("Suppression des reglages et redemarrage...");
//   wm.resetSettings();
//   ESP.restart();
//   delay(30000);
// }

void setup()
{
  pinMode(LED, OUTPUT); //Définition de la LED en OUTPUT

  //----------------------------------------------------Serial
  Serial.begin(9600);
  Serial.println("\n");

  //----------------------------------------------------SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //-----------------------------------------------------AUTH
  //
  // writeFile(SPIFFS, "/username.txt", "admin");
  // writeFile(SPIFFS, "/password.txt", "admin");

  //------------------------------------------------------OLED

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  //-------------------------------------------------------POMPE
  digitalWrite(Pompe, HIGH); //Pompe à OFF par défaut
  pinMode(Pompe, OUTPUT);
  timerFreq = obtenirPompeFreq();   //Temps restant d'une cadence
  timerDuree = obtenirPompeDuree(); //Temps restant d'une durée d'opération

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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_NOM))
    {
      inputMessage = request->getParam(PARAM_NOM)->value();
      writeFile(SPIFFS, "/nom.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_TEMPERATURE))
    {
      inputMessage = request->getParam(PARAM_TEMPERATURE)->value();
      writeFile(SPIFFS, "/temperature.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_CADENCE_FREQ) || request->hasParam(PARAM_CADENCE_DUREE))
    {
      if (request->hasParam(PARAM_CADENCE_FREQ))
      {
        inputMessage = request->getParam(PARAM_CADENCE_FREQ)->value();
        writeFile(SPIFFS, "/cadenceFreq.txt", inputMessage.c_str());
      }
      if (request->hasParam(PARAM_CADENCE_DUREE))
      {
        inputMessage = request->getParam(PARAM_CADENCE_DUREE)->value();
        writeFile(SPIFFS, "/cadenceDuree.txt", inputMessage.c_str());
      }
    }
    else
    {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });

#pragma region API

  //GET
  // Requête qui permet d'obtenir le nom de l'aquarium
  server.on("/obtenirNomAquarium", HTTP_GET, [](AsyncWebServerRequest *request) {
    jsonresponse = "";
    doc.clear();
    if (request->authenticate(readFile(SPIFFS, "/username.txt").c_str(), readFile(SPIFFS, "/password.txt").c_str()))
    {
      doc["nomAquarium"] = readFile(SPIFFS, "/nom.txt");
      serializeJsonPretty(doc, jsonresponse);
      request->send(200, "application/json", jsonresponse);
    }
    else
    {
      doc["message"] = "Unauthorized : Username OR Password invalide";
      serializeJsonPretty(doc, jsonresponse);
      request->send(401, "application/json", jsonresponse);
    }
  });
  // Requête qui permet d'obtenir la température à ne pas dépasser
  server.on("/obtenirTemperatureMax", HTTP_GET, [](AsyncWebServerRequest *request) {
    jsonresponse = "";
    doc.clear();
    if (request->authenticate(readFile(SPIFFS, "/username.txt").c_str(), readFile(SPIFFS, "/password.txt").c_str()))
    {
      doc["temperatureMax"] = readFile(SPIFFS, "/temperature.txt");
      serializeJsonPretty(doc, jsonresponse);
      request->send(200, "application/json", jsonresponse);
    }
    else
    {
      doc["message"] = "Unauthorized : Username OR Password invalide";
      serializeJsonPretty(doc, jsonresponse);
      request->send(401, "application/json", jsonresponse);
    }
  });

  // Requête qui permet d'obtenir la température actuelle
  server.on("/obtenirTemperatureActuelle", HTTP_GET, [](AsyncWebServerRequest *request) {
    jsonresponse = "";
    doc.clear();
    if (request->authenticate(readFile(SPIFFS, "/username.txt").c_str(), readFile(SPIFFS, "/password.txt").c_str()))
    {
      doc["temperatureActuelle"] = demanderTemperature();
      serializeJsonPretty(doc, jsonresponse);
      request->send(200, "application/json", jsonresponse);
    }
    else
    {
      doc["message"] = "Unauthorized : Username OR Password invalide";
      serializeJsonPretty(doc, jsonresponse);
      request->send(401, "application/json", jsonresponse);
    }
  });

  // Requête qui permet d'obtenir l'état de la pompe
  server.on("/obtenirEtatPompe", HTTP_GET, [](AsyncWebServerRequest *request) {
    jsonresponse = "";
    doc.clear();
    if (request->authenticate(readFile(SPIFFS, "/username.txt").c_str(), readFile(SPIFFS, "/password.txt").c_str()))
    {
      if (digitalRead(Pompe) == LOW)
      {
        doc["etat"] = "La pompe est : ON";
      }
      else
      {
        doc["etat"] = "La pompe est OFF";
      }
      serializeJsonPretty(doc, jsonresponse);
      request->send(200, "application/json", jsonresponse);
    }
    else
    {
      doc["message"] = "Unauthorized : Username OR Password invalide";
      serializeJsonPretty(doc, jsonresponse);
      request->send(401, "application/json", jsonresponse);
    }
  });
  // Requête qui permet d'obtenir l'état du chauffe-eau
  server.on("/obtenirEtatChauffeEau", HTTP_GET, [](AsyncWebServerRequest *request) {
    jsonresponse = "";
    doc.clear();
    if (request->authenticate(readFile(SPIFFS, "/username.txt").c_str(), readFile(SPIFFS, "/password.txt").c_str()))
    {
      if (digitalRead(LED) == HIGH)
      {
        doc["etat"] = "Le chauffe-eau est : ON";
      }
      else
      {
        doc["etat"] = "Le chauffe-eau est : OFF";
      }
      serializeJsonPretty(doc, jsonresponse);
      request->send(200, "application/json", jsonresponse);
    }
    else
    {
      doc["message"] = "Unauthorized : Username OR Password invalide";
      serializeJsonPretty(doc, jsonresponse);
      request->send(401, "application/json", jsonresponse);
    }
  });

  //POST
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    jsonresponse = "";
    doc.clear();
    if (request->authenticate(readFile(SPIFFS, "/username.txt").c_str(), readFile(SPIFFS, "/password.txt").c_str()))
    {
      //Requête qui permet de changer le nom de l'aquarium
      if (request->url() == "/modifierNomAquarium" && request->method() == HTTP_POST)
      {
        deserializeJson(doc, (const char *)data);
        writeFile(SPIFFS, "/nom.txt", doc["nomAquarium"]);
        doc["message"] = "Le nom de l'aquarium a été modifié avec succès";
        serializeJsonPretty(doc, jsonresponse);
        request->send(200, "application/json", jsonresponse);
      }
      //Requête qui permet de changer le nom d'utilisateur pour l'authentification
      if (request->url() == "/modifierUsername" && request->method() == HTTP_POST)
      {
        deserializeJson(doc, (const char *)data);
        writeFile(SPIFFS, "/username.txt", doc["username"]);
        doc["message"] = "Le nom d'utilisateur a été modifié avec succès";
        serializeJsonPretty(doc, jsonresponse);
        request->send(200, "application/json", jsonresponse);
      }
      //Requête qui permet de changer le mot de passe pour l'authentification
      if (request->url() == "/modifierPassword" && request->method() == HTTP_POST)
      {
        deserializeJson(doc, (const char *)data);
        writeFile(SPIFFS, "/password.txt", doc["password"]);
        doc["message"] = "Le mot de passe a été modifié avec succès";
        serializeJsonPretty(doc, jsonresponse);
        request->send(200, "application/json", jsonresponse);
      }
      //Requête qui permet de changer la température max à atteindre
      if (request->url() == "/modifierTemperatureMax" && request->method() == HTTP_POST)
      {
        deserializeJson(doc, (const char *)data);
        writeFile(SPIFFS, "/temperature.txt", doc["temperatureMax"]);
        doc["message"] = "La température max a été modifié avec succès";
        serializeJsonPretty(doc, jsonresponse);
        request->send(200, "application/json", jsonresponse);
      }
    }
    else
    {
      doc["message"] = "Unauthorized : Vous n'êtes pas authorisé à faire des modifications";
      serializeJsonPretty(doc, jsonresponse);
      request->send(401, "application/json", jsonresponse);
    }
  });
#pragma endregion API

  server.onNotFound(notFound);
  Serial.println("Serveur actif!");
  server.begin();
}

void loop()
{
  Serial.print("Celsius temperature: ");
  Serial.println(demanderTemperature());
  afficherOLED();        // Fonction qui permet d'afficher les informations sur l'OLED
  verifierTemperature(); // LED => ON lorsque la température est entre la temp. min et la temp. max / Simule le heatpad
  verifierEtatPompe();   // Fonction qui surveille l'état de la pompe
  delay(1000);           // Le code est exécuté à chaque 1s
}