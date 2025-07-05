# Prepaid-Energy-Meter-IoT

A scalable IoT energy metering system using:
- PZEM004T for energy monitoring
- nRF24L01+ RF modules for communication
- ESP8266 gateway for cloud sync via Firebase

## Features
- Real-time voltage, current, power, energy
- Prepaid logic with relay cutoff
- Firebase Firestore integration
- Recharge via App or Firebase console

## Folders
- child_node: Arduino sketch for individual meters
- gateway_esp8266: NodeMCU firmware with Firebase integration
- firestore: Database rules and JSON format

## License
MIT
