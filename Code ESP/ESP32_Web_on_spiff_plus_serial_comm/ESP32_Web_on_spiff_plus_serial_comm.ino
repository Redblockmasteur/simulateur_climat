#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

bool debug = false; // WARNING Enabling debug may cause instability in the communication between the Arduino and ESP.
// Wifi config
bool standalone = true;

const char* apSSID = "Serre_Conectée";


// Replace with your network credentials
const char* ssid = "Wifi name";
const char* password = "Wifi password";


#define INTERVAL_WriteToCSV 5000           // Interval en millisecondes entre chaque écriture
#define INTERVAL_SendParamsToArduino 5000  //36000  // Interval en millisecondes entre chaque écriture


unsigned long previousMillis = 0;
unsigned long CSVpreviousMillis = 0;
unsigned long inputmillis = 0;

WebServer server(80);

float temperature[24];
float humidity[24];
float precipitation[24];

float sensorAirTemperature;
float sensorAirHumidity;
float sensorGroundHumidity;

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "Page non trouvée");
    return;
  }
  String html = file.readString();
  server.send(200, "text/html", html);
  file.close();
}

void handleSuccess() {
  File file = SPIFFS.open("/Success.html", "r");
  if (!file) {
    server.send(404, "text/plain", "Page non trouvée");
    return;
  }
  String Success = file.readString();
  server.send(200, "text/html", Success);
  file.close();
}

void handleCss() {
  File file = SPIFFS.open("/style.css", "r");
  if (!file) {
    server.send(404, "text/plain", "Page non trouvée");
    return;
  }
  String css = file.readString();
  server.send(200, "text/css", css);
  file.close();
}

void handleFavicon() {
  File file = SPIFFS.open("/favicon.svg", "r");
  if (!file) {
    server.send(404, "text/plain", "Fichier non trouvé");
    return;
  }

  server.streamFile(file, "image/svg+xml");
  file.close();
}

void handleCsv() {
  File file = SPIFFS.open("/data.csv", "r");
  if (!file) {
    server.send(404, "text/plain", "Page non trouvée");
    return;
  }
  String csv = file.readString();
  server.send(200, "text/csv", csv);
  file.close();
}

void handleNotFound() {
  String message = "Erreur 404\n\n";
  message += "La page demandée n'existe pas.";
  server.send(404, "text/plain", message);
}

void handleFormSubmit() {
  for (int hour = 0; hour < 24; hour++) {
    temperature[hour] = server.arg("temperature_" + String(hour)).toFloat();
    humidity[hour] = server.arg("humidity_" + String(hour)).toFloat();
    precipitation[hour] = server.arg("precipitation_" + String(hour)).toFloat();
  }
  File file = SPIFFS.open("/Success.html", "r");
  String Success = file.readString();
  server.send(200, "text/html", Success);
  for (int hour = 0; hour < 24; hour++) {
    Serial.print("Heure : ");
    Serial.print(hour);
    Serial.print(" Température : ");
    Serial.print(temperature[hour]);
    Serial.print(" Humidité : ");
    Serial.print(humidity[hour]);
    Serial.print(" Précipitation : ");
    Serial.print(precipitation[hour]);
    Serial.println("--------------------------------------------------");
  }
}


void updateCSV(unsigned long curTime, float tempreatureCSV, float humidityCSV, float soilMoistureCSV) {

  File file = SPIFFS.open("/data.csv", FILE_APPEND);  // Ouvre le fichier data.csv en mode ajout
  if (!file) {
    Serial.println("There was an error opening the file for writing");
    return;
  }

  // Écrit les valeurs dans le fichier CSV
  file.print(curTime);
  file.print(",");
  file.print(tempreatureCSV);
  file.print(",");
  file.print(humidityCSV);
  file.print(",");
  file.print(soilMoistureCSV);
  file.println();

  file.close();
}


void setup() {
  Serial.begin(9600);

  if (standalone == false) {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connexion en cours...");
    }

    Serial.println("Connecté !");
    Serial.println(WiFi.localIP());
  } else {
    // Désactive le mode station (client) et active le mode point d'accès
    WiFi.mode(WIFI_AP);
    // Configure le réseau WiFi en tant que point d'accès avec le SSID et le mot de passe spécifiés
    WiFi.softAP(apSSID);

    // Affiche l'adresse IP du point d'accès créé
    Serial.println("Point d'acces WiFi actif");
    Serial.print("Adresse IP du point d'acces: ");
    Serial.println(WiFi.softAPIP());
  }



  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleFormSubmit);
  server.onNotFound(handleNotFound);

  // Route to load style.css file
  server.on("/style.css", handleCss);
  server.on("/success.html", handleSuccess);
  server.on("/favicon.svg", handleFavicon);

  server.begin();
  Serial.println("Serveur démarré.");

  // Initialisation du SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Erreur lors de l'initialisation du SPIFFS");
    return;
  }

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Vérifie si le fichier "data.csv" existe

  // Crée le fichier "data.csv" s'il n'existe pas
  File file = SPIFFS.open("/data.csv", FILE_WRITE);
  if (!file) {
    Serial.println("There was an error creating the file");
    return;
  }
  // Écrit les en-têtes de colonne dans le fichier CSV
  file.println("Time, Temperature, Humidity, Soil Moisture");
  file.close();

  server.on("/data.csv", handleCsv);
}

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();
  unsigned long hoursElapsed = (millis() / 3600000UL) % 24;
  // Toutes les heures l'esp trasmet ces données a lesp
  if (currentMillis - CSVpreviousMillis >= INTERVAL_SendParamsToArduino) {
    unsigned long hoursElapsed = (millis() / 3600000UL) % 24;
    CSVpreviousMillis = currentMillis;
    sendParamsToArduino(temperature[hoursElapsed], humidity[hoursElapsed], precipitation[hoursElapsed]);
  }

  if (Serial.available()) {
    readSensorData();
    delay(50);
  }
}


void readSensorData() {
  StaticJsonDocument<200> doc;                                // Déclare un document JSON statique
  DeserializationError error = deserializeJson(doc, Serial);  // Déchiffre les données JSON à partir du port série

  if (error) {
    return;
  }
  unsigned long time = doc["time"];
  sensorAirTemperature = doc["tempAir"];
  sensorAirHumidity = doc["hygroAir"];
  sensorGroundHumidity = doc["hygroSoil"];

  if (debug) {
    Serial.println("---------------- GOT DATA FROM ARDUINO ----------------");
    Serial.println("temprature");
    Serial.println(sensorAirTemperature);
    Serial.println("hygroAir");
    Serial.println(sensorAirHumidity);
    Serial.println("hygroSoil");
    Serial.println(sensorGroundHumidity);
  }


  // Write sensors data to csv
  updateCSV(time, sensorAirTemperature, sensorAirHumidity, sensorGroundHumidity);
}

void sendParamsToArduino(float tempAir, float hygroAri, float hygoSoil) {  // Envoie les valeurs cibles a l'Arduino
  // Créer un document JSON
  StaticJsonDocument<200> doc;
  doc["temprature"] = tempAir;  // Exemple de valeur pour le temps
  doc["hygroAir"] = hygroAri;   // Exemple de valeur pour la température
  doc["hygroSoil"] = hygoSoil;  // Exemple de valeur pour l'humidité du sol

  // Convertir le document JSON en chaîne de caractères
  String jsonString;
  serializeJson(doc, jsonString);

  // Envoyer la chaîne de caractères JSON via le port série
  Serial.println(jsonString);
}
