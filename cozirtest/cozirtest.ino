#include <Wire.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
int latestreading = 0;
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Hello");
  mySerial.begin(9600);
  
  Wire.begin(8);                // join i2c bus with address #8
  TWBR = 12;
  Wire.onRequest(requestEvent);
  
}

void loop() { // run over and over
  RefreshReading();
  delay(1000);
}

void requestEvent() {
  Wire.write(latestreading);
  RefreshReading();
}

void RefreshReading(){
  mySerial.write("Z\r\n"); //The command for retrieving a reading
  if (mySerial.available()) {
    //String strdata = mySerial.readString();
    String data = mySerial.readString();
    String reading = String(data);
    //Parsing down to the actual ppm data we need
    reading.remove(0,3);
    int cleanrdg = reading.toInt();
    latestreading = cleanrdg;
    //Serial.print(reading);
    delay(250);
    Serial.println(" ");
    Serial.print(cleanrdg);
  }
}

