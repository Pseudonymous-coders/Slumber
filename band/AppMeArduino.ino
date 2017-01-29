#include "DHT.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#include <SPI.h>

//IMPORTANT: THE BELOW DEFINE TELLS IF ANY PRINTLNs WOULD APPEAR! TO LOWER MEMORY CONSUMPTION!
#define DEBUG false

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

// BLUEFRUIT FEATHER SETTINGS (CURRENTLY DIRTY, CLEAN IT UP)
#define BUFSIZE 128   // Size of the read buffer for incoming data
#define VERBOSE_MODE true  // If set to 'true' enables debug output
#define BLUEFRUIT_SWUART_RXD_PIN 9    // Required for software serial!
#define BLUEFRUIT_SWUART_TXD_PIN 10   // Required for software serial!
#define BLUEFRUIT_UART_CTS_PIN 11   // Required for software serial!
#define BLUEFRUIT_UART_RTS_PIN -1   // Optional, set to -1 if unused
#define BLUEFRUIT_UART_MODE_PIN 12    // Set to -1 if unused
#define BLUEFRUIT_SPI_CS 8
#define BLUEFRUIT_SPI_IRQ 7
#define BLUEFRUIT_SPI_RST 4    // Optional but recommended, set to -1 if unused
#define BLUEFRUIT_SPI_SCK 13
#define BLUEFRUIT_SPI_MISO 12
#define BLUEFRUIT_SPI_MOSI 11
#define FACTORYRESET_ENABLE 1
#define MINIMUM_FIRMWARE_VERSION "0.6.6"
#define MODE_LED_BEHAVIOUR "MODE"

#define SEND_TIME 200 //Every 200 millis

//TEMP/DHT CONFIGURATION
#define DHTPIN 11 //Digital pin of the temp sensor DHT 11
#define DHTTYPE DHT11 //The dht type

//ACCEL/GYRO OFFSETS
#define INTERRUPT_PIN 1 //Interrupt
#define XGYRO 220
#define YGYRO 76
#define ZGYRO -85
#define ZACCEL 1788

//VIBRATION
#define VIBRATIONPIN 10

//BAND VOLTAGE MANAGMENT
#define VBATPIN A9

//Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);


//Initialize accel/gyro
MPU6050 accel;

bool accelReady = false; //True if initialization was a success
uint8_t accelIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

//ACCEL DATA TYPES
Quaternion q;
VectorInt16 tempVals;
VectorInt16 accelVals;
VectorFloat gravity;

//ACCEL MAGNITUDE
float magnitude = 0.0;
float tempMagnitude = 0.0;

//VIBRATE
unsigned char fadeState = 0,
              fadeTime = 0,
              fadeAnalog = 0;

//DHT LAST VALUES
float lastTemperature = 0,
      lastHumidity = 0;

//BATTERY LAST VALUE
float lastBattery = 0;

//BOOL THAT BECOMES TRUE IF NEEDS TO SEND
boolean shouldSend = false;
boolean fadeDone = false, fadeUP = true;

//integers to store movement
int16_t ax, ay, az;
int16_t gx, gy, gz;

long lastFade = 0;

//Initialize the bluefruit on the SPI line
//Defaulted values from the bluefruit feather
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
unsigned long currentTime = 0;
unsigned long currentAccelTime = 0;
unsigned long currentBattTime = 0;
unsigned long currentMinTime = 0;
volatile bool accelInterrupt = false;
void accelDataReady() {
    accelInterrupt = true;
}

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setVibrate(unsigned char level) {
  digitalWrite(VIBRATIONPIN, level);
}


void fadeVibrateState() {
  //State machine, so we don't hog the entire single thread
  switch(fadeState) {
      default:
          break;
      case 1:
          if(fadeAnalog > 254) {
              fadeDone = true;
              break; //Fade state is already completed
          } else if((millis() - lastFade) < fadeTime) {
              delay(1); //Adjust one millis
              break; //Haven't adjusted to the schedule
          }
          fadeDone = false;
          setVibrate(fadeAnalog++);
          break;
     case 2:
          if(fadeAnalog < 1) {
              fadeDone = true;
              break; //Fade down completed
          } else if((millis() - lastFade) < fadeTime) {
              delay(1); //Time adjust
              break;
          }
          fadeDone = false;
          setVibrate(fadeAnalog--);
          break;
  }
}


