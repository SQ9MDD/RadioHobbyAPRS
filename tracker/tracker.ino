/*
 Radio Hobby APRS Tracker
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
 
 TABELA WYBRANYCH SYMBOLI APRS
 tabela  symbol  znaczenie
 /        [      jogger
 /        b      bike
 /        Y      yacht
 /        O      baloon
 /        C      canoe
 /        ;      campground
 \        !      emergency
 
*/
/*****************************************************************************************/
// zmienne do modyfikacji ustawienia podstawowe trakera
char callsign[]                     = "SQ9MDD";                           // znak wywoławczy
char comment[]                      = "tracker test";                     // komentarz
char path[]                         = "WIDE1-1";                          // sciezka pakietowa
char symbol_table[]                 = "/";                                // tabela symboli
char symbol[]                       = "[";                                // symbol domyslnie "[" - jogger
int ssid                            = 11;                                 // SSID znaku
unsigned long beacon_slow_interval  = 1;                                  // długi interwał wysyłki ramek w minutach default = 30
unsigned long beacon_fast_interval  = 1;                                  // krótki interwał wysyłki ramek w minutach default = 5
unsigned long gps_read_interval     = 5000;                               // msec default 1 sec
int tx_delay = 400;                                                       // opóznienie pomiedzy załączeniem tx a nadawaniem ramki
/*****************************************************************************************/

#include <ArduinoQAPRS.h>           // QAPRS Library: https://bitbucket.org/Qyon/arduinoqaprs/
#include <TinyGPS++.h>              // Tiny GPS Library: https://github.com/mikalhart/TinyGPSPlus
#include <SoftwareSerial.h>         // Software Serial: https://github.com/lstoll/arduino-libraries/tree/master/SoftwareSerial

// zmienne pomocnicze wewnętrzne oraz konfiguracja sprzętu
int gps_txd = 4;
int gps_rxd = 3;
int sql_port = 5;                                                         // pin SQL z dorji do sprawdzania zajętości kanału
int ptt_port = 2;                                                         // sterowanie nadawaniem
int voltage_input = A0;                                                   // pomiar napięcia 
int voltage = 0;                                                          // zmierzone napiecie
int tracker_status = 0;                                                   // status trakera kombinacja uruchomiony / brak fixa / praca
char * packet_buffer  = "                                                                         \n";
unsigned long time_to_send_data = 0;                                      // pomocnicza zmienna do wspóldzielenia czasu
unsigned long time_to_get_gps_data = 0;                                   // pomocnicza zmienna do współdzielenia czasu
unsigned long beacon_interval = beacon_fast_interval * 60000;             // pomocnicza zmienna domyślny timing wysyłki pakietów
unsigned long time_to_act_led = 0;                                        // pomocnicza zmienna czas do zmiany stanu leda

TinyGPSPlus gps;                                                          // inicjalizacja tiny gps
SoftwareSerial gpsSerial(gps_txd, gps_rxd);                               // soft serial for GPS

/*****************************************************************************************/

// ustawianie czasu wysyłki pakietów (do zmiany)
void set_packet_interval(){
 if(tracker_status < 2){
    beacon_interval = beacon_slow_interval * 60000;
 }else{
    beacon_interval = beacon_fast_interval * 60000;
 }  
}

// konwersja jednostek geograficznych do formatu APRS
// thanx to Stanley Seow https://github.com/stanleyseow/ArduinoTracker-MicroAPRS
float convertDegMin(float decDeg){
  float DegMin;
  int intDeg = decDeg;
  decDeg -= intDeg;
  decDeg *= 60;
  DegMin = ( intDeg*100 ) + decDeg;
 return DegMin; 
}

