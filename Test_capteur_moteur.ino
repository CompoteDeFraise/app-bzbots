/*
 * PROTOTYPE POMPES - LOCK TARGET + ALTERNANCE + WIFI
 * 1 moteur BLDC + 4 LEDs + 4 capteurs NPN
 */

#include <WiFi.h> // Utilise <ESP8266WiFi.h> si tu es sur un ESP8266

// --- CONFIGURATION WIFI ---
const char* ssid     = "TON_SSID";
const char* password = "TON_MOT_DE_PASSE";
WiFiServer server(80); // Serveur Web sur le port 80

// --- BROCHES (Vérifie la compatibilité si tu passes sur ESP32) ---
const int LED_A_ASPIRE  = 2;
const int LED_A_REFOULE = 3;
const int LED_B_ASPIRE  = 4;
const int LED_B_REFOULE = 5;

const int C1 = 6;   // Commande Aspiration
const int C2 = 7;   // Commande Refoulement
const int C3 = 8;   // Capteur cible pour C1
const int C4 = 9;   // Capteur cible pour C2

// Moteur BLDC
const int pinEnable = 10;
const int pinDir    = 11;
const int pinSpeed  = 12;
const int pinBrake  = A0; // Attention: sur ESP32, utilise plutôt un pin digital classique comme 13 ou 14

int motorSpeed = 150;

// Variables état moteur
bool isSearchingC3 = false;
bool isSearchingC4 = false;
bool defautEnCours = false;
unsigned long chronoDefaut = 0;
unsigned long chronoPause = 0;
unsigned long chronoRedemarrage = 0;

// Paramètres anti-parasite
const unsigned long delaiValidation = 20;   // ms
const unsigned long dureePauseMs   = 3000; // Pause après détection
const unsigned long tempsAveugle   = 500;  // 0.5 s après redémarrage

// Alternance stricte
int lastCommand = 0; // 0 = aucune, 1 = C1, 2 = C2

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- INITIALISATION WIFI ---
  Serial.print("Connexion au Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connecté !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
  
  server.begin(); // Démarrage du serveur web
  // ---------------------------

  pinMode(LED_A_ASPIRE, OUTPUT);
  pinMode(LED_A_REFOULE, OUTPUT);
  pinMode(LED_B_ASPIRE, OUTPUT);
  pinMode(LED_B_REFOULE, OUTPUT);

  pinMode(C1, INPUT_PULLUP);
  pinMode(C2, INPUT_PULLUP);
  pinMode(C3, INPUT_PULLUP);
  pinMode(C4, INPUT_PULLUP);

  pinMode(pinEnable, OUTPUT);
  pinMode(pinDir, OUTPUT);
  pinMode(pinSpeed, OUTPUT);
  pinMode(pinBrake, OUTPUT);

  digitalWrite(pinDir, HIGH);
  stopMotor();

  Serial.println("=== SYSTEME PRET (Lock Target + Alternance + WiFi) ===");
}

