# mqtt_dht22_pir_ssd1306
NodeMCU v1.0 based Sensor connected to DHT22 for temperature, PIR for presence detection and SSD1306 OLED Display

Use Arduino IDE to upload the ino

Parts Used:
1. NodeMCU v 1.0
2. DHT22 Temperature Sensor
3. HC-SR501 Passive IR Sensor (PIR)
4. SSD 1306 128x64 OLED Display
5. 2x 4.7k Ohm Resistor
6. 10K Ohm Resistor
7. Jumper Wires
8. Reliable 5V power supply

Optional:
- Cat 6 copper wires
- Dual-sided PCB board
- Soldering Iron
- Project box (100x60x25mm)

Things to Note: Sometimes when NodeMCU sends WiFi packets it interferes with the PIR motion sensor triggering it. This happens especially when PIR sits on top of NodeMCU inside a project box. To overcome this, we need to fold a piece of houseold aluminum foil and stick duck tape on both sides (to prevent accidental shorts). This can now be stuck on the back of PIR sensor to isolate it from false triggers.
