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

#include <SoftwareSerial.h>
#include <Wire.h>

byte PUMPS[] = {36,38,40,42,44,46,48}; //Logic output pins to relays
#define PUMP_COUNT 7 //Reference for arr size

long checktime = 500;
long min_checktime = 10;
byte MAX_TICKS = 24;
byte PUMP_TICKS[] = {0,0,0,0,0,0,0};
byte PUMP_STATECHANGES[] = {1,3,5,8,11,14,17};
byte OFFTIME = 6; //in 'ticks'

long previousMillis = 0;
long pumpprevMillis = 0;

const long amb_interval = 1000;

const long DPI = 30000;
long CPI = 30000;
long CPImin = 400;

long PUMP_TIMINGS[] = {30000,30000,30000,30000,30000,30000,30000}; //Each pump has subtly different timing. Start at 1 minute.
long PUMP_TIMING_PREVS[] = {0,0,0,0,0,0,0}; 
bool PUMP_STATES[] = {false, false, false, false, false, false, false};

//*** SENSOR READING PARAMS:
int ambientmax = 450;
int ambientmin = 417;
int ambientreading = 0; //the latest ambient reading
long ambientmodifier = 0;
//Typical basal reading with a good indoor calibration is 350-450.
//Unlikely to change much over time with a good calibration.

int pointmax = 100;
int pointmin = 11;
int oldpointreading = 0;
int pointreading = 0; //the latest point reading
long pointscalar = 0;
//Typical basal reading is 9-12.
//Typical maximum is 100.

//*** END SENSOR READING PARAMS

byte samplecount = 0;
byte samplethreshold = 60; //how many seconds to allow for calibration at startup

int MIN_CYCLE = 500;
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
      //Serial.print("AMB RDG: ");
      //Serial.println(ambientreading);
    }
  }
  //New Pump Control Loop- much shorter
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
    if(oldpointreading == 0) oldpointreading = pointreading;
    if(pointreading > pointmax) pointmax = pointreading;
    if(pointreading < pointmin) pointreading = pointmin;
  }

  //if(samplecount >= samplethreshold) UpdatePumpTimeSignature();
  UpdatePumpTimeSignature();
}

void UpdatePumpStates(){
  for(byte i = 0; i < PUMP_COUNT; i++){
    if(PUMP_TICKS[i] >= MAX_TICKS) PUMP_TICKS[i] = 0;
    else PUMP_TICKS[i]++;
    if(PUMP_TICKS[i] > PUMP_STATECHANGES[i] & PUMP_TICKS[i] < (PUMP_STATECHANGES[i]+ OFFTIME)){
      digitalWrite(PUMPS[i], LOW);
    }
    else digitalWrite(PUMPS[i], HIGH);
  }
}

//TODO cleanup on this
void UpdatePumpTimeSignature(){
  pointscalar = map(pointreading, pointmin, pointmax, 300, 1);
  if(pointscalar < min_checktime) pointscalar = min_checktime;
  long pointsq = (long)pointscalar * (long)pointscalar; //betw 90k and 1
  int squaredscalar = map(pointsq, 90001, 1, 300, 1); //remapping after square
  ambientmodifier = map(ambientreading, ambientmin, ambientmax, 20, 1);
  checktime = squaredscalar + ambientmodifier;
  Serial.println(checktime, DEC);
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
