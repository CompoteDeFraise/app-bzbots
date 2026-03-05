#include <WiFi.h>
#include <WebServer.h>

// ================== WIFI AP ==================
const char* ssid = "ESP32_Machine";     // Nom du réseau Wi-Fi
const char* password = "password123";   // Mot de passe (minimum 8 caractères)

WebServer server(80); // Serveur web sur le port 80

// ================== PINS ==================

#define ENM1 2
#define FRM1 3

#define ENM2 4
#define FRM2 5

#define PWM_M3 6
#define ENM3 7

#define BRK_M1M2 8
#define BRK3 13

#define C1 9
#define C2 10
#define C3 11
#define C4 12

// ================== PWM ESP32 ==================

#define PWM_CHANNEL_M3 0
#define PWM_FREQ 20000
#define PWM_RESOLUTION 8
#define PWM_M3_MODERE 100

#define MOTEUR_ON  LOW
#define MOTEUR_OFF HIGH

// ================== ETATS ==================

enum Etat {
  INIT_SCAN,
  MARCHE_C3,
  MARCHE_C4,
  ATTENTE_ROTATION,
  ROT_VERS_C3,
  ROT_VERS_C4
};

Etat etat = INIT_SCAN;
Etat prochainEtatRotation;

// ================== VARIABLES GLOBALES ==================

// NOUVEAU: Cette variable empêche la machine de bouger tant que le bouton "GO" n'est pas cliqué
bool isRunning = false; 

// ================== INTERRUPTIONS ==================

volatile bool flagC3 = false;
volatile bool flagC4 = false;

volatile bool autoriseC3 = false;
volatile bool autoriseC4 = false;

void IRAM_ATTR isrC3() {
  if (autoriseC3)
    flagC3 = true;
}

void IRAM_ATTR isrC4() {
  if (autoriseC4)
    flagC4 = true;
}

// ================== TIMERS ==================

unsigned long timerC3 = 0;
unsigned long timerC4 = 0;

const unsigned long TEMPS_VALIDATION = 15;

unsigned long timerRotation = 0;
const unsigned long DELAI_AVANT_ROTATION = 300;

// ================== M3 ==================

void brakeM3() {
  ledcWrite(PWM_CHANNEL_M3, 0);
  digitalWrite(ENM3, MOTEUR_OFF);
  digitalWrite(BRK3, LOW);
}

void releaseBrakeM3() {
  digitalWrite(BRK3, HIGH);
  digitalWrite(ENM3, MOTEUR_ON);
  ledcWrite(PWM_CHANNEL_M3, PWM_M3_MODERE);
}

// ================== M1 M2 ==================

void brakeM1M2() {
  digitalWrite(BRK_M1M2, LOW);
}

void releaseBrakeM1M2() {
  digitalWrite(BRK_M1M2, HIGH);
}

void driveVersC3() {
  releaseBrakeM1M2();
  digitalWrite(FRM1, HIGH);
  digitalWrite(ENM1, MOTEUR_ON);
  digitalWrite(FRM2, LOW);
  digitalWrite(ENM2, MOTEUR_ON);
}

void driveVersC4() {
  releaseBrakeM1M2();
  digitalWrite(FRM2, HIGH);
  digitalWrite(ENM2, MOTEUR_ON);
  digitalWrite(FRM1, LOW);
  digitalWrite(ENM1, MOTEUR_ON);
}

// ================== FONCTIONS WEB ==================

// Convertit l'état actuel en texte pour la page web
String getEtatNom(Etat e) {
  switch(e) {
    case INIT_SCAN: return "INIT_SCAN";
    case MARCHE_C3: return "MARCHE_C3";
    case MARCHE_C4: return "MARCHE_C4";
    case ATTENTE_ROTATION: return "ATTENTE_ROTATION";
    case ROT_VERS_C3: return "ROT_VERS_C3";
    case ROT_VERS_C4: return "ROT_VERS_C4";
    default: return "INCONNU";
  }
}

// Gère la page d'accueil du serveur web (Optionnel, juste pour debugger depuis un téléphone)
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Contrôle ESP32</title>";
  html += "<style>body{font-family: Arial; text-align: center; margin-top: 50px;} h1{color: #333;}</style>";
  html += "</head><body>";
  html += "<h1>Moniteur Système</h1>";
  html += "<h2>État de la machine : " + String(isRunning ? "<span style='color: green;'>EN MARCHE</span>" : "<span style='color: red;'>A L'ARRET</span>") + "</h2>";
  html += "<h2>État des capteurs : <span style='color: blue;'>" + getEtatNom(etat) + "</span></h2>";
  html += "<p><a href=\"/\"><button style=\"padding: 10px 20px; font-size: 16px;\">Actualiser l'état</button></a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// NOUVEAU: Exécuté quand l'app web envoie la commande /start (Bouton GO)
void handleStart() {
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Permet à l'application web de communiquer sans erreur de sécurité
  isRunning = true;
  etat = INIT_SCAN; // On réinitialise l'état au cas où
  server.send(200, "text/plain", "OK Demarrage");
  Serial.println("Commande Web recue : DEMARRAGE");
}