float getVoltage() {
  return ((analogRead(VBATPIN) * 2) * 3.3) / 1024;
}


void onRecv(char *recvData, unsigned short &size) {
    char fixed[size + 1];
    strncpy(fixed, recvData, size); //Fixedsize
//    fixed[size] = '\0'; //Null terminate string

    Serial.print("Recieved: ");
    Serial.println(fixed);
    
    if(strstr(recvData,"TESTING") != NULL) {
      setVibrate(255);
    } else if(strstr(recvData, "TESTER") != NULL) {
      setVibrate(0);
    }
}


void setup()
{
    Serial.begin(9600);
    debugPrint(F("Init bluetooth"));

    if(!ble.begin(VERBOSE_MODE)) {
        error(F("No ble module"));
    }


    if(FACTORYRESET_ENABLE) {
        debugPrint(F("Factory reset..."));
        if(!ble.factoryReset()) {
            error(F("Failed FR!!!"));
        }
    }

    ble.echo(false); //Disable command echo

    debugPrint(F("Getting ble info"));

//    Disabled because I want it to run without being connected to a bluetooth device, purely for debug
//    ble.info();

    ble.verbose(false); //No more need for debugging
    
    // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
#endif
    
    accel.initialize();
    pinMode(INTERRUPT_PIN, INPUT);
    pinMode(VIBRATIONPIN, OUTPUT);

    digitalWrite(VIBRATIONPIN, LOW);
    
    debugPrint(F("Trying accel..."));
    debugPrint(accel.testConnection() ? F("Complete") : F("Failed!!!"));
    
    devStatus = accel.dmpInitialize(); //Start the accelerometer
    
    dht.begin(); //Start the lib for the temp sensor
    accel.setXGyroOffset(XGYRO);
    accel.setYGyroOffset(YGYRO);
    accel.setZGyroOffset(ZGYRO);
    accel.setZAccelOffset(ZACCEL);
    
    if(devStatus == 0) {
        debugPrint(F("Starting accel"));
        accel.setDMPEnabled(true);
        
        debugPrint(F("Attaching interrupt.."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), accelDataReady, RISING);
        
        accelIntStatus = accel.getIntStatus();
        
        debugPrint(F("Accelerometer ready!"));
        accelReady = true;
        packetSize = accel.dmpGetFIFOPacketSize();
    } else {
        debugPrint(F("Failed accel!"));
        debugPrint(F("Code: "));
        //debugPrint(F(devStatus));
    }

      //Wait for a connection
    while(!ble.isConnected()) {
        delay(700); //Slow loop, eats less processing power
    }

    // LED Activity command is only supported from 0.6.6
    if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) ) {
        // Change Mode LED Activity
        debugPrint(F("Bluetooth LED activity mode: " MODE_LED_BEHAVIOUR));
        ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    }
    currentTime = millis();
    currentAccelTime = millis();
    currentBattTime = millis();
    currentMinTime = millis();
    // Set module to DATA mode
    debugPrint(F("Switching to DATA mode!"));
    ble.setMode(BLUEFRUIT_MODE_DATA);
    lastBattery = getVoltage() * 100.0f;//Reason why I do this is because battery updates every 3 seconds, and so any sends before that would send 0
}

