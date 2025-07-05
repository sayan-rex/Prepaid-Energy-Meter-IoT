#include <SPI.h>
#include <RF24.h>
#include <SoftwareSerial.h>

RF24 radio(9, 10);  // CE, CSN
const byte address[6] = "NODE1";

SoftwareSerial pzemSerial(2, 3); // RX, TX
float voltage, current, power, energy;

float balance = 10.0;  // Initial prepaid balance
float lastEnergy = 0.0;

const int relayPin = 4;

struct DataPacket {
  float voltage;
  float current;
  float power;
  float energy;
  float balance;
  bool relay;
} packet;

void setup() {
  Serial.begin(9600);
  pzemSerial.begin(9600);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  pinMode(relayPin, OUTPUT);
}

void loop() {
  voltage = 230.0 + random(-5, 5);
  current = 0.5 + random(0, 100) / 100.0;
  power = voltage * current;
  energy += power * 1.0 / 3600.0;

  float delta = energy - lastEnergy;
  if (delta > 0.01) {
    balance -= delta;
    lastEnergy = energy;
  }

  bool relayStatus = balance > 0;
  digitalWrite(relayPin, relayStatus ? HIGH : LOW);

  packet = {voltage, current, power, energy, balance, relayStatus};
  radio.write(&packet, sizeof(packet));

  delay(5000);
}