// NOUVEAU: Exécuté quand l'app web envoie la commande /stop (Bouton STOP)
void handleStop() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  isRunning = false;
  brakeM1M2(); // Coupe immédiatement les moteurs
  brakeM3();
  server.send(200, "text/plain", "OK Arret");
  Serial.println("Commande Web recue : ARRET");
}

// ================== SETUP ==================

void setup() {
  Serial.begin(115200); // Pour lire l'IP sur le moniteur série
  
  // Configuration des pins
  pinMode(ENM1, OUTPUT);
  pinMode(FRM1, OUTPUT);
  pinMode(ENM2, OUTPUT);
  pinMode(FRM2, OUTPUT);
  pinMode(BRK_M1M2, OUTPUT);

  pinMode(ENM3, OUTPUT);
  pinMode(BRK3, OUTPUT);

  pinMode(C1, INPUT_PULLUP);
  pinMode(C2, INPUT_PULLUP);
  pinMode(C3, INPUT_PULLUP);
  pinMode(C4, INPUT_PULLUP);

  digitalWrite(ENM1, MOTEUR_OFF);
  digitalWrite(ENM2, MOTEUR_OFF);
  digitalWrite(ENM3, MOTEUR_OFF);
  digitalWrite(BRK_M1M2, HIGH); // brake relâché
  digitalWrite(BRK3, LOW);

  ledcSetup(PWM_CHANNEL_M3, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PWM_M3, PWM_CHANNEL_M3);
  ledcWrite(PWM_CHANNEL_M3, 0);

  attachInterrupt(digitalPinToInterrupt(C3), isrC3, RISING);
  attachInterrupt(digitalPinToInterrupt(C4), isrC4, RISING);

  // --- Configuration du Point d'Accès Wi-Fi ---
  Serial.println("\nConfiguration du point d'accès Wi-Fi...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Point d'accès démarré. Adresse IP : ");
  Serial.println(IP);

  // --- Configuration et Démarrage du serveur Web ---
  server.on("/", handleRoot);
  server.on("/start", handleStart); // Fait le lien entre ton bouton web et la fonction
  server.on("/stop", handleStop);   // Fait le lien entre ton bouton web et la fonction
  server.begin();
  Serial.println("Serveur HTTP démarré.");
}

// ================== LOOP ==================

void loop() {
  // Gère les requêtes des clients web (clics sur les boutons)
  server.handleClient();

  // NOUVEAU: Si on n'a pas appuyé sur GO, on zappe la suite du code (la machine reste à l'arrêt)
  if (!isRunning) {
    return; 
  }

  bool c1 = digitalRead(C1) == LOW;
  bool c2 = digitalRead(C2) == LOW;

  bool c3 = digitalRead(C3);
  bool c4 = digitalRead(C4);

  bool frontC3 = flagC3;
  bool frontC4 = flagC4;

  flagC3 = false;
  flagC4 = false;

  switch (etat) {

    case INIT_SCAN:
      releaseBrakeM3();

      if (c3 && !c4) {
        if (timerC3 == 0) timerC3 = millis();
        if (millis() - timerC3 >= TEMPS_VALIDATION) {
          brakeM3();
          etat = MARCHE_C3;
          timerC3 = 0;
        }
      } else timerC3 = 0;

      if (c4 && !c3) {
        if (timerC4 == 0) timerC4 = millis();
        if (millis() - timerC4 >= TEMPS_VALIDATION) {
          brakeM3();
          etat = MARCHE_C4;
          timerC4 = 0;
        }
      } else timerC4 = 0;

      break;

    case MARCHE_C3:
      driveVersC3();
      if (c2) {
        etat = ATTENTE_ROTATION;
        prochainEtatRotation = ROT_VERS_C4;
        timerRotation = millis();
      }
      break;

    case MARCHE_C4:
      driveVersC4();
      if (c1) {
        etat = ATTENTE_ROTATION;
        prochainEtatRotation = ROT_VERS_C3;
        timerRotation = millis();
      }
      break;

    case ATTENTE_ROTATION:
      brakeM1M2();
      if (millis() - timerRotation >= DELAI_AVANT_ROTATION) {
        if (prochainEtatRotation == ROT_VERS_C3) {
          autoriseC3 = true;
          autoriseC4 = false;
          flagC3 = false;
          flagC4 = false;
          etat = ROT_VERS_C3;
        } else {
          autoriseC3 = false;
          autoriseC4 = true;
          flagC3 = false;
          flagC4 = false;
          etat = ROT_VERS_C4;
        }
      }
      break;

    case ROT_VERS_C3:
      releaseBrakeM3();
      if (frontC3 && !c4) {
        autoriseC3 = false;
        brakeM3();
        driveVersC3();
        etat = MARCHE_C3;
      }
      break;

    case ROT_VERS_C4:
      releaseBrakeM3();
      if (frontC4 && !c3) {
        autoriseC4 = false;
        brakeM3();
        driveVersC4();
        etat = MARCHE_C4;
      }
      break;
  }
}