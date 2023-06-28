#include <Servo.h>  // Bibliothèque pour le servo moteur
#include <DHT.h>    // Bibliothèque pour le capteur de température (DHT11)
#include <ArduinoJson.h>

#define INTERVAL_SendSensorDataToESP 6000 //300000 pour 5 minutes

unsigned long currentMillis;
unsigned long previousMillis;

// Pin de configuration
const int servoPin = 9;     // Broche du servo moteur
const int resistorPin = 4;  // Broche de la résistance
const int pumpPin = 5;      // Broche connectée au relais ou transistor contrôlant la pompe

//
const int AirValue = 600;   //Place the sensor in humid soil
const int WaterValue = 350;  //Insure the sensor is dry and in the air

// Paramètres de greenhouse()
Servo myServo;  // Déclare un objet
bool greenhouseOpen = false;
bool OPEN = true;
bool CLOSE = false;
float AIR_HUMIDITY_THREASHOLD = 5.;  // on accepte 5% de tolerance entre l'humidite voulue et l'humidite mesurée avant d'agir

// Paramètres de temp()
bool resistorActive = false;
bool START_HEAT = true;
bool STOP_HEAT = false;
float TEMPERATURE_THREASHOLD = 2.;  // on accepte 2 °C de tolerance entre la temperature voulue et la temperature mesurée avant d'agir

// Paramètres de pump()
unsigned long PUMPING_DURATION = 30 * 1000;                                    // Détermine le temps d'activation. La pompe fonctionne pendant 30 secondes
unsigned long PUMP_WAITING_DURATION = 24 * 60 * 60 * 1000 - PUMPING_DURATION;  // Détermine le temps d'attente entre deux arrosages
bool pumpWaiting = false;
bool pumpActive = false;
bool START_PUMP = true;
bool STOP_PUMP = false;
unsigned long pumpStartedAt = 0;
unsigned long pumpStoppedAt = 0;

// Wanted data
float wantedAirHumidity = 0.;  // Valeur a venir changer avec le site
float wantedTemperature = 0.;
float wantedGroundHumidity = 0.;

// Srings from serial port
String tempratureRead;
String humidityRead;
String precipitationRead;


// Paramètres du DHT11
#define DHTPIN 2       // La broche à laquelle le capteur est connecté
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);

  // Setup greenhouse()
  myServo.attach(servoPin);  // Pin du servo
  myServo.write(0);          // Position initiale du servo

  // Setup temp()
  pinMode(resistorPin, OUTPUT);    // Pin de la résistance en sortie
  digitalWrite(resistorPin, LOW);  // Initailement désactivé

  // Setup pump
  pinMode(pumpPin, OUTPUT);    // Pin de la pompe en sortie
  digitalWrite(pumpPin, LOW);  // Initialement désactivé

  // Setup capteur température
  dht.begin();
}

void loop() {
  unsigned long currentTime = millis();

  if (Serial.available()) {
    readWantedData();
    delay(40);
  }

  manageHumidity(currentTime);
  manageTemperature(currentTime);
  managePump(currentTime);
  currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL_SendSensorDataToESP) {
    previousMillis = currentMillis;
    Serial.println("sendSensorDataToESP");
    //sendSensorDataToESP(millis(), random(7,35), random(35,45), random(20,100) );
    sendSensorDataToESP(millis(), readAirTemp(), readAirHumidity(), readSoilHumidity() );
    
  }
}

void readWantedData() {
  StaticJsonDocument<200> doc;                                // Déclare un document JSON statique
  DeserializationError error = deserializeJson(doc, Serial);  // Déchiffre les données JSON à partir du port série
  if (error) {
    return;
    delay(50);
  }

  wantedTemperature = doc["temprature"];
  wantedAirHumidity = doc["hygroAir"];
  wantedGroundHumidity = doc["hygroSoil"];

  Serial.println("temprature");
  Serial.println(wantedTemperature);
  Serial.println("hygroAir");
  Serial.println(wantedAirHumidity);
  Serial.println("hygroSoil");
  Serial.println(wantedGroundHumidity);
  delay(50);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////     HUMIDITY CODE   ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void manageHumidity(unsigned long currentTime) {
  long currentHumidity = readAirHumidity();  // Remplace l'humidité précédente par l'actuelle
  if (currentHumidity > 0.) {
    if (currentHumidity > wantedAirHumidity) {
      greenhouse(OPEN);
    } else if (currentHumidity < wantedAirHumidity) {
      greenhouse(CLOSE);
    }
  }
}

