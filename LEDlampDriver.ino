/*
 * STEROWNIK LAMPY LEDOWEJ
 * autor: Igor Sołdrzyński
 * igor.soldrzynski@gmail.com
 * Użycie całości lub fragmentu kodu za zgodą autora.
 * biblioteka RTClib https://github.com/adafruit/RTClib 
 */
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;


//Moce kanałów:
double maxBlue = 0.85;
double maxWhite = 0.82;
double maxUv = 0.9;


//Godziny start:
float gStartBlue = 8.8;
float gStartWhite = 9.5;
float gStartUv = 8.5;

//Godziny stop:
float gStopBlue = 21.2;
float gStopWhite = 20.5;
float gStopUv = 21.5;

//Ustawienie numerów pinów PWM:
int pwmBlue = 3;
int pwmWhite = 5;
int pwmUv = 6;

//Do testów:
float ustalCzas=-1;
float testTMP;



void setup()
{
  Serial.begin(9600);

  rtc.begin();
  // Ustawienie czasu w zegarze. Po ustawieniu zakomentować 2 poniższe linie i wgrać program jeszcze raz!
  // Należy ustawiać czas letni!
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // rtc.adjust(DateTime(2017, 12, 13, 13, 14, 0));

  //zmiana częstotliwości PWM, żeby zasilacz nie piszczał:
  setPwmFrequency(pwmWhite, 8);
  setPwmFrequency(pwmUv, 8);

  Serial.println("STEROWNIK LAMPY LEDOWEJ \nautor: Igor Sołdrzyński");

}

void loop()
{
  DateTime zegar = rtc.now();

  //suma godziny i minuty:
  float godzinoMinuta = (zegar.hour() * 100 + zegar.minute() * 100 / 60)/100.0;
  //czas zimowy:
  //jeżeli miesiąc jest od listopada do marca czyli 11, 12, 1, 2, 3 to minus jedna godzina, czyli, żeby załączał godzinę wcześniej:
  if(zegar.month() > 10 || zegar.month() < 4) godzinoMinuta = godzinoMinuta - 1;

    
  //---------------------_TESTY_---------------: 
  float test = Serial.parseFloat();
  if(testTMP==-1.0) test=1.0;
  else if(testTMP==-2.0) test=2.0;
  else if(testTMP==-3.0) test=3.0;
  else if(testTMP==-4.0) test=4.0;
  else if(testTMP==-5.0) test=5.0;
  


  //Ustalenie konkretnej godziny-niedokończone:
  if(test==1.0){
    Serial.println("Podaj godzinę do ustawienia (np. 16.5). -3 -wyjście");
    //czekanie aż używkownik wprowadzi godzinę:
    while(Serial.available()==0);
    ustalCzas=Serial.parseFloat();
    
    if(ustalCzas!=-3) testTMP=-1;
    else testTMP=0;
  }


  //Przebieg pełnej doby:
  if(test==2.0){
    if(ustalCzas==-1) ustalCzas=0;
    if(ustalCzas<24 && ustalCzas>=0){
      ustalCzas=ustalCzas+0.05;
      testTMP=-2.0;
    }
    else{
      testTMP=0;
      ustalCzas=-1.0;
    }
  }

  //Przebieg dnia:
  if(test==3.0){
    if(ustalCzas==-1) ustalCzas=min(min(gStartBlue, gStartWhite), gStartUv);
    if(ustalCzas<max(max(gStopBlue, gStopWhite), gStopUv) && ustalCzas>=0){
      ustalCzas=ustalCzas+0.01;
      testTMP=-3.0;
    }
    else{
      testTMP=0;
      ustalCzas=-1.0;
    }
  }

  //Moc max (godzina 15):
  if(test==4.0){
      ustalCzas=15;
      testTMP=-4;
  }

  //Test mozliwosci:
  if(test==5.0) testMozliwosci();

  //Test moce:
  if(test==6.0) testMocKanalu();


  
  if(ustalCzas>=0.0) {
    godzinoMinuta = ustalCzas;
  }
  //-----------------KONIEC TESTÓW---------------------------




    
  //wyliczenie PWM wg. aktualnej godziny:
  int mocBlue = obliczMocKanalu(godzinoMinuta, gStartBlue, gStopBlue, maxBlue);
  int mocWhite = obliczMocKanalu(godzinoMinuta, gStartWhite, gStopWhite, maxWhite); 
  int mocUv = obliczMocKanalu(godzinoMinuta, gStartUv, gStopUv, maxUv); 
  
  //ustawienie pwm:
  ustawKanaly(mocBlue, mocWhite, mocUv);

  //wywołanie funkcji wysyłającej dane na ekrak komputera:
  drukujDane(godzinoMinuta, mocBlue, mocWhite, mocUv, test);
    
  //opóżnienie w zależności czy wykonujemy test czy nie:
  if(testTMP<0) delay(500);
  else delay(20000);
}

