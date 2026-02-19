#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUIDs uniques (Identiques à ton appli Web)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// MODIFICATION : On utilise la broche A0 que tu as repérée sur ta carte
const int relayPin = A0; 

class MesCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // Pour la carte Arduino Nano ESP32 officielle, on utilise std::string
      std::string valeurRecue = pCharacteristic->getValue();

      if (valeurRecue.length() > 0) {
        char commande = valeurRecue[0];
        
        if (commande == '1') {
          digitalWrite(relayPin, HIGH); 
          Serial.println("Commande reçue : 1 -> Pompe (ON sur A0)");
        } 
        else if (commande == '0') {
          digitalWrite(relayPin, LOW);
          Serial.println("Commande reçue : 0 -> Pompe (OFF sur A0)");
        }
      }
    }
};

void setup() {
  Serial.begin(115200); 
  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Pompe éteinte au démarrage

  Serial.println("Démarrage du Bluetooth BLE...");

  BLEDevice::init("Pompe_BZBots"); // Nom visible sur le téléphone
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
  
  Serial.println("Prêt ! Connectez-vous à 'Pompe_BZBots'.");
  Serial.println("Le relais/LED doit être branché sur la broche A0.");
}

void loop() {
  delay(1000);
}