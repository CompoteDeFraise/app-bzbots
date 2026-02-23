#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>

// UUIDs identiques à l'application Web BZBots
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- CONFIGURATION PINS ---
const int pinEnable = A0; // Signal EN (Marche/Arrêt)
const int pinSpeed  = 9;  // Signal SV (Vitesse via PWM)

// --- ÉTAT DU SYSTÈME ---
int vitessePWM = 150;      // Valeur de base (0-255)
bool moteurActif = false;  // État de marche

class MesCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string valeurRecue = pCharacteristic->getValue();

      if (valeurRecue.length() > 0) {
        // --- CAS 1 : GESTION DE LA VITESSE (Format "SPD:XX") ---
        if (valeurRecue.find("SPD:") == 0) {
          std::string strVitesse = valeurRecue.substr(4); // On récupère ce qui est après "SPD:"
          int pourcentage = atoi(strVitesse.c_str());    // Conversion en entier (0-100)
          
          // Conversion 0-100% vers 0-255 PWM
          vitessePWM = map(pourcentage, 0, 100, 0, 255);
          
          // Si le moteur tourne, on applique la vitesse immédiatement
          if (moteurActif) {
            analogWrite(pinSpeed, vitessePWM);
          }
          
          Serial.print("Vitesse mise à jour : ");
          Serial.print(pourcentage);
          Serial.println("%");
        } 
        // --- CAS 2 : COMMANDE START ---
        else if (valeurRecue[0] == '1') {
          moteurActif = true;
          analogWrite(pinSpeed, vitessePWM);
          digitalWrite(pinEnable, LOW); // Active le driver (LOW = ON sur Primopal)
          
          Serial.print("Moteur : START | Puissance : ");
          Serial.println(vitessePWM);
        } 
        // --- CAS 3 : COMMANDE STOP ---
        else if (valeurRecue[0] == '0') {
          moteurActif = false;
          digitalWrite(pinEnable, HIGH); // Désactive le driver
          analogWrite(pinSpeed, 0);
          
          Serial.println("Moteur : STOP");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  
  // Configuration des broches
  pinMode(pinEnable, OUTPUT);
  pinMode(pinSpeed, OUTPUT);
  
  // Sécurité au démarrage
  digitalWrite(pinEnable, HIGH); 
  analogWrite(pinSpeed, 0);

  Serial.println("Initialisation Pompe BZBots (Vitesse PWM)...");

  // Initialisation Bluetooth
  BLEDevice::init("Pompe_BZBots");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MesCallbacks());
  pService->start();

  // Publicité du service
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("Prêt ! Le variateur de vitesse de l'App est maintenant fonctionnel.");
}

void loop() {
  // Le travail est géré par les callbacks BLE
  delay(10); 
}