#include "DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

// Définir le type de capteur DHT
#define DHTTYPE DHT22

// Utiliser GPIO4 qui correspond à la broche D2 sur l'ESP8266
#define DHTPIN 4  // GPIO4 correspond à la broche D2 sur NodeMCU
#define RELAY_venti 16 // d0
#define RELAY_lamp 2 // d4

// Initialisation du DHT
DHT dht(DHTPIN, DHTTYPE);

// wifi
const char* ssid = "TP-Link_3406";
const char* password = "43241668";

// mqtt
const char* mqtt_server = "192.168.0.153"; //ip du pc portable
const int mqttPort = 1883;

float consigne_temp = 25.0; // Température cible par défaut
float hysteresis = 0.5; // Pour éviter des basculements trop fréquents
float temperature;

bool mode_automatique = true; // Mode automatique par défaut
bool commande_manuelle_active = false; // Indique si une commande manuelle est active

ESP8266WiFiMulti WiFiMulti;
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Initialisation de la communication série pour afficher les données
  Serial.begin(9600);
  Serial.println("Démarrage du capteur DHT22");

  // Initialisation du capteur DHT
  dht.begin();
  pinMode(RELAY_venti, OUTPUT);
  pinMode(RELAY_lamp, OUTPUT);
  setup_wifi();
  setup_mqtt();
}

void loop() {
  delay(2000);

  // Lire la température en Celsius
  temperature = dht.readTemperature();

  // Vérifier si les lectures sont valides
  if (isnan(temperature)) {
    Serial.println("Erreur de lecture du DHT22 !");
    return;
  }

  client.publish("value", String(temperature).c_str());

  // Si en mode automatique et aucune commande manuelle n'est active
  if (mode_automatique && !commande_manuelle_active) {
    if (temperature >= consigne_temp + hysteresis) {
      digitalWrite(RELAY_venti, HIGH); // Allume le ventilateur pour refroidir
      digitalWrite(RELAY_lamp, LOW); // Éteint la lampe
    } else if (temperature <= consigne_temp - hysteresis) {
      digitalWrite(RELAY_venti, LOW); // Éteint le ventilateur
      digitalWrite(RELAY_lamp, HIGH); // Allume la lampe pour chauffer
    }
    // Si dans la plage de régulation, ne rien changer
  }

  // Afficher les valeurs lues
  Serial.print("Température: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  client.loop();
}

void setup_wifi() {
  // connexion au wifi
  WiFiMulti.addAP(ssid, password);
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.print("MAC : ");
  Serial.println(WiFi.macAddress());
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt() {
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback); // Déclaration de la fonction de souscription
  reconnect();
}

void callback(char* topic, byte *payload, unsigned int length) {
  Serial.println("-------Nouveau message du broker mqtt-----");
  Serial.print("Canal:");
  Serial.println(topic);
  Serial.print("donnee:");
  Serial.write(payload, length);
  Serial.println();

    // Convertir le payload en une chaîne de caractères correctement terminée
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';  // Assurez-vous que la chaîne est terminée


  if (strcmp(topic, "Temperature") == 0) {
    // Mettre à jour la consigne de température avec la valeur reçue
    consigne_temp = atof(message); // Convertir en flottant
    Serial.print("Nouvelle valeur de consigne de température : ");
    Serial.println(consigne_temp);
  } else if (strcmp(topic, "OnOff") == 0) {
    // Activer le mode manuel
    mode_automatique = false;
    commande_manuelle_active = true; // Une commande manuelle est active

    switch ((char)payload[0]) {
      case '1':
        Serial.println("Allumer le ventilateur (manuel)");
        digitalWrite(RELAY_venti, HIGH);
        digitalWrite(RELAY_lamp, LOW);
        break;
      case '2':
        Serial.println("Allumer la lampe (manuel)");
        digitalWrite(RELAY_lamp, HIGH);
        digitalWrite(RELAY_venti, LOW);
        break;
      case '3':
        Serial.println("Éteindre les deux (manuel)");
        digitalWrite(RELAY_venti, LOW);
        digitalWrite(RELAY_lamp, LOW);
        break;
      case '4':
        Serial.println("Passage en mode automatique");
        mode_automatique = true; // Retour au mode automatique
        commande_manuelle_active = false; // Fin de la commande manuelle
        break;
      default:
        Serial.println("Commande non reconnue");
        break;
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("ESP32Client")) {
      Serial.println("MQTT connecté");
      client.subscribe("Temperature");
      client.subscribe("OnOff");
    } else {
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
      delay(2000);
    }
  }
}
