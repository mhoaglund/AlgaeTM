//Co2 Intake and Pump Control Firmware
//Developed for Alison Hiltner, 2017
//Developed by maxwell.hoaglund@gmail.com
//****
//This firmware takes a reading from each of two different co2 sensors.
//The point sensor on Serial1 measures the co2 within the environment of a mask.
//The ambient sensor measures the co2 content within the entire gallery.

//Both readings are used to control the cycle rate of a set of 7 pumps, which
//aerate tanks of algae.

//Readings are also sent to a raspberry pi via rs485.

#include <SoftwareSerial.h>
#include <Wire.h>

byte PUMPS[] = {36,38,40,42,44,46,48}; //Logic output pins to relays
#define PUMP_COUNT 7 //Reference for arr size

long previousMillis = 0;
const long amb_interval = 1000;

const long DPI = 60000;
long CPI = 60000;

long PUMP_TIMINGS[] = {60000,60000,60000,60000,60000,60000,60000}; //Each pump has subtly different timing. Start at 1 minute.
long PUMP_TIMING_PREVS[] = {0,0,0,0,0,0,0}; 
bool PUMP_STATES[] = {false, false, false, false, false, false, false};

int ambientmax = 450;
int ambientmin = 417;
int ambientreading = 0; //the latest ambient reading
long ambientmodifier = 0;
//Typical basal reading with a good indoor calibration is 350-450.
//Unlikely to change much over time with a good calibration.

int pointmax = 115;
int pointmin = 11;
int pointreading = 0; //the latest point reading
long pointscalar = 0;
//Typical basal reading is 10-14.
//Typical maximum is 100.

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
  //while (!Serial | !Serial2 | !Serial3) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
  randomSeed(analogRead(0));
  // put your setup code here, to run once:

}

void loop() {
  unsigned long currentMillis = millis();
  //Schedule interactions with ambient sensor
  if(currentMillis - previousMillis > amb_interval) {
    previousMillis = currentMillis; 
    Serial2.write("Z\r\n");  //ask for a CO2 reading
    delay(1);
    if(Serial2.available()){
      String ambreading = Serial2.readStringUntil('\n');
      //String reading = String(data);
      //Parsing down to the actual ppm data we need
      ambreading.remove(0,3);
      int cleanambrdg = ambreading.toInt();
      ambientreading = cleanambrdg;
      if(ambientreading > ambientmax) ambientmax = ambientreading;
      if(ambientreading < ambientmin) ambientmin = ambientreading;
      if(samplecount <= samplethreshold) samplecount++;
      UpdateIntern();
      Serial.print("Ambient Modifier: ");
      Serial.println(ambientmodifier);
      Serial.print("Point Scalar: ");
      Serial.print(pointmax);
      Serial.print(" ++, ");
      Serial.print(pointmin);
      Serial.print(" --, ACTUAL: ");
      Serial.print(pointscalar);
      Serial.print(" RDG: ");
      Serial.println(pointreading);
    }
  }
  //Pump control loop. Will they start in phase with one another and slowly leave?
  for(byte i=0; i<PUMP_COUNT; i++){
    //PUMP_TIMINGS[i]
    //PUMP_TIMING_PREVS[i]
    if(currentMillis - PUMP_TIMING_PREVS[i] > PUMP_TIMINGS[i]){
      PUMP_TIMING_PREVS[i] = currentMillis;
      if(PUMP_STATES[i] == true) digitalWrite(PUMPS[i], LOW);
      else digitalWrite(PUMPS[i], HIGH);
      if(PUMP_STATES[i] == true) PUMP_STATES[i] = false;
      else PUMP_STATES[i] = true;
    }
  }
  
  if (Serial1.available()){
    String ptreading = Serial1.readStringUntil('\n');
    ptreading.remove(0,10);
    int cleanptrdg = ptreading.toInt();
    pointreading = cleanptrdg;
    if(pointreading > pointmax) pointmax = pointreading;
    if(pointreading < pointmin) pointreading = pointmin;
  }

  if(samplecount >= samplethreshold) UpdateCorePumpTiming();
}

///Using sensor values, update core timing values to be interpreted per pump.
void UpdateCorePumpTiming(){
  pointscalar = map(pointreading, pointmin, pointmax, 1, 120);
  //the point reading with DIVIDE the base cycle time, cap is BASE_CYCLE/60
  ambientmodifier = map(ambientreading, ambientmin, ambientmax, 1, 10000);
  //the ambient reading with BE SUBTRACTED FROM the base cycle time, cap is BASE_CYCLE - 10 seconds
  CPI = (DPI - ambientmodifier)/pointscalar;
  for(byte i=0; i<PUMP_COUNT; i++){
    byte flip = random(10);
    long salt = random(CPI/stochasticity);
    if(flip > 5) PUMP_TIMINGS[i] = CPI + salt;
    else if (CPI > salt) PUMP_TIMINGS[i] = CPI - salt;
    if(CPI < MIN_CYCLE) CPI = MIN_CYCLE;
  }
}

void UpdateIntern(){
  Wire.beginTransmission(8);
  Wire.write(pointscalar);
  Wire.write(ambientmodifier);
  Wire.endTransmission();
}

