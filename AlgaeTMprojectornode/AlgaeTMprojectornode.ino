//Sensor Reading Relay Listener
//Developed for Alison Hiltner, 2017
//Developed by maxwell.hoaglund@gmail.com
//****
//This firmware simply listens on an rs485 bus and makes any packets it
//receives available over i2c. Expected final configuration is that This
//will be running on a device connected to a Raspberry Pi via i2c.

#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Wire.h>

#define ENABLE_PIN 4
#define TX_PIN 2
#define RX_PIN 3
SoftwareSerial rs485 (RX_PIN, TX_PIN);  // receive pin, transmit pin
long ambient = 0;
long point = 0;
byte readings[] = {0,0,0,0};

int fAvailable ()
   {
   return rs485.available ();  
   }
 
 int fRead ()
   {
   return rs485.read ();  
   }
 

RS485 channel (fRead, fAvailable, NULL, 20);

void setup() {
  pinMode (ENABLE_PIN, OUTPUT);
  digitalWrite (ENABLE_PIN, LOW);  // disable sending
  Serial.begin(9600);
  rs485.begin (38400);
  channel.begin();
  Wire.begin(8);
  TWBR = 12;
  Wire.onRequest(requestEvent);
  Serial.println("Starting!");
}

void requestEvent() {
  Wire.write(readings, sizeof(readings));
}

void loop() {
  if (channel.update ())
  {
    if(channel.getLength() != 4){
      Serial.println("Network Problem!");
      return;
    }
    int ambpack = word(channel.getData()[1], channel.getData()[0]);
    int pointpack = word(channel.getData()[3], channel.getData()[2]);
    Serial.println(ambpack); //YES!
    Serial.println(pointpack); //YES!
    readings[0] = ambpack;
    readings[1] = pointpack;
  }
}
