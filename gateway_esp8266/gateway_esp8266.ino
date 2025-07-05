#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <RF24.h>
#include <SPI.h>

// ========== CONFIGURATION ========== //
#define WIFI_SSID       "your-ssid"
#define WIFI_PASSWORD   "your-password"
#define FIREBASE_HOST   "your-project-id.firebaseio.com"
#define FIREBASE_AUTH   "your-firebase-secret"

// ========== Firebase & WiFi ========== //
FirebaseData fbdo;
FirebaseJson json;

// ========== RF24 (nRF24L01+) Setup ========== //
RF24 radio(D2, D8);  // CE, CSN pins for ESP8266
const int maxNodes = 100;
const char* baseName = "NODE";  // e.g., NODE0001, NODE0002

// ========== Data Structure ========== //
struct DataPacket {
  float voltage;
  float current;
  float power;
  float energy;
  float balance;
  bool relay;
};

// ========== Setup Function ========== //
void setup() {
  Serial.begin(115200);

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  // Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // RF Module
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);  // More stable for long distance
  radio.setRetries(5, 15);
  radio.stopListening();

  Serial.println("Gateway ready to poll child nodes.");
}

// ========== Main Loop ========== //
void loop() {
  static int currentNode = 1;

  char pipeName[6];
  snprintf(pipeName, sizeof(pipeName), "N%04d", currentNode);  // N0001, N0002 ...

  // Open writing pipe to child node
  radio.openWritingPipe((uint8_t*)pipeName);
  const char* request = "PING";

  bool success = radio.write(&request, sizeof(request));
  if (success) {
    delay(5);
    radio.openReadingPipe(1, (uint8_t*)pipeName);
    radio.startListening();

    unsigned long start = millis();
    while (millis() - start < 100) {
      if (radio.available()) {
        DataPacket data;
        radio.read(&data, sizeof(data));

        Serial.printf("Node %d ➜ V:%.1f I:%.2f P:%.1f E:%.2f B:%.2f R:%d\n",
                      currentNode, data.voltage, data.current, data.power,
                      data.energy, data.balance, data.relay);

        // Upload to Firebase
        json.clear();
        json.set("voltage", data.voltage);
        json.set("current", data.current);
        json.set("power", data.power);
        json.set("energy", data.energy);
        json.set("balance", data.balance);
        json.set("relay", data.relay);

        String path = "/energyNodes/child" + String(currentNode);
        Firebase.updateNode(fbdo, path, json);
        break;
      }
    }

    radio.stopListening();
  } else {
    Serial.printf("❌ Node %d not responding.\n", currentNode);
  }

  currentNode++;
  if (currentNode > maxNodes) currentNode = 1;

  delay(100);  // Adjustable polling interval
}
