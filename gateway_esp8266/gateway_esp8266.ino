#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <RF24.h>
#include <SPI.h>

#define WIFI_SSID       "your-ssid"
#define WIFI_PASSWORD   "your-password"
#define FIREBASE_HOST   "your-project-id.firebaseio.com"
#define FIREBASE_AUTH   "your-firebase-secret"

FirebaseData fbdo;
FirebaseJson json;

RF24 radio(D2, D8);  // CE, CSN
const int maxNodes = 100;
const byte heartbeatAddr[6] = "HBART";  // Heartbeat pipe

struct DataPacket {
  float voltage;
  float current;
  float power;
  float energy;
  float balance;
  bool relay;
};

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(5, 15);

  Serial.println("\n Master Gateway Ready");
}

void loop() {
  sendHeartbeat();
  pollAndUpload();
}

// Broadcast heartbeat for shadow
void sendHeartbeat() {
  static unsigned long lastBeat = 0;
  if (millis() - lastBeat >= 5000) {
    radio.stopListening();
    radio.openWritingPipe(heartbeatAddr);
    const char* beat = "HEARTBEAT";
    radio.write(&beat, sizeof(beat));
    lastBeat = millis();
  }
}

//  Poll child nodes and upload to Firestore
void pollAndUpload() {
  static int currentNode = 1;
  char pipeName[6];
  snprintf(pipeName, sizeof(pipeName), "N%04d", currentNode); // N0001-N0100

  radio.openWritingPipe((uint8_t*)pipeName);
  const char* request = "PING";

  if (radio.write(&request, sizeof(request))) {
    delay(5);
    radio.openReadingPipe(1, (uint8_t*)pipeName);
    radio.startListening();
    unsigned long start = millis();

    while (millis() - start < 100) {
      if (radio.available()) {
        DataPacket data;
        radio.read(&data, sizeof(data));

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
  }

  currentNode++;
  if (currentNode > maxNodes) currentNode = 1;
  delay(100);
}

