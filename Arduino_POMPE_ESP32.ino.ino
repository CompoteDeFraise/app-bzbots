#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs identiques à l'application Web BZBots
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- CONFIGURATION PINS ---
const int pinEnable = A0; // Signal EN (Marche/Arrêt)
const int pinSpeed  = 9;  // Signal SV (Vitesse via PWM sur D9)

// --- PARAMÈTRES VITESSE ---
// Valeur entre 0 et 255 (128 = env. 50% de puissance)
int vitesseCible = 150; 

class MesCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string valeurRecue = pCharacteristic->getValue();

      if (valeurRecue.length() > 0) {
        char commande = valeurRecue[0];
        
        if (commande == '1') {
          // 1. On règle la vitesse avant de démarrer
          analogWrite(pinSpeed, vitesseCible);
          
          // 2. On active le moteur (LOW sur Primopal = ON)
          digitalWrite(pinEnable, LOW); 
          
          Serial.print("Moteur : START | Vitesse : ");
          Serial.println(vitesseCible);
        } 
        else if (commande == '0') {
          // 1. On coupe l'activation
          digitalWrite(pinEnable, HIGH);
          
          // 2. On remet la vitesse à 0
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

  Serial.println("Initialisation Pompe Brushless (Vitesse Logicielle)...");

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

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("Prêt ! Connectez D9 au port SV du driver.");
}

void loop() {
  delay(1000);
}