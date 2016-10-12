#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"


#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
#define VBATTPIN                    A9

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

//WHAT TO SEND: Temperature, Heartbeat, Accelerometer (3-axis)
// FORMATTING:
//[temp, temp, temp ;, hrt, hrt, hrt, ;, x, x, x, y, y, y, z, z, z]

String toSend = "";
String battSend = "";
int temperature, heartBeat,xAccel, yAccel, zAccel, battCounter;
long randNumber;//For debugging
float battLvl;
// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup(void)
{
  battCounter = 0;
  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Command <-> Data Mode Example"));
  Serial.println(F("------------------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));
  randomSeed(analogRead(0));//For debugging - to generate weird numbers every time BLE is reset
}

void loop(void)
{
  battCounter++; //Incriments the counter
  //Read sensors
  temperature = random(150);
  heartBeat = analogRead(0);
  xAccel = analogRead(1);
  yAccel = analogRead(2);
  zAccel = analogRead(3);
  //Construct string with ';' as seperators
  toSend = String(temperature) + ";" + String(heartBeat) + ";" + String(xAccel) + String(yAccel) + String(zAccel);
  //Serial.print(toSend);
  ble.print(toSend);
  delay(100);
  if (battCounter >= 20) {
    battLvl = analogRead(VBATTPIN);
    battLvl *= 2;
    battLvl *= 3.3;
    battLvl /= 1024;
    ble.print("V"+(String)battLvl);
    Serial.println("Voltage: "+(String)battLvl);
    battCounter = 0;
    delay(50);
  }
  
}
