#include "RS485_protocol.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Software Serial for Local CO2 Sensor:
#define SENSOR_TX_PIN 11
#define SENSOR_RX_PIN 10
SoftwareSerial sensorSerial(SENSOR_RX_PIN, SENSOR_TX_PIN); // RX, TX

//Software Serial for RS485 network:
#define RS485_ENABLE_PIN 4
#define RS485_TX_PIN 2
#define RS485_RX_PIN 3
SoftwareSerial rs485 (RS485_RX_PIN, RS485_TX_PIN);  // RX, TX
void fWrite (const byte what){rs485.write(what);}
int fAvailable (){return rs485.available ();}
int fRead (){return rs485.read ();}

//Pump stuff:
#define PUMP_RELAY_1 9
#define PUMP_RELAY_2 8
bool PUMP1 = false;
bool PUMP2 = false;
int READING = 0;
int RATE = 0;
byte myAddress = 0;
byte neighborReadings[] = {0,0,0,0};

int interval = 50;
unsigned long previousMillis = 0;

void setup() {
  myAddress = EEPROM.read (0);
  sensorSerial.begin(9600);
  rs485.begin (38400);
  Serial.begin(9600);
  pinMode (RS485_ENABLE_PIN, OUTPUT);
  pinMode (PUMP_RELAY_1, OUTPUT);
  pinMode (PUMP_RELAY_2, OUTPUT);
  Serial.println(myAddress, DEC);
}

void loop() {
  rs485.listen();
  byte buf [10]; //why 10?
  byte received = recvMsg (fAvailable, fRead, buf, sizeof (buf));
  if (received)
   //Serial.println("Received!");
   {
    if (buf [0] != myAddress)
      return;  // TODO gather other readings that come in
    updateReading();
    byte msg [] = {
       0,  // device 0 (master),
       myAddress, //who sent this
       READING // latest sensor reading
    };
    
    delay (1);  // give the master a moment to prepare to receive
    digitalWrite (RS485_ENABLE_PIN, HIGH);  // enable sending
    sendMsg (fWrite, msg, sizeof msg);
    digitalWrite (RS485_ENABLE_PIN, LOW);  // disable sending
  }
  //updateReading();
  //unsigned long currentMillis = millis();
  //if ((unsigned long)(currentMillis - previousMillis) >= interval) {
  //   updatePumps();
  //   previousMillis = currentMillis;
  //}
}

void updateReading(){
  sensorSerial.listen();
  delay(75);
  if (sensorSerial.available()) {
    //String strdata = mySerial.readString();
    int data = sensorSerial.read();
    READING = data;
    Serial.write(data);
  }
  rs485.listen();
}

void updatePumps(){
  if(PUMP1 == true){digitalWrite (PUMP_RELAY_1, LOW);}
  else if(PUMP1 == false){digitalWrite (PUMP_RELAY_1, HIGH);}
  if(PUMP2 == true){digitalWrite (PUMP_RELAY_2, LOW);}
  else if(PUMP2 == false){digitalWrite (PUMP_RELAY_2, HIGH);}
}

