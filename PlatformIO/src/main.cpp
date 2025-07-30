/*
https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/

OK - 06/01/2024 showTime(); //show the time on display -> showing time and seconds changing on display TFT
OK - 07.01.2024 implementation of display of date, now it shows in order and it change the color of the characters each 60 seconds
  hour:min
  day/month
  day of week
  like:
  19:12
  7/Jan
  Sun

TODO
NOK - 21/01/2024 - Config o Timer de ovos para 7 minutos
  Dados: GND <- (SW RED -Timer) -> IO27 (como input pullup)
  GND <- (Buzzer 5V) -> IO26 (Output)
*/
//************************************************************************************************
//Library Definitions:
#include <Arduino.h> //Include Arduino Headers
#include "credentials.h" // Wifi credentials file
#include "SPI.h"
#include "Adafruit_GFX.h" 
#include "WROVER_KIT_LCD.h"  //320x240 display

//************************************************************************************************
//Fonts
#include <Fonts/FreeSans24pt7b.h> //ok simples
#include <Fonts/FreeSans18pt7b.h> //ok simples

// RTC demo for ESP32, that includes TZ and DST adjustments
// Get the POSIX style TZ format string from  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// Created by Hardy Maxa
// Complete project details at: https://RandomNerdTutorials.com/esp32-ntp-timezones-daylight-saving/

#include <WiFi.h>
#include "time.h"
//************************************************************************************************
//Egg Timer definitions
// constants won't change. They're used here to set pin numbers:
const int timerButtonPin = 27; //IO where the sw of the timer is connected
const int buzzerPin = 26; //IO where the 5V buzzer is connected

// Variables will change:
boolean timerButtonState;            // the current reading from the input pin
boolean buzzerState = LOW;        // the current state of the output pin
int lasttimerButtonState = LOW;  // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 150;    // the debounce time; increase if the output flickers

unsigned long  timerTime = 5000 ; //time for the egg boils (7 minutes)
//unsigned long  timerTime = 60000 * 7; //time for the egg boils (7 minutes)

boolean swVal = 1 ; //switch still not pressed
boolean swFlag = 0; //informs when the push button was pressed
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;  // will store last time LED was updated
boolean buzzerOn = 0; //informs that the buzzer is On

//************************************************************************************************
// Display Definitions
WROVER_KIT_LCD tft;

unsigned long previousMillisClk = 0;  // will store last time the last minute has been displayed
// constants won't change:
//const long interval = 1000;  // interval at which to change de displayed time
const long interval = 60000;  // interval at which to change de displayed time

// Array for Color definitions
char *colorDef[] = { "BLACK", "BLUE", "RED", "GREEN", "CYAN", "MAGENTA", "YELLOW", "WHITE" };
uint16_t numcolorDef[] = { 0x0000, 0x001F,0xF800, 0x07E0, 0x07FF, 0xF81F, 0xFFE0, 0xFFFF };
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// TODO need organization from here ==>
void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time 1");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");

    // Print date in the format: day/month/year
  Serial.print("Current date: ");
  Serial.print(timeinfo.tm_mday);
  Serial.print("/");
  Serial.print(timeinfo.tm_mon + 1); // tm_mon is 0-based, so add 1
  Serial.print("/");
  Serial.println(timeinfo.tm_year + 1900); // tm_year is years since 1900


  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%b");
  
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
}

void  startWifi(){
  WiFi.begin(ssid, wifipw);
  Serial.println("Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Wifi RSSI=");
  Serial.println(WiFi.RSSI());
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}



//************************************************************************************************
//Functions predefinitions

void testFillScreen();  //background color test
void testText();        //display text test
void showTime();        //show the time on display
void eggTimer();        //check if egg timer has started
void readButton();      //check if egg timer push button was pressed

//************************************************************************************************
//Setup Function

void setup(){
  Serial.begin(115200);
  //Serial.setDebugOutput(true);

  //Egg Timer
  pinMode(timerButtonPin, INPUT_PULLUP); //push button timer IO pin
  pinMode(buzzerPin, OUTPUT);        //buzzer IO pin
  digitalWrite(buzzerPin, LOW);      //switches buzzer sound off

  startWifi();                    //startds the wifi connection

  //initTime("WET0WEST,M3.5.0/1,M10.5.0");   // Set for Melbourne/AU
  initTime("<-03>3");             //Set time zone for São Paulo
  setTimezone("<-03>3");          //Set time zone for São Paulo

  Serial.println("Printing NTP Server Data ...");
  printLocalTime();           

  tft.begin();                    //start display
  tft.setRotation(2);             //display in portrait mode

  //testado-ok// testFillScreen();  //background color test
  //testado-ok// testText();        //display text test
} //end setup

//************************************************************************************************
//Loop Function
void loop() {
  showTime();           //show the time on display
  //readButton();
 // eggTimer();           //run the egg timer function
  
  //delay(1000);
  //delay(1000*60);
  
}

