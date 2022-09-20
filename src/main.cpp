#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>

//---------------Hardware Config------------------//
const int LM35_Pin = A0; //LM35 signal wire to analog 0
const int Photo_Pin = A1; //Photores wire to analog 1

const int chipSelect = 10; //Chip select pin to digital 10. Wired internally, don't use D10
uint32_t syncTime = 0; // time of last sync()

//---------------RTC Config-----------------------//
RTC_PCF8523 RTC; //defines RTC object

//---------------SD Config------------------------//
File logfile; //defines file
const int SYNC_INTERVAL = 10000; //Sync every 10sec
const int LOG_INTERVAL = 1000; //Measure every 1sec


void setup() {
  //Start the serial port
  Serial.begin(57600);

  //Setting pins
  pinMode(LM35_Pin, INPUT);
  pinMode(Photo_Pin, INPUT);
  pinMode(chipSelect, OUTPUT);

  //--------------RTC Setup---------------------//
  Serial.print("Setup RTC...");
  Wire.begin();
  if (!RTC.begin()) { //checking to make sure our RTC is there
    Serial.print("RTC failed");
  }

  if (!RTC.initialized() || RTC.lostPower()) { //If the RTC doesn't know what time it is, we give it the computer's time
    Serial.print("RTC not initialized, setting time...");
    RTC.adjust(DateTime(F(__DATE__),F(__TIME__)));
    Serial.println("Time set.");
  }
  RTC.start();
  Serial.println("RTC good.");

  //--------------SD Setup----------------------//
  Serial.print("Setup SD...");
  if(!SD.begin(chipSelect)) { //Check to see if card worked
    Serial.print("Card failed.");
    while (1); //We just loop forever since there's no point to do anything if the card doesn't log
  }
  Serial.println("Card good.");

  //-------------Logfile Setup------------------//
  Serial.print("Setup logfile...");
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) { //Making filename
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) { // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  //leave the loop!
    }
  }

  if (!logfile) { //We check to see if the log file actually opened
    Serial.print("Logfile failed.");
    while (1); //endless loop of death
  }
  Serial.print("Logging to: ");
  Serial.println(filename);

  //Formatting the log file (unix time, millis(), tempC, photoread)
  logfile.print("Unix_Time");
  logfile.print(',');
  logfile.print("millis()");
  logfile.print(',');
  logfile.print("Temperature_C");
  logfile.print(',');
  logfile.println("Photo_Reading");
}

void loop() {
  DateTime now;
  now = RTC.now();

  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

  int LM35_Reading = analogRead(LM35_Pin);
  int Photo_Reading = analogRead(Photo_Pin);

  //Doing some math to convert the analog measurement into degrees celsius
  double tempC = (LM35_Reading / 1023) * 500; 

  //Serial print some outputs for debugging
  Serial.print("Temp = ");
  Serial.print(tempC);
  Serial.print("  PhotoReading = "); 
  Serial.println(Photo_Reading);

  //We now print the values of each measurement into a single row of our CSV file
  logfile.print(now.unixtime());
  logfile.print(',');
  logfile.print(millis());
  logfile.print(',');
  logfile.print(tempC);
  logfile.print(',');
  logfile.println(Photo_Reading);

  if((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logfile.flush();
}