/**
 * @file softRTC.ino
 * @brief Implementation of an sotfware RTC in ESP8266 or Arduino
 * @author Jorge Aguero
 * @date 19/09/2016
 */
#include <EEPROM.h>

#define LOCMAX       16   //EEPROM size and locations
#define LOCMYRESET   0    //status of software reset
#define LOCMIN       1
#define LOCHOUR      2
#define LOCDAY       3 
#define LOCMONTH     4
#define LOCYEAR      5
#define LOCDAYFIX    7    //correction24h
#define LOCMTHFIX    8    //correction30d

#define SOFTRESET       0xAA  //switch to make a software reset and not lose dates
#define STARTHOUR       13    //Hour of start the clock

unsigned long timeNow = 0;
unsigned long timeLast = 0;

int dayMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};  //days of each month
unsigned int seconds = 0;
int minutes = 0;
int hours = STARTHOUR;
int days =  1;
int months = 1;
int years = 16;

boolean timeSync = false;  //After of the reset, the clock not is syncronize
boolean upReset = true;
boolean showMsg = true;

//daily adjustment to synchronize the clock error
int dailyFactor = 5; // set the average number of seconds slow/fast time of your CPU clock for each day
int correction24h = 0; //counter variable. Every 24 hours to correct
boolean dailySlowClock = true; // delay o not
boolean fixToday = false; // daily adjustment 

//monthly adjustment to synchronize the clock error
int monthlyFactor = 0; //set the average number of seconds your CPU time delays or advance for month
int correction30d = 1; //counter variable. Every 30 days to correct 
boolean monthlySlowClock = true; // delay o not
boolean fixMonth = false; // monthly adjustment 

/**
 * @brief Split a string according to a char (separator)
 * @details Split a string according to a char and stores it in an array
 * 
 * @param cadena input string
 * @param separador char of separation
 * @param arr output array of "int" that store the parts
 */
void splitString(String cadena, char separador, int *arr){
  int punt = 0;
  int init = 0;
  String data = "";
  int k = 0;

  punt = cadena.indexOf(separador);
  while( punt != -1){
    arr[k] = cadena.substring(init, punt).toInt();
    init = punt + 1;
    punt = cadena.indexOf(separador, init);
    k++;
  }
   arr[k] = cadena.substring(init, cadena.length()).toInt();
 
}


/**
 * @brief Set time and date
 * @details Set time of clock 
 * 
 * @param time time of clock
 * @param fecha date of clock
 */
void setTime(String fecha, String time) {
  int tmp[4];
  splitString(time, ':', tmp);
  timeNow = millis()/1000; 
  seconds = (unsigned char) (timeNow - timeLast);
  timeLast -= (tmp[2] - seconds);  //set the seconds as close as possible
  minutes = tmp[1];
  hours = tmp[0];
  splitString(fecha, '/', tmp);
  days =  tmp[0];
  months = tmp[1];
  years = tmp[2];
  timeSync = true;
  //Reset millis() (ESP.reset), to improve the precision (slow/fast your Arduino 
  //internal clock), but you must save startingHour in the eeprom
}

/**
 * @brief Set time for serial port
 * @details Set time with the string
 */
void setTimeForSerial(){
	if (Serial.available()){  //expected by example 20/09/16 10:00:00\n
		String dateTime = Serial.readStringUntil('\n');
		dateTime.trim();
		int punt = dateTime.indexOf(' ');
	    if ( punt != -1){
	    	setTime(dateTime.substring(0, punt), dateTime.substring(punt +1 , dateTime.length()));
	    }
	}
}


/**
 * @brief Save the time clocks
 * @details Save in the eeprom the time
 */
void storeTime(){
  EEPROM.write(LOCMIN, minutes);
  EEPROM.write(LOCHOUR, hours);
  EEPROM.write(LOCDAY, days);
  EEPROM.write(LOCMONTH, months);
  EEPROM.write(LOCYEAR, years);
  EEPROM.write(LOCDAYFIX, correction24h);
  EEPROM.write(LOCMTHFIX, correction30d);
  EEPROM.commit();
}

/**
 * @brief Read the time clocks
 * @details Read time of the eeprom
 */