//Function Definitions:
//**********************************************************************************
void showTime(){
  static int i = 1;                                           //changing colors: start in the second color
  unsigned long currentMillisClk = millis();                  
  if (currentMillisClk - previousMillisClk >= interval) {     //just change time on display each: 'interval' time
      previousMillisClk = currentMillisClk;                   // save the last time you changed the display screen

      struct tm timeinfo;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time 1");
        return;
      } //end if

      tft.setRotation(2);                                    //portrait mode
      tft.fillScreen(WROVER_BLACK);                          //clear screen in black
      tft.setCursor(0, 64);                                  //initial position to start to show the time
      tft.setTextColor(numcolorDef[i]);                      //always changes the font color
      tft.setFont(&FreeSans24pt7b);                          //Set the Font Type
      tft.setTextSize(2);                                    //text size: 1 is the default
      
      //******************* print current time on display: *****************************************************
      tft.print(&timeinfo, "%H"); tft.print(":"); tft.println(&timeinfo, "%M"); //print hour in format HH:MM 24hs
      //tft.println(&timeinfo, "%S"); //print seconds

      //******************* print current date on display: *****************************************************
      tft.setCursor(10, 140);                                //initial position to start to show the date
      //tft.setTextSize(1); //text size 1 is the default
      tft.setFont(&FreeSans18pt7b);                          //Set the Font Typee
      tft.print(timeinfo.tm_mday);                           //day of month
      tft.print("/");
      //tft.print(timeinfo.tm_mon + 1); // tm_mon is 0-based, so add 1
      tft.println(&timeinfo, "%b");                          //print the month with 3 letters
      //tft.print("/");
      //tft.print(timeinfo.tm_year + 1900); // tm_year is years since 1900

      //******************* print the day of week with 3 letters: *****************************************************
      tft.setCursor(0, 215);                               //initial position to start to show the day of the week
      tft.print(&timeinfo, "   %a");
      Serial.println("i = " + String (i) + " color = " + String(colorDef[i]) );
      i++;
      if (i == 8)
        i = 1; //return to the first color
  } //end if

  //******************* show status of Egg Timer on Display *****************************************************
/*  //if (swFlag) {
      tft.setCursor(0, 250);                               //initial position to start to show the day of the week
      tft.setTextSize(1);
      //tft.println("Egg Timer ON");
      tft.print("swVal: ");
      tft.print(swVal);
  //} //end if swFlag
*/
} //end void showTime


//**********************************************************************************
void testFillScreen() {
  // Testing the background colors of the TFT display
  tft.fillScreen(WROVER_BLACK);
  Serial.println("WROVER_BLACK: " + String (WROVER_BLACK));
  yield();
  delay(2000);
  tft.fillScreen(WROVER_RED);
  Serial.println("WROVER_RED: " + String (WROVER_RED));
  yield();
  delay(2000);
  tft.fillScreen(WROVER_GREEN);
  Serial.println("WROVER_GREEN: " + String (WROVER_GREEN));
  yield();
  delay(2000);
  tft.fillScreen(WROVER_BLUE);
  Serial.println("WROVER_BLUE: " + String (WROVER_BLUE));
  yield();
  delay(2000);
  tft.fillScreen(CYAN);
  Serial.println("CYAN: " + String (CYAN));
  yield();
  delay(2000);
  tft.fillScreen(MAGENTA);
  Serial.println("MAGENTA: " + String (MAGENTA));
  yield();
  delay(2000);
  tft.fillScreen(YELLOW);
  Serial.println("YELLOW: " + String (YELLOW));
  yield();
  delay(2000);
  tft.fillScreen(WHITE);
  Serial.println("WHITE: " + String (WHITE));
  yield();
  delay(2000);
  tft.fillScreen(WROVER_BLACK);
  yield();
  delay(2000);

    /*
    WROVER_BLACK: 0
    WROVER_RED: 63488
    WROVER_GREEN: 2016
    WROVER_BLUE: 31
    CYAN: 2047
    MAGENTA: 63519
    YELLOW: 65504
    WHITE: 65535
    */
}

//**********************************************************************************
void testText() {
  // OK it is working and showin in the display of the Wrover Kit all the data
  tft.fillScreen(WROVER_BLACK); //clear screen in black
  tft.setCursor(0, 64);
  tft.setTextColor(WROVER_GREEN);
  tft.setFont(&FreeSans24pt7b); // will be this one
  tft.setTextSize(2); //text size 1 is the default
  tft.println("19:48");
  tft.println("06/01");
  tft.println("SAT.");

  delay(2000);
  tft.fillScreen(WROVER_BLACK); //clear screen in black
  tft.setCursor(0, 64);
  tft.setTextColor(WROVER_GREEN);
  tft.setFont(&FreeSans24pt7b); // will be this one
  tft.setTextSize(2); //text size 1 is the default
  tft.println("20:33");
  tft.println("12/05");
  tft.println("SUN.");

}

//**********************************************************************************
void readButton() {
  boolean timerButtonVal; 
  int swCounter = 0;
  timerButtonVal = digitalRead(timerButtonPin);                                                   //check if the push buttun was pressed

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (timerButtonVal != lasttimerButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the timerButtonVal is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (timerButtonVal != timerButtonState) {
      timerButtonState = timerButtonVal;

      // only toggle the LED if the new button state is HIGH
      if (timerButtonState == HIGH) {
        buzzerState = !buzzerState;
      }
    }
  }

  swVal = buzzerState; //define if the button was pressed and saves it to use in the function eggTimer()
  //digitalWrite(buzzerPin, buzzerState);

  // save the timerButtonVal. Next time through the loop, it'll be the lasttimerButtonState:
  lasttimerButtonState = timerButtonVal;
}

//**********************************************************************************
void eggTimer() {
  
  int swCounter = 0;
  
  if ((swVal == 0) || swFlag == 1) {                                                //just run if the push button was pressed
    swFlag = 1;                                                                   //signalise that the push puttun was pressed  
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= timerTime) {                            //check if the boiling egg time was elapsed
      previousMillis = currentMillis;
      digitalWrite(buzzerPin, HIGH);                                                 //if so, then sound the alarm!
      buzzerOn = 1;
      swFlag = 0;      
      swCounter = 1;                                         
    } //end if interno
  } //end if externo
  else {
    digitalWrite(buzzerPin, LOW); 
  }

 
}