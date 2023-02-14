#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TM1637Display.h>

/*WiFi*/
const char* ssid="..."; //name of WiFi network
const char* password="...";//WiFi password
int connectingTime=15000;
bool WiFiStatus=1;

/*NTP server*/
const char* MY_TZ="CET-1CEST,M3.5.0/02,M10.5.0/03"; //defining TZ and DST
const char* MY_NTP_SERVER="pool.ntp.org"; //name of ntp server

/*Clock*/
int hour;
int minute;
int colonPreviousMillis;
bool colonStatus;
uint8_t colon=0b01000000;
int colonDelay=500;
uint8_t error[]=
{
  SEG_G, //"-"
  SEG_G, //"-"
  SEG_G, //"-"
  SEG_G  //"-"
};

/*Display module - 4dig 7seg (TM1637)*/
#define CLK 14 //D5 for WeMos D1 mini
#define DIO 12 //D6 for WeMos D1 mini
TM1637Display display(CLK, DIO); //initialize display
int displayBrightness=1; // set display brightness from 0 (lowest brightness) to 7 (highest brightness)
const uint8_t celsiusDegrees[]=
{
  SEG_A|SEG_B|SEG_F|SEG_G,  //degree symbol
  SEG_A|SEG_F|SEG_E|SEG_D   //Celsius symbol
};

/*BME280 - temperature, pressure and humidity sensor*/
Adafruit_BME280 bme; //set I2C (SCL=D1, SDA=D2 for WeMos D1 mini)
int temp;
int press;
int hum;

/*Button*/
const int buttonPin=13; //D7 for WeMos D1 mini
int buttonState;
int buttonMode=0;
int buttonDelay=400;
unsigned long buttonPreviousMillis;


void setup()
{
  Serial.begin(115200);

  /*Connecting to WiFi*/
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid,password);
  while (millis()<connectingTime)
  {
    delay(500);
    Serial.print(".");
  }
  if(WiFi.status()==WL_CONNECTED)
  {
    Serial.println("");
    Serial.print("Connected: "); Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("");
    Serial.print("Wifi not connencted");
    WiFiStatus=0;
  }

  /*NTP configuration*/
  configTime(MY_TZ, MY_NTP_SERVER);

  /*Button configuration*/
  pinMode(buttonPin,INPUT_PULLUP);

  /*display brightness setting*/
  display.setBrightness(displayBrightness);

  /*BME280 check*/
  if (!bme.begin(0x76)) 
  {
    Serial.println("Could not find a valid BME280 sensor");
    while (1);
  }
}

void loop() 
{
  getButtonMode();
  getBME280Data();
  getTime();
  displayInfo();
}

void displayInfo()
{
  if(buttonMode==0)
  {
    if(WiFiStatus==1)
    {    
      display.showNumberDecEx(hour,colon,true,2,0);
      display.showNumberDec(minute,true,2,2);
      if(millis()-colonPreviousMillis>colonDelay)
      {
        colonStatus=!colonStatus;
        colonPreviousMillis=millis();
      }
      colonStatus ? colon=0b01000000 : colon=0b00000000;
    }
    else if(WiFiStatus==0)
    {
      display.setSegments(error,4,0);
    }
  }
  if(buttonMode==1)
  {
    display.showNumberDec(temp,false,2,0);
    display.setSegments(celsiusDegrees,2,2);
  }
  else if(buttonMode==2)
  {
    display.showNumberDec(press,false);
  }
  else if(buttonMode==3)
  {
    display.showNumberDec(hum,false);
  }
}

void getButtonMode()
{
  buttonState=digitalRead(buttonPin);

  if(buttonState==0 && millis()-buttonPreviousMillis>buttonDelay)
  {
    if(buttonMode<3)
    {
      buttonMode++;
    }
    else
    {
      buttonMode=0;
    }
    buttonPreviousMillis=millis();
  }
}

void getBME280Data()
{
  temp=round(bme.readTemperature());
  press=bme.readPressure()/100; //convert to hPa
  hum=bme.readHumidity();
}

void getTime()
{
  time_t now;                 // Unix epoch time, number of seconds that have elapsed since January 1, 1970
  tm tm;                      // transform time information into a tm structure that split time into seconds, minutes, hours, days, months, years, day of week, day of year
  
  time(&now);                 // read the current time
  localtime_r(&now, &tm);     // update the structure tm with the current time
  
  hour=tm.tm_hour;            // hour [0-23]
  minute=tm.tm_min;           // minute [0-59]
}