void greenhouse(bool wantedStateOpen) {

  if (!greenhouseOpen && wantedStateOpen) {
    // Ouvre la serre
    myServo.write(0);
    greenhouseOpen = true;
  }

  if (greenhouseOpen && !wantedStateOpen) {
    // Ferme la serre
    myServo.write(240);
    greenhouseOpen = false;
  }
}

float readAirHumidity() {
  // Lecture du pourcentage d'humidite.
  float t = dht.readHumidity();
  // Vérifier si la lecture a réussi, sinon renvoyer une valeur d'erreur
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return -999.;  // valeur d'erreur
  }
  return t;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////     TEMPERATURE CODE   ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void manageTemperature(unsigned long currentTime) {
  float currentTemperature = readAirTemp();  // Remplace la température précédente par l'actuelle
  if (currentTemperature > 0.) {
    if (currentTemperature < wantedTemperature) {
      temp(START_HEAT);
    } else if (currentTemperature > wantedTemperature) {
      temp(STOP_HEAT);
    }
  }
}

void temp(bool wantedHeaterOn) {
  if (!resistorActive && wantedHeaterOn) {
    // Allume la résistance
    digitalWrite(resistorPin, HIGH);
    resistorActive = true;
  }

  if (resistorActive && !wantedHeaterOn) {
    // Désactive la résistance
    digitalWrite(resistorPin, LOW);
    resistorActive = false;
  }
}

float readAirTemp() {
  // Lecture de la température en Celsius.
  float t = dht.readTemperature();
  // Vérifier si la lecture a réussi, sinon renvoyer une valeur d'erreur
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return -999;  // valeur d'erreur
  }
  return t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////     PUMPING CODE   ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void managePump(unsigned long currentTime) {
  // si pas pumpActive et délai pump Waiting dépassé
  if (!pumpActive && (currentTime - pumpStoppedAt) > PUMP_WAITING_DURATION) {
    pump(START_PUMP);
    pumpActive = true;
    pumpWaiting = false;
    pumpStartedAt = currentTime;
  }

  // si pumpActive et delai de pompage dépassé
  if (pumpActive && (currentTime - pumpStartedAt) > PUMPING_DURATION) {
    pump(STOP_PUMP);
    pumpActive = false;
    pumpWaiting = true;
    pumpStoppedAt = currentTime;
  }
}

void pump(bool doPump) {

  if (!pumpActive && doPump) {
    // Allume la pompe
    digitalWrite(pumpPin, HIGH);
    pumpActive = true;
  }

  if (pumpActive && !doPump) {
    // Désactive la pompe
    digitalWrite(pumpPin, LOW);
    pumpActive = false;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////  Data ARduino -> ESP   /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void sendSensorDataToESP(unsigned long Time, float dhtTemp, float dhtHygo, float soilMoisture) {
  // Créer un document JSON
  StaticJsonDocument<200> doc;
  doc["time"] = millis();  // Exemple de valeur pour le temps
  doc["tempAir"] = dhtTemp;   // Exemple de valeur pour la température
  doc["hygroAir"] = dhtHygo;  // Exemple de valeur pour l'humidité
  doc["hygroSoil"] = soilMoisture;

  // Convertir le document JSON en chaîne de caractères
  String jsonString;
  serializeJson(doc, jsonString);

  // Envoyer la chaîne de caractères JSON via le port série
  Serial.println(jsonString);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////  Soil moisture ensor   /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

float readSoilHumidity(){
  float sH = map(analogRead(A0), AirValue, WaterValue, 0, 100);
  if(sH < 0){
    sH = 0;
  }
  if(sH > 100){
    sH = 100;
  }
  return sH;
}
