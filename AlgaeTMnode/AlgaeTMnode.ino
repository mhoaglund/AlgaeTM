#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Software Serial for Local CO2 Sensor:
#define SENSOR_TX_PIN 11
#define SENSOR_RX_PIN 10
SoftwareSerial mySerial(SENSOR_RX_PIN, SENSOR_TX_PIN); // RX, TX

//Software Serial for RS485 network:
#define RS485_ENABLE_PIN 4
#define RS485_TX_PIN 3
#define RS485_RX_PIN 2
SoftwareSerial rs485 (RS485_RX_PIN, RS485_TX_PIN);  // receive pin, transmit pin
size_t fWrite (const byte what){return rs485.write(what);}
int fAvailable (){return rs485.available ();}
int fRead (){return rs485.read ();}
//The channel object manages asynchronous reads and writes.
RS485 _channel(fRead, fAvailable, fWrite, 0);

//Pump stuff:
#define PUMP_RELAY_1 = 9
#define PUMP_RELAY_2 = 8

int READING = 0;
int RATE = 0;
byte myAddress = 0;

void setup() {
   myAddress = EEPROM.read (0);
   rs485.begin (38400);
  _channel.begin();
  Serial.begin(9600);
  //Serial.println("Hello");
  mySerial.begin(9600);
  pinMode (ENABLE_PIN, OUTPUT);
  pinMode (PUMP_RELAY_1, OUTPUT);
  pinMode (PUMP_RELAY_2, OUTPUT);
}

void loop() {
  if(_channel.update()){
    Serial.write(_channel.getData(), _channel.getLength());
    byte msg [] = {
       0,  // device 0 (master),
       myAddress, //who sent this
       READING // latest sensor reading
    };
    digitalWrite (ENABLE_PIN, HIGH); //Does the class handle these?
    _channel.sendMsg (msg, sizeof (msg));
    digitalWrite (ENABLE_PIN, LOW);
    //TODO: set globals for pump rate based on what comes in from other sensors
  }
  RefreshReading();
}

void respond(msg){
  _channel.sendMsg (msg, sizeof (msg));
}
void updateReading() {
  RefreshReading();
}

void RefreshReading(){
  if (mySerial.available()) {
    //String strdata = mySerial.readString();
    int data = mySerial.read();
    READING = data;
    Serial.write(data);
  }
}

