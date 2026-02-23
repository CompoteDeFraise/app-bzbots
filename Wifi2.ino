#include <WiFi.h>
#include <WebServer.h>

/**
 * --- CONFIGURATION RÉSEAU BZBOTS ---
 * L'ESP32 crée son propre réseau. 
 * Connectez votre smartphone à "BZBots_Pump_AP" (mdp: password_bzbots)
 */
const char* ssid = "BZBots_Pump_AP";
const char* password = "password_bzbots"; 

// --- CONFIGURATION PINS ---
const int pinEnable = A0; // Signal EN (Marche/Arrêt)
const int pinSpeed  = 9;  // Signal SV (Vitesse via PWM)

WebServer server(80);

// --- ÉTAT DU SYSTÈME ---
int vitessePWM = 150;     // Valeur par défaut (0-255)
bool moteurActif = false; // État de marche

/**
 * Ajoute les headers CORS pour permettre à l'application web 
 * de communiquer avec l'ESP32 sans blocage de sécurité.
 */
void sendCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void handleOn() {
  moteurActif = true;
  analogWrite(pinSpeed, vitessePWM);
  digitalWrite(pinEnable, LOW); // Signal d'activation (LOW sur Primopal)
  Serial.println("WIFI: Commande START reçue");
  sendCORS();
}

void handleOff() {
  moteurActif = false;
  digitalWrite(pinEnable, HIGH); // Signal de coupure
  analogWrite(pinSpeed, 0);
  Serial.println("WIFI: Commande STOP reçue");
  sendCORS();
}

void handleSpeed() {
  if (server.hasArg("val")) {
    int pourcentage = server.arg("val").toInt();
    // On bride entre 0 et 100 pour la sécurité
    pourcentage = constrain(pourcentage, 0, 100);
    
    // Conversion mathématique du signal 0-100% vers 0-255 PWM
    vitessePWM = map(pourcentage, 0, 100, 0, 255);
    
    // Si la pompe est en marche, on applique le changement immédiatement
    if (moteurActif) {
      analogWrite(pinSpeed, vitessePWM);
    }
    
    Serial.printf("WIFI: Puissance réglée à %d%%\n", pourcentage);
    sendCORS();
  }
}

void handlePing() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "PONG");
}

void setup() {
  Serial.begin(115200);
  
  // Initialisation des broches de contrôle
  pinMode(pinEnable, OUTPUT);
  pinMode(pinSpeed, OUTPUT);
  
  // Sécurité : Moteur coupé par défaut
  digitalWrite(pinEnable, HIGH); 
  analogWrite(pinSpeed, 0);

  // Démarrage du mode Point d'Accès
  Serial.println("\n--- Initialisation Point d'Accès BZBots ---");
  WiFi.softAP(ssid, password);
  
  // Par défaut, l'IP est 192.168.4.1
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Réseau : "); Serial.println(ssid);
  Serial.print("Adresse IP de la pompe : "); Serial.println(IP);

  // Définition des "routes" (les URLs que l'app appelle)
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/speed", handleSpeed);
  server.on("/ping", handlePing);

  server.begin();
  Serial.println("Serveur prêt à recevoir des commandes HTTP.");
}

void loop() {
  // On laisse le serveur traiter les requêtes entrantes
  server.handleClient();
}