//Sensor Reading Relay Listener
//Developed for Alison Hiltner, 2017
//Developed by maxwell.hoaglund@gmail.com
//****
//This firmware is an i2c intern and rs485 sender.
//Main device updates this device with sensor readings over i2c.
//When that update happens, this device asynchronously emits those
//new readings over an rs485 bus.

//In the final install, another device on the same rs485 bus will listen for readings.
//See AlgaeTMprojectornode.ino

//Target is Arduino Nano 328.

#include <RS485_non_blocking.h>

#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Wire.h>

#define ENABLE_PIN 4
#define TX_PIN 2
#define RX_PIN 3
SoftwareSerial rs485 (RX_PIN, TX_PIN);  // receive pin, transmit pin

bool hasNew = false;
long ambient = 0;
long point = 0;

size_t fWrite (const byte what)
  {
    return rs485.write (what);  
  }

RS485 channel (NULL, NULL, fWrite, 0);
  
void setup() {
  Serial.begin(9600);
  rs485.begin (38400);
  channel.begin();
  pinMode (ENABLE_PIN, OUTPUT);
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Serial.println("Starting!");
}

void receiveEvent(int howMany) {
  byte packet[] = {0,0,0,0};
  byte i = 0;
  if(howMany != 4) return; //bad packet somehow
  while (0 < Wire.available()) {
    byte c = Wire.read();
    packet[i] = c;
    i++;
  }
  int ambpack = word(packet[1], packet[0]);
  Serial.println(ambpack);
  int pointpack = word(packet[3], packet[2]);
  Serial.println(pointpack);
  const byte msg[4] = {ambpack, pointpack};
  digitalWrite (ENABLE_PIN, HIGH);
  channel.sendMsg(packet, 4);
  digitalWrite (ENABLE_PIN, LOW);
}

void loop() {
  delay(100);
}
