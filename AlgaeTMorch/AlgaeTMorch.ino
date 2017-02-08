#include "RS485_protocol.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>

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

const byte GRACE_PERIOD = 24; //how many times we let a node fail before we stop polling it

const byte KnownInterns = 4;
byte internReadings[] = {0,0,0,0};
byte deadNodes[] = {0,0,0,0};
byte probation[KnownInterns];

void setup() {
  myAddress = EEPROM.read(0);
  rs485.begin (38400);
  Serial.begin(9600);
  pinMode (RS485_ENABLE_PIN, OUTPUT);
  pinMode (PUMP_RELAY_1, OUTPUT);
  pinMode (PUMP_RELAY_2, OUTPUT);
}

void loop() {
  //constantly loop over intern sensors, querying each for a response.
  for (int i=1; i <= KnownInterns; i++){
    if(deadNodes[i-1] == 1){ //if we reach the id of a dead node, skip it so the whole deal doesn't slow down.
      continue; //TODO: how does a node recover from being blacklisted here? Implement a recovery timer with millis() to operate in concert with a 
      //no-contact timer on the intern end so that we go Normal Op->Fail 25 Reports->BlackList->No Contact for 30 seconds->Intern reboots->attempt contact again after 35 seconds
    }
    byte msg [] = { 
      i    // device 1
    };
    digitalWrite (RS485_ENABLE_PIN, HIGH);  // enable sending
    sendMsg (fWrite, msg, sizeof msg);
    digitalWrite (RS485_ENABLE_PIN, LOW);  // disable sending
    
  // receive response. does awaiting a response stop execution?  
    byte buf [10];
    byte received = recvMsg (fAvailable, fRead, buf, sizeof buf);
    if(received != 0 && buf[1] == i){ //if we got a response, and it's the one we expected...
      Serial.print("Received ");
      Serial.print(buf[2], DEC);
      Serial.print(" From ");
      Serial.println(buf[1]);
      probation[i-1] = 0;
    }
    else if(received == 0){
      probation[i-1] += 1;
      Serial.print("No Reply from ");
      Serial.println(i, DEC);
      if(probation[i-1] >= GRACE_PERIOD){
        deadNodes[i-1] = 1;
        probation[i-1] = 0;
      }
    }
  } 
}

//byte msg [] = {
//   0,  // device 0 (master),
//   MY_ID, //who sent this
//   packet  // latest sensor reading
//};
