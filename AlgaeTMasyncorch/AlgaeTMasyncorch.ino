#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//Software Serial for RS485 network:
#define RS485_ENABLE_PIN 4
#define RS485_TX_PIN 2
#define RS485_RX_PIN 3
SoftwareSerial rs485 (RS485_RX_PIN, RS485_TX_PIN);  // RX, TX

size_t fWrite (const byte what)
{
  return Serial.write (what);  
}
int fAvailable (){return rs485.available ();}
int fRead (){return rs485.read ();}

RS485 _channel(fRead, fAvailable, fWrite, 20);

void setup ()
{
  Serial.begin (38400);
  _channel.begin ();
}  // end of setup

const byte msg [] = "Hello world";

void loop ()
{
  _channel.sendMsg (msg, sizeof (msg));
  delay (1000);   
}  // end of loop
