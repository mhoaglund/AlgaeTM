//RS485 Async receiver Firmware
//Developed for Alison Hiltner, 2017
//Developed by maxwell.hoaglund@gmail.com
//****
//This firmware asynchronously listens for sensor readings transmitted over RS485.
//It also responds to requests for those readings over i2c.

//In initial application, readings are being requested by a Raspberry Pi, which
//acts as the i2c master in this case.

//Target is Arduino 328.

#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define ENABLE_PIN 4
#define TX_PIN 2
#define RX_PIN 3
SoftwareSerial rs485 (RX_PIN, TX_PIN);  // receive pin, transmit pin
byte reducedAmbient = 0;
const int POINT_MAX = 255;
int AMB_MAX = 500;
byte reducedPoint = 0;
byte readings[] = {0,0};

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
  byte packet[] = {
    reducedAmbient,
    reducedPoint,
    };
  Wire.write(packet, sizeof(packet));
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

    if(ambpack > AMB_MAX) AMB_MAX = ambpack;
    reducedAmbient = map(ambpack, 350, AMB_MAX, 1, 11);
    if(pointpack > POINT_MAX) pointpack = POINT_MAX;
    reducedPoint = map(pointpack, 10, POINT_MAX, 1, 11);
  }
}