// obsługa GPS-a tutaj wyciągamy dane: prędkość, wysokość, koordynaty, ilość satelit, itd...
void make_data(){
  if(millis() >= time_to_get_gps_data){
    // pomiar napiecia
    int v_prefix = voltage / 10;
    int v_sufix = voltage % 10; 
    char lat_s[10];
    char lon_s[10];
    
    float latitude = gps.location.lat();
    float longtitu = gps.location.lng();
    // float speed_knots = gps.speed.knots();
    // float course_deg = gps.course.deg();
    int wysokosc = gps.altitude.meters();
    dtostrf(fabs(convertDegMin(latitude)),7,2,lat_s);
    dtostrf(fabs(convertDegMin(longtitu)),7,2,lon_s);

    //Serial.println("latitude");
    
    // zmiana jednostek N/S w zaleznosci od lokalizacji
    char n_s[2];
    if(convertDegMin(latitude) >= 0){
      n_s[0] = 'N';
    }else if(convertDegMin(latitude) < 0){
      n_s[0] = 'S';
    }
    n_s[1] = 0;
    
    // zmiana jednostek W/E w zależności od lokalizacji
    char w_e[2];
     if(convertDegMin(longtitu) >= 0){
      w_e[0] = 'E';
    }else if(convertDegMin(longtitu) < 0){
      w_e[0] = 'W';
    }
    w_e[1] = 0;
    
    // wyciągamy ilość satelit
    int sat_number = gps.satellites.value();
    
    // przygotowanie pakietu
    if(longtitu < 10){
      //sprintf(packet_buffer,"!%s%s%s00%s%s%s%s SAT=%01u U=%01u.%01uV Alt=%um",lat_s,n_s,symbol_table,lon_s,w_e,symbol,comment,sat_number,v_prefix,v_sufix,wysokosc);  
      sprintf(packet_buffer,"!%s%s%s00%s%s%s%s",lat_s,n_s,symbol_table,lon_s,w_e,symbol,comment);      
    }else if(longtitu >= 10 && longtitu < 100){
      //sprintf(packet_buffer,"!%s%s%s0%s%s%s%s SAT=%01u U=%01u.%01uV Alt=%um",lat_s,n_s,symbol_table,lon_s,w_e,symbol,comment,sat_number,v_prefix,v_sufix,wysokosc);
      sprintf(packet_buffer,"!%s%s%s0%s%s%s%s",lat_s,n_s,symbol_table,lon_s,w_e,symbol,comment);
    }else{
      //sprintf(packet_buffer,"!%s%s%s%s%s%%s sSAT=%01u U=%01u.%01uV Alt=%um",lat_s,n_s,symbol_table,lon_s,w_e,symbol,comment,sat_number,v_prefix,v_sufix,wysokosc);
      sprintf(packet_buffer,"!%s%s%s%s%s%%s",lat_s,n_s,symbol_table,lon_s,w_e,symbol,comment);
    }
    
    // flaga fixa nie jest zdejmowana po jego utracie trzeba coś z tym zrobić
    if (gps.location.isValid()){ 
      if(sat_number > 2){   
        tracker_status = 2;
      }else{
        tracker_status = 1;
        sprintf(packet_buffer,">U=%01u.%01uV NO FIX",v_prefix,v_sufix); 
      }
    }else{
      tracker_status = 1;
      sprintf(packet_buffer,">U=%01u.%01uV NO FIX",v_prefix,v_sufix);      
    }    
   time_to_get_gps_data = millis() + gps_read_interval;
   
   // zmiana czasu wysyłki po przeliczeniu danych z GPS-a
   // docelowo popracować nad smartbikonem
   set_packet_interval(); 
  }
}

// obsługa wysyłki danych
void send_aprs_packet(){
  if(millis() >= time_to_send_data){    
     digitalWrite(ptt_port,LOW);                        // ON PTT
     delay(tx_delay);                                   // wait for transceiver to be ready
     QAPRS.sendData(packet_buffer);                     // wysyłka pakietu
     Serial.println(packet_buffer);
     digitalWrite(ptt_port,HIGH);                       // OFF PTT
     delay(10);     
     time_to_send_data = millis() + beacon_interval; 
  }
}

/*****************************************************************************************/
// setup
void setup(){   
  // komunikacja z peryferiami
  Serial.begin(115200);                                         // serial
  gpsSerial.begin(9600);                                        // polaczenie do GPS
  delay(1000);                                                  //
  analogReference(INTERNAL);                                    // zmiana punktu odniesienia pomiaru napięć na 1.1V
  
  
  // konfiguracja, znak, ścieżka, SSID itd.
  QAPRS.init(0,0,callsign, '0', "APZQAP", '0', path);
  QAPRS.setFromAddress(callsign, ssid);                         //ustawiamy znak i SSID
  QAPRS.setRelays(path);                                        //ustawiamy ścieżkę  
  sprintf(packet_buffer,">NO FIX");                             //domyślny pakiet po starcie

  Serial.println("tracker ready...");
}

// pętla główna
void loop(){
  if(gpsSerial.available() > 0){
      //Serial.write(gpsSerial.read());
      if (gps.encode(gpsSerial.read())){
        make_data();            
      }    
  }

  // wysyłka pakietu
  send_aprs_packet();     
}
/*****************************************************************************************/
//E.O.F.