void loop() {
  // --- GESTION DU SERVEUR WEB (Placé en haut pour ne pas être bloqué par les return) ---
  gererServeurWeb();

  // Lecture commandes
  bool cmdC1 = (digitalRead(C1) == LOW);
  bool cmdC2 = (digitalRead(C2) == LOW);

  // --- Lancement recherche moteur avec alternance ---
  if (cmdC1 && !isSearchingC3 && !isSearchingC4 && lastCommand != 1) {
    modePompeA_Aspire();
    Serial.println("-> Commande C1 : Recherche C3...");
    isSearchingC3 = true;
    chronoDefaut = millis();
    startMotor();
  } 
  else if (cmdC2 && !isSearchingC4 && !isSearchingC3 && lastCommand != 2) {
    modePompeA_Refoule();
    Serial.println("-> Commande C2 : Recherche C4...");
    isSearchingC4 = true;
    chronoDefaut = millis();
    startMotor();
  }

  // --- Gestion pause / temps aveugle ---
  if (chronoPause > 0 && millis() - chronoPause < dureePauseMs) return;
  if (chronoRedemarrage > 0 && millis() - chronoRedemarrage < tempsAveugle) return;

  // Lecture capteurs cibles
  bool c3 = (digitalRead(C3) == LOW);
  bool c4 = (digitalRead(C4) == LOW);

  // --- Validation XOR pour C3 ---
  if (isSearchingC3) {
    bool unSeul = c3 ^ c4;
    if (unSeul && c3) {
      if (!defautEnCours) {
        defautEnCours = true;
        chronoDefaut = millis();
      }
      if (millis() - chronoDefaut >= delaiValidation) {
        Serial.println("🛑 C3 atteint, moteur stoppé");
        stopMotor();
        chronoPause = millis();
        chronoRedemarrage = 0;
        defautEnCours = false;
        isSearchingC3 = false;
        lastCommand = 1; 
      }
    } else {
      defautEnCours = false;
    }
  }

  // --- Validation XOR pour C4 ---
  if (isSearchingC4) {
    bool unSeul = c4 ^ c3;
    if (unSeul && c4) {
      if (!defautEnCours) {
        defautEnCours = true;
        chronoDefaut = millis();
      }
      if (millis() - chronoDefaut >= delaiValidation) {
        Serial.println("🛑 C4 atteint, moteur stoppé");
        stopMotor();
        chronoPause = millis();
        chronoRedemarrage = 0;
        defautEnCours = false;
        isSearchingC4 = false;
        lastCommand = 2; 
      }
    } else {
      defautEnCours = false;
    }
  }
}

// --- Fonctions moteur ---
void startMotor() {
  digitalWrite(pinBrake, HIGH);
  digitalWrite(pinEnable, LOW);
  analogWrite(pinSpeed, motorSpeed);
}

void stopMotor() {
  analogWrite(pinSpeed, 0);
  digitalWrite(pinBrake, LOW);
}

// --- Fonctions LED pompe ---
void modePompeA_Aspire() {
  digitalWrite(LED_A_ASPIRE, HIGH);
  digitalWrite(LED_A_REFOULE, LOW);
  digitalWrite(LED_B_ASPIRE, LOW);
  digitalWrite(LED_B_REFOULE, HIGH);
}

void modePompeA_Refoule() {
  digitalWrite(LED_A_ASPIRE, LOW);
  digitalWrite(LED_A_REFOULE, HIGH);
  digitalWrite(LED_B_ASPIRE, HIGH);
  digitalWrite(LED_B_REFOULE, LOW);
}

// --- Fonction Serveur Web ---
void gererServeurWeb() {
  WiFiClient client = server.available(); // Vérifie si un client (navigateur) se connecte
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          // La ligne vide marque la fin de la requête HTTP
          if (currentLine.length() == 0) {
            // Envoi de l'en-tête HTTP
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=UTF-8");
            client.println("Connection: close");
            client.println();
            
            // Affichage HTML
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<title>Supervision Pompes</title></head>");
            client.println("<body style=\"font-family: Arial; text-align: center; margin-top: 50px;\">");
            client.println("<h1>État du Système</h1>");
            
            // Affichage dynamique de l'état
            if (isSearchingC3) {
              client.println("<h2 style=\"color: blue;\">Moteur en marche : Recherche C3 (Aspiration)</h2>");
            } else if (isSearchingC4) {
              client.println("<h2 style=\"color: green;\">Moteur en marche : Recherche C4 (Refoulement)</h2>");
            } else {
              client.println("<h2 style=\"color: red;\">Moteur à l'arrêt</h2>");
            }

            client.print("<p>Dernière commande exécutée : <strong>");
            if (lastCommand == 0) client.print("Aucune");
            else if (lastCommand == 1) client.print("C1 (Aspiration)");
            else if (lastCommand == 2) client.print("C2 (Refoulement)");
            client.println("</strong></p>");

            client.println("</body></html>");
            client.println(); // Fin de la réponse
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop(); // Ferme la connexion
  }
}