void loop()
{
  //This should only run if a value changed. This is seperate so that there isn't a chance of it not running due to a faulty conditional.
  //This is also at the top so that the return in the accel doesn't skip this.
    if(shouldSend){
      char buffPrint[48];
      sprintf(buffPrint, "S%d;%d;%d;%d;E", (int) magnitude, (int) lastTemperature, (int) lastHumidity, (int) lastBattery);
      ble.print(buffPrint); //Send to ble device
      //debugPrint(buffPrint);
      shouldSend = false;
    }

  //Updating the battery. This is here because we don't want to update on send because then hub wouldn't be updated with the voltage if the band sits there
  if(millis() >= currentBattTime + 30000){
    int voltage = (int) (getVoltage()*100.0f);
    //Adding a threshold because this changes frequently
    if((voltage >= (lastBattery + 1)) || (voltage <= (lastBattery - 1))){
      lastBattery = voltage;
      debugPrint(F("Battery changed-send"));
      shouldSend = true;
    }
    currentBattTime = millis();
  }
    
  //Updating temperature/humidity every two seconds because DHT updates at that rate
  //NOTE: It currently lags a little bit here. Maybe it is because of the multiple dht reads. Consolidate this in one variable?
  if(millis() >= currentTime + 2000){
    //Only sending if the values change to save power
    if(lastTemperature != dht.readTemperature(true) || lastHumidity != dht.readHumidity()){
      lastTemperature = dht.readTemperature(true);
      lastHumidity = dht.readHumidity();
      debugPrint(F("Temp changed - send"));
      shouldSend = true;
    }
    currentTime = millis();
  }

  if(millis() >= currentMinTime + 60000){
      currentMinTime = millis();
      shouldSend = true;
  }  
  if(accelReady) {//If the accelerometer exists - set true in setup
        fifoCount = accel.getFIFOCount();
        if(!accelInterrupt && fifoCount < packetSize) {
            //Wait - should this be removed?
            return;//break so that code isn't halted
        }
        
        accelInterrupt = false;
        accelIntStatus = accel.getIntStatus();
        
        
        
        if((accelIntStatus & 0x10 || fifoCount == 1024)) {//This only seems to run when the code lags a bit when the temperature updates
            accel.resetFIFO();
            return;
        } else if(accelIntStatus & 0x02) {
            while(fifoCount < packetSize)
            {
              //This is where the code spends most of its time
              fifoCount = accel.getFIFOCount();
            }
            accel.getFIFOBytes(fifoBuffer, packetSize);

            fifoCount -= packetSize;

            accel.dmpGetQuaternion(&q, fifoBuffer);
            accel.dmpGetAccel(&tempVals, fifoBuffer);
            accel.dmpGetGravity(&gravity, &q);
            accel.dmpGetLinearAccel(&accelVals, &tempVals, &gravity);
            
            //The below equation gets the diagonal of a rectangular prism with the x, y, and z values representing the side lengths
            //Making this variable so I don't have to get sqrt twice
            //float tempMagnitude = sqrt((accelVals.x * accelVals.x) + (accelVals.y * accelVals.y) + (accelVals.z * accelVals.z));
            tempMagnitude = max(tempMagnitude, max(abs(accelVals.x),max(abs(accelVals.y),abs(accelVals.z))));
            //Only send if the magnitude changed. Reason why we didn't do if it is not zero is because Eli says there will always be change.
            //Sending the max of the last second and comparing it to the previous max to prevent spamming the hub
            //NOTE: THE BELOW THRESHOLD SOMEHOW MAKES IT OVERFLOW/WAIT (BUT ONLY WHEN STILL - NOT AN ISSUE?)
            if(((magnitude >= tempMagnitude + 50) || (magnitude <= tempMagnitude - 50)) && millis() >= currentAccelTime + 1000){//Giving a 50-50 range to prevent sends when still
              debugPrint(F("Passed threshold"));
              magnitude = tempMagnitude;
              shouldSend = true;
              tempMagnitude = 0;
              currentAccelTime = millis();
            }
        }
    }
    
}

//Why do I have this? For a little bit of somewhat - optimization. We need to debug? Set DEBUG to true, and then we consume memory to see println!
//Otherwise, barely any memory taken
void debugPrint(const __FlashStringHelper* debugStatement){
  #if DEBUG == true
    Serial.println(debugStatement);
  #endif
}
