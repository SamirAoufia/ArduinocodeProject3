#define RELAY_PIN1 D2
#define RELAY_PIN2 D0 // Utilisez la broche D2 de l'Arduino pour le relais
#define LM35 A0
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

//wifi
const char* ssid = "SAMIR";
const char* password = "Samir1601";
//mqtt

const char* mqtt_server = "192.168.7.45"; //ip du pc portable
const int mqttPort = 1883;

int temp = 20;
float temperature;

ESP8266WiFiMulti WiFiMulti;
WiFiClient espClient;
PubSubClient client(espClient);


void setup() {
  pinMode(RELAY_PIN1, OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);

  Serial.begin(9600);
  setup_wifi();
  setup_mqtt();
  
}

void loop() {

  int lecture = analogRead(A0); // Lire la valeur analogique
  temperature =  (lecture / 1023.0) * 500; 

  client.publish("value", String(temperature).c_str());

          // Serial.println(" ");
          // Serial.print("Temperature : ");
          // Serial.print(temperature);
          // Serial.print(" \xC2\xB0");

  // Condition pour activer ou désactiver le relais en fonction de la température
  if (temperature >= temp) {
    digitalWrite(RELAY_PIN1, HIGH); 
    digitalWrite(RELAY_PIN2, LOW);

  } else {    
    digitalWrite(RELAY_PIN1, LOW);
    digitalWrite(RELAY_PIN2, HIGH);
  }

  delay(1000);

  Serial.print(temp);

  client.loop();

}




void setup_wifi(){
  //connexion au wifi
  WiFiMulti.addAP(ssid, password);
  while ( WiFiMulti.run() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.print("MAC : ");
  Serial.println(WiFi.macAddress());
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}


void setup_mqtt(){
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);//Déclaration de la fonction de souscription
  reconnect();
}


void callback(char* topic, byte *payload, unsigned int length) {
   Serial.println("-------Nouveau message du broker mqtt-----");
   Serial.print("Canal:");
   Serial.println(topic);
   Serial.print("donnee:");
   Serial.write(payload, length);
   Serial.println();

   // Recherche du mot-clé "Temperature" dans le topic
   if (strstr(topic, "Temperature") != NULL) {
     // Si le mot-clé est trouvé, mettez à jour la variable temp avec la valeur reçue
     temp = atoi((char*)payload);
     Serial.print("Nouvelle valeur de température : ");
     Serial.println(temp);
   } else if (strcmp(topic, "OnOff") == 0) {
     // Si le topic est "OnOff", mettez à jour le relais en fonction de la valeur reçue
     switch ((char)payload[0]) {
       case '0':
         if (temperature >= temp) {
           // Activer le relais
           digitalWrite(RELAY_PIN2, HIGH);
           digitalWrite(RELAY_PIN1, LOW);
           Serial.println(" - Relais activé");
         } else {
           // Désactiver le relais
           digitalWrite(RELAY_PIN2, LOW);
           digitalWrite(RELAY_PIN1, HIGH);
           Serial.println(" - Relais désactivé");
         }
         break;
       case '1':
         Serial.println("Allumer le relais 1 ");
         digitalWrite(RELAY_PIN1, HIGH);
         digitalWrite(RELAY_PIN2, LOW);
         break;
       case '2':
         Serial.println("Allumer le relais 2  ");
         digitalWrite(RELAY_PIN1, LOW);
         digitalWrite(RELAY_PIN2, HIGH);
         break;
       case '3':
         Serial.println("Éteindre les relais");
         digitalWrite(RELAY_PIN1, LOW);
         digitalWrite(RELAY_PIN2, LOW);
         break;
       default:
         Serial.println("Commande non reconnue");
         break;
     }
   }
}



 void reconnect(){
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("ESP32Client")) {
      Serial.println("MQTT connecté");
      client.subscribe("Temperature");
      client.subscribe("OnOff");
    }
    else {
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
    delay(2000);
    }
  }
}



