/*
 Radio Hobby APRS Weather Station
 Bazuje na bibliotece QAPRS
 http://sq9mdd.qrz.pl
 
 Ten kod jest wolnym oprogramowaniem; możesz go rozprowadzać dalej
 i/lub modyfikować na warunkach Powszechnej Licencji Publicznej GNU,
 wydanej przez Fundację Wolnego Oprogramowania - według wersji 2 tej
 Licencji lub (według twojego wyboru) którejś z późniejszych wersji.
 
 Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
 użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
 gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
 ZASTOSOWAŃ. W celu uzyskania bliższych informacji sięgnij do
 Powszechnej Licencji Publicznej GNU.
 
 weather frame with position without time structure
 ramka pogodowa z pozycja bez czasu

   !               - required      identifier
   5215.01N        - required      latitude
   /               - required      symbol table
   02055.58E       - required      longtitude
   _               - required      icon (must be WX)
   ...             - required      wind direction (from 0-360) if no data set: "..."         
   /               - required      separator
   ...             - required      wind speed (average last 1 minute) if no data set: "..."          
   g...            - required      wind gust (max from last 5 mins) if no data set: "g..." 
   t030            - required      temperature in fahrenheit    
   r000            - option        rain xxx
   p000            - option        rain xxx
   P000            - option        rain xxx  
   h00             - option        relative humidity (00 means 100%Rh)     
   b10218          - option        atmosferic pressure in hPa multiple 10  
   Fxxxx           - option        water level above or below flood stage see: http://aprs.org/aprs12/weather-new.txt
   V138            - option        battery volts in tenths   128 would mean 12.8 volts
   Xxxx            - option        radiation lvl
 
*/
/*****************************************************************************************/
// zmienne do modyfikacji ustawienia podstawowe trakera
char callsign[]                     = "SQ9MDD";                           // CALLSIGN
char latitude[]                     = "5215.00N";
char longtitude[]                   = "02055.61E";
int above_sea_lvl                   = 125;
char comment[]                      = " test pogodynki";                    // comment max 46 chars
char path[]                         = "WIDE1-1";                          // packet path
int ssid                            = 11;                                 // SSID znaku
int beacon_interval                 = 10;                                 // default 10m
int tx_delay                        = 300;                                // opóznienie pomiedzy załączeniem tx a nadawaniem ramki
boolean show_voltage                = false;                              // true/false shows battery voltage
/*****************************************************************************************/

#include <ArduinoQAPRS.h>                                                 // QAPRS Library: https://bitbucket.org/Qyon/arduinoqaprs/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

#define sql_port 12                                                       // pin SQL do sprawdzania zajętości kanału
#define ptt_port 2                                                        // sterowanie nadawaniem
#define voltage_input A0 

// zmienne pomocnicze wewnętrzne oraz konfiguracja sprzętu
int voltage = 0;                                                          // zmierzone napiecie
char * packet_buffer  = "                                                                         \n";
unsigned long time_to_send = 0;

/*****************************************************************************************/

// obsługa wysyłki danych 
void send_aprs_packet(){
  if(millis() >= time_to_send){
    float tempC = bme.readTemperature();
    int tempF = tempC*9/5+32;
    int humi = bme.readHumidity();
    if(humi == 100){
      humi = 0; 
    }
    float baroR = bme.readPressure() / 100.0F;
    int baro = int(baroR + (above_sea_lvl * 0.10933))*10;
    sprintf(packet_buffer,"!%s/%s_.../...g...t%03uh%02ub%05uaaDu%s",latitude,longtitude,tempF,humi,baro,comment);
    
     digitalWrite(ptt_port,LOW);                                          // ON PTT
     delay(tx_delay);                                                     // wait for transceiver to be ready
     QAPRS.sendData(packet_buffer);                                       // wysyłka pakietu
     //gpsSerial.begin(9600);                                               // enable software seriall again
     Serial.print(packet_buffer);                                       // debug
     digitalWrite(ptt_port,HIGH);                                         // OFF PTT
     delay(10);                                                           // 
     time_to_send = millis() + (beacon_interval * 60000);
  }
}

/*****************************************************************************************/
// setup
void setup(){ 
  Serial.begin(115200); 
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }  
  analogReference(INTERNAL);                                              // zmiana punktu odniesienia pomiaru napięć na 1.1V 
  QAPRS.init(sql_port,ptt_port,callsign, '0', "APZQAP", '0', path);       // inicjalizacja QAPRS
  QAPRS.setFromAddress(callsign, ssid);                                   // set callsign and SSID
  QAPRS.setRelays(path);                                                  // set packet path  
  sprintf(packet_buffer,">RESTART");                                      // set status after boot
}

// pętla główna
void loop(){
  delay(1000);
  //Serial.print("WAAPS300");
  //Serial.print("*");
  //Serial.print(bme.readTemperature());
  //Serial.print("*");
  //Serial.print(bme.readPressure() / 100.0F);
  //Serial.print("*");
  //Serial.print(bme.readHumidity());
  send_aprs_packet();        
}
/*****************************************************************************************/
//E.O.F.