//Test ręcznie moc kanałów:
void testMocKanalu(){
  int kanal=0;
  int moc=0;
  while(kanal!=-3){
    moc=0;
    Serial.println("Podaj kanał do ustawienia (1 -BLUE, 2 -WHITE, 3 -UV). -3 -wyjście");
    //czekanie aż używkownik wprowadzi dane:
    while(Serial.available()==0);
    kanal=Serial.parseFloat();
    while(moc!=-3){
      Serial.println("Podaj moc: (0-255)");
      //czekanie aż używkownik wprowadzi dane:
      while(Serial.available()==0);
      moc=Serial.parseFloat();
    
      if(kanal==1) ustawKanaly(moc, 0, 0);
      else if(kanal==2) ustawKanaly(0, moc, 0);
      else if(kanal==3) ustawKanaly(0, 0, moc);
    }
  }
}
//Test możliwośći:
void testMozliwosci(){
  int i;
  int wait=200;
  for(i=255; i>=0; i--){ ustawKanaly(0, 0, i); delay(wait);}   
  for(i=255; i>=0; i--){ ustawKanaly(i, 0, 0); delay(wait);}    
  for(i=255; i>=0; i--){ ustawKanaly(0, i, 0); delay(wait);} 
  
  for(i=255; i>=0; i--){ ustawKanaly(255-i, 0, i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(0, 255-i, i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(255-i, i, 0); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(255-i, 255-i, 255-i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(i, i, i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(i, i, 255-i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(255-i, i, i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(i, 255-i, i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(255-i, 255-i, i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(i, 255-i, 255-i); delay(wait);}
  for(i=255; i>=0; i--){ ustawKanaly(255-i, i, 255-i); delay(wait);}
  
}

//Funkcja ustwaiająca wyjścia pwm:
void ustawKanaly(int mocBlue, int mocWhite, int mocUv){
  //korekcja rozbłysków driverów:
  if(mocWhite>0 && mocWhite<26) mocWhite=26;
  if(mocBlue>0 && mocBlue<5) mocBlue=5;
  //ustawiamy pwm-y:
  analogWrite(pwmBlue, 255 - mocBlue);
  analogWrite(pwmWhite, 255 - mocWhite);
  analogWrite(pwmUv, 255 - mocUv);  
}


//Funkcja obliczająca moc w danej godzinie:
int obliczMocKanalu(float godzinoMinuta, float godzinaStart, float godzinaStop, float mocMax){
  int moc=0;
  if(godzinoMinuta>=godzinaStart && godzinoMinuta<=godzinaStop){
    int czasMap = map((int)(godzinoMinuta*100), (int)(godzinaStart*100), (int)(godzinaStop*100), 900, 2100);
    float czas = czasMap / 100.0;
    //0,0831x^3-6,406x^2+133,09x-740,4
   
    moc=(int)((0.0831149*pow(czas,3)-6.406366*pow(czas,2)+133.09292*czas-740.4333)*mocMax*255/100);
    if(moc<0) moc=0;
  }
  return moc;
}

//Funkcja wysyłająca na port szeregowy dane:
void drukujDane(float godzinoMinuta, int mocBlue, int mocWhite, int mocUv, float test){
    Serial.print("Godzinominuta: ");
    Serial.println(godzinoMinuta);
    Serial.print("Blue: ");
    Serial.print(mocBlue);
    Serial.print("(");
    Serial.print((int)(mocBlue/255.0*100.0));
    Serial.print("%)   White: ");
    Serial.print(mocWhite);
    Serial.print("(");
    Serial.print((int)(mocWhite/255.0*100.0));
    Serial.print("%)   UV: ");
    Serial.print(mocUv);
    Serial.print("(");
    Serial.print((int)(mocUv/255.0*100.0));
    Serial.println("%)");
    Serial.println("Testy: 1-określ godzinę, 2-przebieg doby, 3-przebieg dnia, 4-moc max, 5-możliwości, 6-ręcznie moc kanałów");
    Serial.print("Wybrany test: ");
    Serial.println(test);
    Serial.println("-------------------------");
}




//Funkcja zmieniająca częstotliwość PWM-ów:
/**
 * Divides a given PWM pin frequency by a divisor.
 * 
 * The resulting frequency is equal to the base frequency divided by
 * the given divisor:
 *   - Base frequencies:
 *      o The base frequency for pins 3, 9, 10, and 11 is 31250 Hz.
 *      o The base frequency for pins 5 and 6 is 62500 Hz.
 *   - Divisors:
 *      o The divisors available on pins 5, 6, 9 and 10 are: 1, 8, 64,
 *        256, and 1024.
 *      o The divisors available on pins 3 and 11 are: 1, 8, 32, 64,
 *        128, 256, and 1024.
 * 
 * PWM frequencies are tied together in pairs of pins. If one in a
 * pair is changed, the other is also changed to match:
 *   - Pins 5 and 6 are paired on timer0
 *   - Pins 9 and 10 are paired on timer1
 *   - Pins 3 and 11 are paired on timer2
 * 
 * Note that this function will have side effects on anything else
 * that uses timers:
 *   - Changes on pins 3, 5, 6, or 11 may cause the delay() and
 *     millis() functions to stop working. Other timing-related
 *     functions may also be affected.
 *   - Changes on pins 9 or 10 will cause the Servo library to function
 *     incorrectly.
 * 
 * Thanks to macegr of the Arduino forums for his documentation of the
 * PWM frequency divisors. His post can be viewed at:
 *   http://forum.arduino.cc/index.php?topic=16612#msg121031
 */
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x07; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
