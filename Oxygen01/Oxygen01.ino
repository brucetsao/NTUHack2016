#include <math.h>
#include <PowerManagement.h>

#define turnon HIGH
#define turnoff LOW
#define DHTSensorPin 7
#define ParticleReset 8
#define ParticleSensorLed 9
#define InternetLed 10
#define AccessLed 11

#include "String.h"
#include "OxygenData.h"
#include "PMType.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
// THIS INLCUDE LIB FOR dhx sensor
#include "DHT.h"
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

#include <Wire.h>  // Arduino IDE 內建
// LCD I2C Library，從這裡可以下載：
// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads

#include "RTClib.h"
RTC_DS1307 RTC;
DateTime nowT  ;

#include <SoftwareSerial.h>

uint8_t MacData[6];

SoftwareSerial mySerial(0, 1); // RX, TX
//char ssid[] = "PM25";      // your network SSID (name)
//char pass[] = "12345678";     // your network password
char ssid[] = "TSAO_1F";      // your network SSID (name)
char pass[] = "TSAO1234";     // your network password
//  使用者改上面的AP名字與AP 密碼就可以換成您自己的


int keyIndex = 0;               // your network key Index number (needed only for WEP)

const char gps_lat[] = "23.954710";
const char gps_lon[] = "120.574482";
const char gps_alt[] = "30";
//  使用者改上面的gps_lat[] & gps_lon[] 經緯度內容就可以換成您自己的位置

char server[] = "gpssensor.ddns.net"; // the MQTT server of LASS

#define MAX_CLIENT_ID_LEN 10
#define MAX_TOPIC_LEN     50
char clientId[MAX_CLIENT_ID_LEN];
char outTopic[MAX_TOPIC_LEN];

WiFiClient wifiClient;
PubSubClient client(wifiClient);
IPAddress  Meip , Megateway , Mesubnet ;
String MacAddress ;
int status = WL_IDLE_STATUS;
boolean ParticleSensorStatus = true ;
WiFiUDP Udp;

const char ntpServer[] = "pool.ntp.org";
const long timeZoneOffset = 28800L;
//const long timeZoneOffset = 0L;
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
const byte nptSendPacket[ NTP_PACKET_SIZE] = {
    0xE3, 0x00, 0x06, 0xEC, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x31, 0x4E, 0x31, 0x34,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
byte ntpRecvBuffer[ NTP_PACKET_SIZE ];

#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
static  const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0
uint32_t epochSystem = 0; // timestamp of system boot up


#define pmsDataLen 24
unsigned char buf[pmsDataLen];
int idx = 0;
int pm10 = 0;
int pm25 = 0;
int pm100 = 0;
int NDPyear, NDPmonth, NDPday, NDPhour, NDPminute, NDPsecond;
int HumidityData = 0 ;
int TemperatureData = 0 ;
unsigned long epoch  ;

DHT dht(DHTSensorPin, DHTTYPE);
bool hasPm25Value = false;
int ErrorCount = 0 ;
#define ErrorRebootCount 5
BloodData blooddata ;
char serialchar ;
String readdata  ;
int readlen = 0 ;

void setup() 
{
  // put your setup code here, to run once:
   // initPins() ;
   Serial.begin(9600);
  
     mySerial.begin(115200); // PMS 3003 UART has baud rate 9600


  
}

void loop() 
{
    if (mySerial.available()>0)
        {
            serialchar =  mySerial.read() ;
            if (serialchar == 0x55)
                {
                  DecodeData(blooddata) ;
                  ShowData() ;
                }
        }
    
    
  // put your main code here, to run repeatedly:
  
}

boolean  DecodeData(BloodData bdata)
{
  int stepno = 0 ;
    readlen = 0 ;
    readdata = "U" ;
    if (mySerial.available()>0)
       while ( true) 
          {
            serialchar =  mySerial.read() ;
        //    Serial.println(serialchar) ;
            if (serialchar == 0x0d)
            {
               serialchar =  mySerial.read() ;
                       if (stepno == 3) 
                          {
                            // readdata[readlen+1] = 0x0a ;
                              blooddata.BloodFlow = readdata.toInt() ;
                              stepno = 4 ;
                              readlen = 0 ;
                           //                       Serial.println("add Blood Flow") ;
                        //       Serial.println(bdata.BloodFlow) ;
                                  readdata = "" ;
                              }
              //    Serial.println("end of read") ;
                break ;
            }
            if (( serialchar != 0x3a) && ( serialchar != 0x2c))
                {
                //    Serial.println("add char") ;
                     readlen ++ ;
                     readdata.concat(serialchar) ; 
                     loop ;
                }
                if ( serialchar == 0x2c) 
                    {
                 //   Serial.println("Comma detect") ;

                     if (stepno == 2) 
                          {
                           //  readdata[readlen+1] = 0x0a ;
                              blooddata.HeartBeats = readdata.toInt() ;
                              stepno = 3 ;
                              readlen = 0 ;
                              //                    Serial.println("add Heart Flow") ;
                              //                      Serial.println(bdata.HeartBeats) ;
                                   readdata = "" ;
                                   loop ;
                          }
                     if (stepno == 1) 
                          {
                           //  readdata[readlen+1] = 0x0a ;
                              blooddata.OxyData = readdata.toInt() ;                             
                              stepno = 2 ;
                              readlen = 0 ;
                                 //                 Serial.println("add OxyData") ;   
                                 //                   Serial.println(bdata.OxyData) ;   
                                   readdata = "" ;                        
                                  loop ;
                          }
                       }
                   if ( serialchar == 0x3a) 
                    {
                      if (stepno == 0) 
                          {
                          //   readdata[readlen+1] = 0x0a ;
                              blooddata.Tagname =readdata ;
                              stepno = 1 ;

                              //                      Serial.println("add Tagname") ; 
                               //                       Serial.println(bdata.Tagname) ;                           
                               readdata = "" ;
                              readlen = 0 ;
                                   loop ;
                          }
                    }

          }
      
}


void ShowData()
{
    Serial.print("Tag:") ;
    Serial.print(blooddata.Tagname) ;
    Serial.print("/") ;
    Serial.print(blooddata.OxyData) ;
    Serial.print("/") ;
    Serial.print(blooddata.HeartBeats) ;
    Serial.print("/") ;
    Serial.print(blooddata.BloodFlow) ;
    Serial.print("\n") ;
    
  
}

