/*
 * PROGRAMME DE CONTRÔLE BLDC - ARDUINO NANO ESP32
 * Logiciel avec Serveur Web pour contrôle via Application BZBots
 */

#include <WiFi.h>
#include <WebServer.h> // 🔹 AJOUT : Pour écouter les commandes de l'appli

// --- Paramètres Wi-Fi (Point d'Accès) ---
const char* ssid = "Machine_UV_Relining";
const char* password = "SuperPassword123";

WebServer server(80); // 🔹 AJOUT : Le serveur écoute sur le port 80

// --- Définition des broches ---
const int pinEnable  = D4;  
const int pinDir     = D2;  
const int pinSpeed   = D3;  
const int pinBrake   = D7;  
const int pinSensor1 = D5;  
const int pinSensor2 = D6;  

// --- Paramètres ---
const int ETAT_DECLENCHEMENT = HIGH;
int vitesseRotation = 255;
const unsigned long dureePauseMs = 3000;
const unsigned long delaiValidation = 20;
const unsigned long tempsAveugle = 500;

// --- Variables d'état ---
bool estEnPause = false;
bool commandeActive = false; // 🔹 AJOUT : Est-ce que l'utilisateur a cliqué sur START ?
unsigned long chronoPause = 0;
unsigned long chronoRedemarrage = 0;

// --- Variables détection défaut ---
bool defautEnCours = false;
unsigned long chronoDefaut = 0;

// ==========================================
// 🔹 GESTIONNAIRES DE COMMANDES (WI-FI)
// ==========================================

void handleOn() {
  Serial.println("📡 Commande reçue : START");
  commandeActive = true;
  if (!estEnPause) faireTournerMoteur();
  server.send(200, "text/plain", "OK");
}

void handleOff() {
  Serial.println("📡 Commande reçue : STOP");
  commandeActive = false;
  arreterMoteur();
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("val")) {
    vitesseRotation = server.arg("val").toInt();
    Serial.print("📡 Vitesse réglée à : ");
    Serial.println(vitesseRotation);
    if (commandeActive && !estEnPause) {
       analogWrite(pinSpeed, vitesseRotation);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handlePing() {
  server.send(200, "text/plain", "PONG");
}

void setup() {
  Serial.begin(115200);

  // --- Initialisation Wi-Fi ---
  WiFi.softAP(ssid, password);
  Serial.println("Réseau Wi-Fi en ligne !");
  Serial.print("IP : ");
  Serial.println(WiFi.softAPIP());

  // --- Configuration des routes du serveur ---
  server.on("/on", handleOn);       // Route pour démarrer
  server.on("/off", handleOff);     // Route pour arrêter
  server.on("/speed", handleSpeed); // Route pour la vitesse
  server.on("/ping", handlePing);   // Route pour le test de connexion
  server.begin();

  pinMode(pinEnable, OUTPUT);
  pinMode(pinDir, OUTPUT);
  pinMode(pinSpeed, OUTPUT);
  pinMode(pinBrake, OUTPUT); 

  pinMode(pinSensor1, INPUT_PULLUP);
  pinMode(pinSensor2, INPUT_PULLUP);

  digitalWrite(pinDir, HIGH);
  arreterMoteur(); // On attend l'ordre de l'appli au démarrage

  Serial.println("=== SYSTEME PRET (Contrôle Web Actif) ===");
}

void loop() {
  // 🔹 AJOUT : Indispensable pour traiter les requêtes Wi-Fi
  server.handleClient();

  // Si l'utilisateur n'a pas lancé la pompe via l'appli, on ne fait rien
  if (!commandeActive) {
    return;
  }

  // ================================
  // 1️⃣ GESTION DE LA PAUSE 3s
  // ================================
  if (estEnPause) {
    if (millis() - chronoPause >= dureePauseMs) {
      Serial.println("▶️ Fin de pause - Redémarrage");
      faireTournerMoteur();
      estEnPause = false;
      chronoRedemarrage = millis();
    }
    return;
  }

  // ================================
  // 2️⃣ TEMPS D'AVEUGLEMENT 0.5s
  // ================================
  if (millis() - chronoRedemarrage < tempsAveugle) {
    return;
  }

  // ================================
  // 3️⃣ LECTURE CAPTEURS
  // ================================
  bool c1 = (digitalRead(pinSensor1) == ETAT_DECLENCHEMENT);
  bool c2 = (digitalRead(pinSensor2) == ETAT_DECLENCHEMENT);
  bool unSeul = (c1 ^ c2);

  // ================================
  // 4️⃣ VALIDATION DU DÉFAUT
  // ================================
  if (unSeul) {
    if (!defautEnCours) {
      defautEnCours = true;
      chronoDefaut = millis();
    }
    if (millis() - chronoDefaut >= delaiValidation) {
      Serial.println("🛑 Défaut validé : 1 seul capteur actif");
      arreterMoteur();
      estEnPause = true;
      chronoPause = millis();
      defautEnCours = false;
    }
  } else {
    defautEnCours = false;
  }
}

// ================================
// FONCTIONS MOTEUR
// ================================

void faireTournerMoteur() {
  digitalWrite(pinBrake, HIGH);     // Relâche frein
  digitalWrite(pinEnable, LOW);      // Active driver
  analogWrite(pinSpeed, vitesseRotation);
}

void arreterMoteur() {
  analogWrite(pinSpeed, 0);         // Coupe PWM
  digitalWrite(pinBrake, LOW);      // Active frein
}