void readTime(){
  minutes =  EEPROM.read(LOCMIN);
  hours = EEPROM.read(LOCHOUR);
  days = EEPROM.read(LOCDAY);
  months = EEPROM.read(LOCMONTH);
  years = EEPROM.read(LOCYEAR);
  correction24h = EEPROM.read(LOCDAYFIX);
  correction30d = EEPROM.read(LOCMTHFIX);
}

/**
 * @brief Software reset of ESP 
 * @details Software reset without losing time
 */
void softResetForTime(){
    if ( (hours == 4) || (hours == 10) || (hours == 16) || (hours == 22) ) {
      if ((minutes==5) && !upReset) {
          EEPROM.write(LOCMYRESET, SOFTRESET);
          storeTime();
          delay(1);
          ESP.reset();
      } else if (minutes==6){
         upReset = false;
      }
    } else {
      upReset = false;
    }
}

/**
 * @brief Check that is software reset
 * @details Recover the time, it is software reset
 */
void setTimeAfterReset(){
  if (EEPROM.read(LOCMYRESET) == SOFTRESET){
      readTime();
      EEPROM.write(LOCMYRESET, 0);
      EEPROM.commit();
      timeSync = true;
      Serial.println("\n Iniciando del soft reset ");
  }
  upReset = true;
}

/**
 * @brief Show time
 * @details Show time
 */
void show(){  //debug
  Serial.print("\n The time is: ");
  char tmpStr[20] = {0};
  sprintf(tmpStr, "%02d/%02d/%02d  ", days, months, years);  
  Serial.print(tmpStr);
  sprintf(tmpStr, "%02d:%02d:%02d", hours, minutes, seconds); 
  Serial.print(tmpStr);
}


/**
 * @brief Update time
 * @details Update the time
 */
void updateTime(){
  timeNow = millis()/1000; // the number of milliseconds that have passed since boot
  seconds = (unsigned char) (timeNow - timeLast); //Check roll over. The number of seconds that have passed since the last time.
   
  //update minutes
  if (seconds >= 60) {  //It must equal 60, but to prevent
    timeLast = timeNow;
    minutes++;
    showMsg = true;  //debug
    seconds = 0;  //debug
  }
  
  //if 60 minutes has passed, updates the hours of the clock.
  if (minutes == 60){ 
    minutes = 0;
    hours++;
    correction24h++;  //every 24 hours fix clock
  }
  
  //if 24 hours has passed, add one day to the clock
  if (hours == 24){
    hours = 0;
    days++;
    correction30d++;  //every 30 days fix clock
  }

  //verify if the last day of months has passed
  if (days > dayMonth[months-1]){
    if( (years%4==0) && (months==2) && (days==29)) { // It is a leap-year and have not been 29/02
    } else {
      days = 1;
      months++;
    }
  }

  //end the year
  if (months == 13){
      days=1;
      months=1;
      years++;
     
  }

  //if 24 hours have passed, add (+ o -) the correction (in seconds). 
  //You must determine how many seconds slow/fast your Arduino internal clock is on a daily.
  if ((correction24h == 24) && fixToday){
    if (dailySlowClock){
      timeLast += dailyFactor;
    } else {
      timeLast -= dailyFactor;
    }
    fixToday = false;
    correction24h = 0;
  }
  
  //every time 24 hours has passed, you must activated the correction of the clock
  if ( (correction24h == 23) && !fixToday){
       fixToday = true;
  } 

  //if one month have passed , add the correction (in seconds). 
  //You must determine how many seconds slow/fast your Arduino internal clock is on a monthly.
  if ((correction30d == 31) && fixMonth) {
    if (monthlySlowClock) {
      timeLast += monthlyFactor;
    } else {
      timeLast -= monthlyFactor;
    }
    fixMonth = false;
    correction30d = 1;
  }
  
  //every 30 days have passed, you must activated the correction of the clock
  if ( (correction30d == 30) && !fixMonth) {
      fixMonth = true;
  } 
  
  //debug
  if (showMsg) {
    show();
    showMsg = false;
  }

  softResetForTime();
}

void setup() {
  timeSync = false;
  upReset = true;
  Serial.begin(115200);
  EEPROM.begin(LOCMAX);
  setTimeAfterReset();
}


void loop() {
  updateTime();
  setTimeForSerial(); //test
}
