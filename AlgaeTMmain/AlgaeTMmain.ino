//Co2 Intake and Pump Control Firmware
//Developed for Alison Hiltner, 2017
//Developed by maxwell.hoaglund@gmail.com
//****
//This firmware takes a reading from each of two different co2 sensors.
//The point sensor on Serial1 measures the co2 within the environment of a mask.
//The ambient sensor measures the co2 content within the entire gallery.

//Both readings are used to control the cycle rate of a set of 7 pumps, which
//aerate tanks of algae.

//Readings are also sent to a listener device for routing them to other
//devices over other protocols: see AlgaeTMlistener.ino

//Target is Arduino Mega 2560.

#include <SoftwareSerial.h>
#include <Wire.h>

byte PUMPS[] = {36,38,40,42,44,46,48}; //Logic output pins to relays
#define PUMP_COUNT 7 //Reference for arr size

long checktime = 500;
byte MAX_TICKS = 21;
byte PUMP_TICKS = 0;
byte PUMP_STATECHANGES[] = {1,4,7,10,13,16,19};
byte PUMP_STATECHANGE_AUX_STARTS[] = {0,0,0,0,0,0,0};
byte PUMP_STATECHANGE_AUX_PAUSES[] = {0,0,0,0,0,0,0};
byte PUMP_STATECHANGE_AUX_RAND_PAUSES[] = {0,0,0,0,0,0,0};

boolean DIR = true;
boolean CLEANSWEEP = true;
byte CLEAN_THRESHOLD = 20;

long previousMillis = 0;
long pumpprevMillis = 0;

const long amb_interval = 1000;

//*** SENSOR READING PARAMS:
int ambientmax = 515;
int ambientmin = 375;
int ambientreading = 0; //the latest ambient reading
long ambientmodifier = 0;
//Typical basal reading with a good indoor calibration is 350-450.
//Unlikely to change much over time with a good calibration.

int pointmax = 60;
int pointmin = 11;
int pointreading = 0; //the latest point reading
const int MIN_BASE_CHECKTIME = 45;
const int MAX_BASE_CHECKTIME = 300;
//Typical basal reading is 9-12.
//Typical maximum is 100.

//*** END SENSOR READING PARAMS

byte samplecount = 0;
byte samplethreshold = 60; //how many seconds to allow for calibration at startup

byte stochasticity = 4;

void setup() {
  pinMode(PUMPS[0], OUTPUT);
  pinMode(PUMPS[1], OUTPUT);
  pinMode(PUMPS[2], OUTPUT);
  pinMode(PUMPS[3], OUTPUT);
  pinMode(PUMPS[4], OUTPUT);
  pinMode(PUMPS[5], OUTPUT);
  pinMode(PUMPS[6], OUTPUT);
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial1.begin(9600);
  Wire.begin();
  randomSeed(analogRead(0));
}

void loop() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > amb_interval) {
    previousMillis = currentMillis; 
    UpdateIntern();
    Serial2.write("Z\r\n");  //ask for a CO2 reading
    delay(1);
    if(Serial2.available()){
      String ambreading = Serial2.readStringUntil('\n');
      ambreading.remove(0,3);
      int cleanambrdg = ambreading.toInt();
      ambientreading = cleanambrdg;
      if(ambientreading > ambientmax) ambientmax = ambientreading;
      if(ambientreading < ambientmin) ambientmin = ambientreading;
      if(samplecount <= samplethreshold) samplecount++;
      //Serial.println(ambientreading);
    }
    else{
      ambientreading = 450;
    }
  }
  
  long timeDelta = currentMillis - pumpprevMillis;
  if(timeDelta > checktime) {
    pumpprevMillis = currentMillis; 
    UpdatePumpStates();
  }
  
  if (Serial1.available()){
    String ptreading = Serial1.readStringUntil('\n');
    ptreading.remove(0,10);
    int cleanptrdg = ptreading.toInt();
    pointreading = cleanptrdg;
    if(pointreading > pointmax) pointmax = pointreading;
    if(pointreading < pointmin) pointreading = pointmin;
  }
  UpdatePumpTimeSignature();
}

void UpdatePumpStates(){
    if(PUMP_TICKS >= MAX_TICKS){
      DecoratePumpCycle();
      DIR = false;
    }
    if(PUMP_TICKS <= 0){
      //if(CLEANSWEEP) CLEANSWEEP = false;
      DIR = true;
    }
    if(DIR) PUMP_TICKS++;
    else PUMP_TICKS--;
    
  if(CLEANSWEEP){
    for(byte i = 0; i < PUMP_COUNT; i++){
      if(PUMP_TICKS > PUMP_STATECHANGES[i]){
        digitalWrite(PUMPS[i], LOW);
      }
      else digitalWrite(PUMPS[i], HIGH);
    }
    return;
  }
  //If CLEANSWEEP is off, the exhale should talk back with stochastic events.
  for(byte i = 0; i < PUMP_COUNT; i++){
    if(!DIR){
      if(PUMP_TICKS < PUMP_STATECHANGE_AUX_STARTS[i] | PUMP_TICKS == PUMP_STATECHANGE_AUX_PAUSES[i] | PUMP_TICKS == PUMP_STATECHANGE_AUX_RAND_PAUSES[i]){
        digitalWrite(PUMPS[i], HIGH);
      }
      else digitalWrite(PUMPS[i], LOW);
    }
    else{
      if(PUMP_TICKS > PUMP_STATECHANGES[i]){
        digitalWrite(PUMPS[i], LOW);
      }
      else digitalWrite(PUMPS[i], HIGH);
    }
  }
}

void UpdatePumpTimeSignature(){
  if(pointreading > CLEAN_THRESHOLD) CLEANSWEEP = false;
  else CLEANSWEEP = true;

  int logscalar = log((0.1 * pointreading)-0.5)*60;
  if(logscalar < 1) logscalar = 1; //superstition
  int mappedscalar = map(logscalar, 200, 1, MIN_BASE_CHECKTIME, MAX_BASE_CHECKTIME);
  ambientmodifier = map(ambientreading, ambientmin, ambientmax, 20, 1);
  checktime = mappedscalar + ambientmodifier;
  Serial.println(logscalar, DEC);
}

void DecoratePumpCycle(){
  for(byte i = 0; i < PUMP_COUNT; i++){
     int base = PUMP_STATECHANGES[i];
     byte _newpause = random(base, base+stochasticity);
     byte _randpause = random(1, MAX_TICKS);
     byte _newstart = random(base-stochasticity, base);
     if(_newstart < 1) _newstart = 1;
     if(_newpause > MAX_TICKS) _newpause = MAX_TICKS;
     PUMP_STATECHANGE_AUX_STARTS[i] = _newstart;
     PUMP_STATECHANGE_AUX_PAUSES[i] = _newpause;
     PUMP_STATECHANGE_AUX_RAND_PAUSES[i] = _randpause;
  }
}

void UpdateIntern(){
  Wire.beginTransmission(8);
  byte packet[] = {
    lowByte(ambientreading),
    highByte(ambientreading),
    lowByte(pointreading),
    highByte(pointreading)};
  Wire.write(packet, 4);
  Wire.endTransmission();
}
