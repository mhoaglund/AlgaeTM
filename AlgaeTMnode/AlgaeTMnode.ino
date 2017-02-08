#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Software Serial for Local CO2 Sensor:
#define SENSOR_TX_PIN 11
#define SENSOR_RX_PIN 10
SoftwareSerial mySerial(SENSOR_RX_PIN, SENSOR_TX_PIN); // RX, TX

//Software Serial for RS485 network:
#define RS485_ENABLE_PIN 4
#define RS485_TX_PIN 2
#define RS485_RX_PIN 3
SoftwareSerial rs485 (RS485_RX_PIN, RS485_TX_PIN);  // RX, TX
size_t fWrite (const byte what){return rs485.write(what);}
int fAvailable (){return rs485.available ();}
int fRead (){return rs485.read ();}
//The channel object manages asynchronous reads and writes.
RS485 _channel(fRead, fAvailable, fWrite, 20);

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
  rs485.begin (38400);
  _channel.begin();
  Serial.begin(9600);
  mySerial.begin(9600);
  pinMode (RS485_ENABLE_PIN, OUTPUT);
  pinMode (PUMP_RELAY_1, OUTPUT);
  pinMode (PUMP_RELAY_2, OUTPUT);
}

void loop() {
  if(_channel.update()){
    Serial.write("something");
    //Serial.write(_channel.getData(), _channel.getLength());
    if(_channel.getData()[0] == 0){
      //if message is from master, reply.
      byte msg [] = {
         0,  // device 0 (master),
         myAddress, //who sent this
         0
         //READING // latest sensor reading
      };
      digitalWrite (RS485_ENABLE_PIN, HIGH);   
      _channel.sendMsg (msg, sizeof (msg));
      digitalWrite (RS485_ENABLE_PIN, LOW);
    }
    else{
      neighborReadings[_channel.getData()[1]] = _channel.getData()[2];
    }
    //TODO: set globals for pump rate based on what comes in from other sensors
  }
  //updateReading();
  unsigned long currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
     updatePumps();
     previousMillis = currentMillis;
  }
}

void _respond(byte msg[]){
  _channel.sendMsg (msg, sizeof (msg));
}

void updateReading(){
  if (mySerial.available()) {
    //String strdata = mySerial.readString();
    int data = mySerial.read();
    READING = data;
    //Serial.write(data);
  }
}

void updatePumps(){
  if(PUMP1 == true){digitalWrite (PUMP_RELAY_1, LOW);}
  else if(PUMP1 == false){digitalWrite (PUMP_RELAY_1, HIGH);}
  if(PUMP2 == true){digitalWrite (PUMP_RELAY_2, LOW);}
  else if(PUMP2 == false){digitalWrite (PUMP_RELAY_2, HIGH);}
}

