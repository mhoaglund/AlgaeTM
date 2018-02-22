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

byte PUMPS[] = {36,38,40,42}; //Logic output pins to relays
#define PUMP_COUNT 4 //Reference for arr size

const long SIGNAL_CHECKTIME = 45; //pump loop speed for signal wave
const byte MAX_TICKS = 12; //number of ticks in a measure of time
const byte PUMP_STATECHANGES[] = {1,4,7,10}; //pump changeover points

//At times, pump timing will be varied by small random amounts. Pauses will also be inserted.
byte PUMP_STATECHANGE_AUX_STARTS[] = {0,0,0,0};
byte PUMP_STATECHANGE_AUX_PAUSES[] = {0,0,0,0};
byte PUMP_STATECHANGE_AUX_RAND_PAUSES[] = {0,0,0,0};

//Working variables for the pump loop.
byte PUMP_TICKS = 12;
long checktime = 500;

boolean DIR = true;
boolean CLEANSWEEP = true;
const byte CLEAN_THRESHOLD = 40;
const byte STEADY_TIME = 8;

long previousMillis = 0;
long pumpprevMillis = 0;

const long amb_interval = 1000;

//*** SENSOR READING PARAMS:
int ambientmax = 515;
int ambientmin = 375;
int ambientreading = 450; //the latest ambient reading
//Typical basal reading with a good indoor calibration is 350-450.
//Unlikely to change much over time with a good calibration.

int pointmax = 60;
int pointmin = 11;
int pointreading = 0; //the latest point reading
int oldpointreading = 0;
const int MIN_BASE_CHECKTIME = 90;
const int MAX_BASE_CHECKTIME = 450;
//Typical basal reading is 9-12.
//Typical maximum is 100.

boolean SHOULD_SIGNAL = false;
boolean SWEEP_DEBOUNCE = false;
long debouncemillis = 0;
long debounce_interval = 60000;

//*** END SENSOR READING PARAMS
byte stochasticity = 4;

void setup() {
  pinMode(PUMPS[0], OUTPUT);
  pinMode(PUMPS[1], OUTPUT);
  pinMode(PUMPS[2], OUTPUT);
  pinMode(PUMPS[3], OUTPUT);
  Serial.begin(9600);
  //Serial3.begin(9600);
  Serial1.begin(9600);
  Wire.begin();
  randomSeed(analogRead(0));
  Serial.println("starting...");
}

void loop() {
  if(debouncemillis > debounce_interval){
    debouncemillis = 0;
    SWEEP_DEBOUNCE = false;
    Serial.println("Debounce gate closed");
  }
  if(SWEEP_DEBOUNCE) debouncemillis++;

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > amb_interval) {
    previousMillis = currentMillis; 
    
    UpdateIntern();
  }
  
  long timeDelta = currentMillis - pumpprevMillis;
  if(timeDelta > checktime) {
    pumpprevMillis = currentMillis; 
    UpdatePumpStates();
  }
  
  if (Serial1.available()){
    String ptreading = Serial1.readStringUntil('\n');
    if(ptreading.length() < 10) return;
    ptreading.remove(0,10); //the SprintIR sensor sends back a raw reading and a processed one. trimming the raw one here.
    int cleanptrdg = ptreading.toInt();
    if(cleanptrdg < 11) cleanptrdg = 10;
    pointreading = cleanptrdg;
    Serial.print("PT: ");
    Serial.println(pointreading);
    if(pointreading > pointmax) pointmax = pointreading;
    if(pointreading < pointmin) pointreading = pointmin;
  }
  UpdatePumpTimeSignature();
}

byte offct = 0;
byte onct = 0;
void UpdatePumpStates(){
  if(PUMP_TICKS >= MAX_TICKS){
    DecoratePumpCycle();
    if(CLEANSWEEP){
      if(onct < STEADY_TIME){
        onct++;
        return;
      }else onct = 0;
    }
    DIR = false;
  }
  if(PUMP_TICKS <= 0){
    if(CLEANSWEEP){
      if(offct < STEADY_TIME){
        offct++;
        return;
      }else offct = 0;
    }
    DIR = true;
    SHOULD_SIGNAL = false; //clear signalling flag
  }
  if(DIR) PUMP_TICKS++;
  else PUMP_TICKS--;
    
  if(CLEANSWEEP | SHOULD_SIGNAL){
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
      if(PUMP_TICKS < PUMP_STATECHANGE_AUX_STARTS[i] | PUMP_TICKS == PUMP_STATECHANGE_AUX_PAUSES[i]){
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

//May need to debounce this if readings ever vacillate near to our clean threshold.
void SetSweepState(boolean _state){
  if(CLEANSWEEP == _state | SWEEP_DEBOUNCE == true) return;
  Serial.println("Flipped cleansweep state, signal wave incoming");
  SWEEP_DEBOUNCE = true;
  CLEANSWEEP = _state;
  SHOULD_SIGNAL = true;
  PUMP_TICKS = 1;
  DIR = true;
  for(byte i = 0; i < PUMP_COUNT; i++){
    digitalWrite(PUMPS[i], LOW);
  }
}

void UpdatePumpTimeSignature(){
  if(pointreading > CLEAN_THRESHOLD){
    SetSweepState(false);
  }
  else{
    SetSweepState(true);
  }
  if(SHOULD_SIGNAL){
    checktime = SIGNAL_CHECKTIME;
    return;
  }
  int logscalar = log((0.1 * pointreading)-0.5)*60;
  if(logscalar < 1) logscalar = 1; //superstition
  int base_scalar = map(pointreading, 300, 1, MIN_BASE_CHECKTIME, MAX_BASE_CHECKTIME);
  //int mappedscalar = map(logscalar, 200, 1, MIN_BASE_CHECKTIME, MAX_BASE_CHECKTIME);
  int ambientmodifier = map(ambientreading, ambientmin, ambientmax, 20, 1);
  //checktime = mappedscalar + ambientmodifier;
  checktime = base_scalar + ambientmodifier;
  if(checktime < SIGNAL_CHECKTIME) checktime = SIGNAL_CHECKTIME; //throttling for now, for the pumps
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
