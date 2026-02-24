/*
 * PROGRAMME DE CONTRÔLE BLDC - ARDUINO NANO ESP32
 * Logique améliorée :
 * - 1 seul capteur actif VALIDÉ 20 ms = Blocage 3 s
 * - 2 capteurs actifs               = Laisse tourner
 * - 0 capteur actif                 = Laisse tourner
 */

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
const unsigned long delaiValidation = 20;   // 🔹 Anti-déclenchement parasite (20 ms)
const unsigned long tempsAveugle = 500;     // 🔹 0.5 s après redémarrage

// --- Variables d'état ---
bool estEnPause = false;
unsigned long chronoPause = 0;
unsigned long chronoRedemarrage = 0;

// --- Variables détection défaut ---
bool defautEnCours = false;
unsigned long chronoDefaut = 0;

void setup() {
  Serial.begin(115200);

  pinMode(pinEnable, OUTPUT);
  pinMode(pinDir, OUTPUT);
  pinMode(pinSpeed, OUTPUT);
  pinMode(pinBrake, OUTPUT); 

  pinMode(pinSensor1, INPUT_PULLUP);
  pinMode(pinSensor2, INPUT_PULLUP);

  digitalWrite(pinDir, HIGH);

  Serial.println("=== SYSTEME PRET (Version robuste) ===");
  faireTournerMoteur();
}

void loop() {

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

  bool unSeul = (c1 ^ c2);   // XOR → vrai si un seul actif

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
    // Etat normal → reset validation
    defautEnCours = false;
  }
}


// ================================
// FONCTIONS MOTEUR
// ================================

void faireTournerMoteur() {
  digitalWrite(pinBrake, HIGH);     // Relâche frein
  digitalWrite(pinEnable, LOW);     // Active driver
  analogWrite(pinSpeed, vitesseRotation);
}

void arreterMoteur() {
  analogWrite(pinSpeed, 0);         // Coupe PWM
  digitalWrite(pinBrake, LOW);      // Active frein
}