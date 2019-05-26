#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
/*
 * STEROWNIK LAMPY LEDOWEJ
 * autor: Igor Sołdrzyński
 * igor.soldrzynski@gmail.com
 * Użycie całości lub fragmentu kodu tylko za zgodą autora.
 * biblioteka RTClib https://github.com/adafruit/RTClib 
 */

//Moce kanałów:
const float maxWhite = 0.95;
const float maxBlue = 0.80;
const float maxUv = 0.90;

//Godziny start (dziesiętnie):
const float gStartWhite = 9.5;
const float gStartBlue = 9.0;
const float gStartUv = 8.5;

//Godziny stop (dziesiętnie):
const float gStopWhite = 19.5;
const float gStopBlue = 21.5;
const float gStopUv = 20.5;

//Ustawienie numerów pinów PWM:
const int pinWhite = 9;
const int pinBlue = 3;
const int pinUv = 5;
//zamienić na typ byte...

//Piny czujników i wyjść:
const byte pinTemSens = 10;
const byte pinWtLv = 2;
const byte pinOutWent = 13;
const byte pinOutHeat = 12; 
const byte pinOutPump = 7;
//dodatkowa dioda (wolna) pin 6

//Piny softwareserial (bluetooth) - A0, A1
SoftwareSerial softSerial(A0, A1);

//Obiekty niezbędna do połączenia z termometrem DS18B20 oraz zegarem RTC:
OneWire bus(pinTemSens);
DallasTemperature sensors(&bus);
DeviceAddress sensor;
RTC_DS3231 rtc;

//Ustawienia zadanej temperatury:
const float tarTemp = 25.0;
const float flucTemp = 0.5;

//Pomiary ATO:
const byte nMeasMax = 3; //liczba pomiarów upewniających się o stanie
byte numMeas = nMeasMax;
byte curSecFi = 0;
boolean lastMeas = 0;

//poprzednie godzinoMinuta do odstępów między pomiarami:
float p2GodzinoMinuta = 0.0;


//----------------------------------------KLASA KanalLED---------------------------
// Klasa, której instancje są poszczególnymi kanałami LED
class KanalLED
{
  public:
    KanalLED(int pin, float gStart, float gStop, float moc);
    void ustawKanal(float godzinominuta);
    bool jestRowny(KanalLED innyKanal);
    int getPin();
    float getMoc();
    float getGstart();
    float getGstop();
    void setPin(int newPin);
    void setMoc(float newMoc);
    void setGstart(float newGstart);
    void setGstop(float newGstop);
    
  private:
    int _pin;
    float _gStart;
    float _gStop;
    float _moc;
    float _godzinominuta;
    
};
//konstruktor
KanalLED::KanalLED(int pin, float gStart, float gStop, float moc)
{
  //int _pin = pin;
  //float _gStart = gStart;
  //float _gStop = gStop;
  //float _moc = moc;
  setPin(pin);
  setGstart(gStart);
  setGstop(gStop);
  setMoc(moc);
}
//metoda ustawiająca aktualną moc kanału na podstawie godzinominuty
void KanalLED::ustawKanal(float godzinominuta)
{
  _godzinominuta = godzinominuta;
  
  //oblicznie aktualnej mocy
  int aktualnaMoc=0;
  if(_godzinominuta>=_gStart && _godzinominuta<=_gStop)
  {
    int czasMap = map((int)(_godzinominuta*100), (int)(_gStart*100), (int)(_gStop*100), 900, 2100);
    float czas = czasMap / 100.0;
    //aktualnaMoc = 0,0831x^3-6,406x^2+133,09x-740,4 ; gdzie x = aktualny czas  
    aktualnaMoc=(int)((0.0831149*pow(czas,3)-6.406366*pow(czas,2)+133.09292*czas-740.4333)*_moc*255/100);
    if(aktualnaMoc<0) aktualnaMoc=0;
  }

  //korekcja rozbłysków driverów
  if(aktualnaMoc>0 && aktualnaMoc<26) aktualnaMoc=15;
  //ustawianie pwm
  analogWrite(_pin, 255 - aktualnaMoc);
}
//metoda sprawdzająca czy obiekt kanał jest identyczny z innym obiektem kanał
bool KanalLED::jestRowny(KanalLED innyKanal)
{
  if(_pin==innyKanal.getPin() && _gStart==innyKanal.getGstart() && _gStop==innyKanal.getGstop() && _moc==innyKanal.getMoc())
  {
    return true;
  }
  else
  {
    return false;
  }
}

//---------------------------------------Setery------------------------------------
//metoda zwracająca nr pin kanału
void KanalLED::setPin(int newPin){ _pin = newPin; }
//metoda zwracająca moc (mnożnik) kanału
void KanalLED::setMoc(float newMoc){ _moc = newMoc; }
//metoda zwracająca godzinę włączenia kanału
void KanalLED::setGstart(float newGstart){ _gStart = newGstart; }
//metoda zwracająca godzinę wyłączenia kanału
void KanalLED::setGstop(float newGstop){ _gStop = newGstop; }
//---------------------------------------Setery koniec-----------------------------

//---------------------------------------Getery------------------------------------
//metoda ustawiająca nr pin kanału
int KanalLED::getPin(){ return _pin; }
//metoda ustawiająca moc (mnożnik) kanału
float KanalLED::getMoc(){ return _moc; }
//metoda ustawiająca godzinę włączenia kanału
float KanalLED::getGstart(){ return _gStart; }
//metoda ustawiająca godzinę wyłączenia kanału
float KanalLED::getGstop(){ return _gStop; }
//---------------------------------------Getery koniec-----------------------------

//-----------------------------------KONIEC klasy KanalLED-------------------------


//powołanie kanałów z wartościami początkowymi
KanalLED white(pinWhite, gStartWhite, gStopWhite, maxWhite);
KanalLED blue(pinBlue, gStartBlue, gStopBlue, maxBlue);
KanalLED uv(pinUv, gStartUv, gStopUv, maxUv);


//--------------------------------------- SETUP ------------------------------------
void setup()
{
  Serial.begin(9600);
  softSerial.begin(9600);
  rtc.begin();
  // Ustawienie czasu w zegarze. Po ustawieniu zakomentować poniższe linie kodu i wgrać program jeszcze raz!
  // Należy ustawiać czas letni!
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //systemowy
  // rtc.adjust(DateTime(2017, 12, 13, 13, 14, 0)); //lub ręcznie ustawiony

  // Start sensora temperatury DS18B20:
  sensors.begin();
  sensors.getAddress(sensor, 0);

  // Ustawienie pinów wyjścia/wejścia:
  pinMode(pinWtLv, INPUT_PULLUP);
  pinMode(pinOutHeat, OUTPUT);
  pinMode(pinOutWent, OUTPUT);
  pinMode(pinOutPump, OUTPUT);
  digitalWrite(pinOutHeat, LOW);
  digitalWrite(pinOutWent, LOW);
  digitalWrite(pinOutPump, LOW);

  //przywrócenie zapisanych ustawień w EEPROM
  if(EEPROM.read(0) != 0)
  {
    EEPROM.get(0, white);
    EEPROM.get(sizeof(white), blue);
    EEPROM.get(sizeof(white)+sizeof(blue), uv);
  }
}


//--------------------------------------- LOOP --------------------------------------
void loop()
{
  //--------------------------------Godzinominuta obliczanie-------------------------
  DateTime zegar = rtc.now();
  //suma godziny i minuty:
  float godzinoMinuta = (zegar.hour() * 100 + zegar.minute() * 100 / 60)/100.0; 
  //czas zimowy (tak mniej więcej):
  //jeżeli miesiąc jest od listopada do marca czyli 11, 12, 1, 2, 3 to minus jedna godzina, czyli, żeby załączał godzinę wcześniej:
  if(zegar.month() > 10 || zegar.month() < 4) godzinoMinuta = godzinoMinuta - 1;
  //--------------------------------Godzinominuta koniec-----------------------------

  //ustawianie kanałów
  blue.ustawKanal(godzinoMinuta);
  uv.ustawKanal(godzinoMinuta);
  white.ustawKanal(godzinoMinuta);


  //sprawdzenie i obsługa żądań aplikacji zewnętrznej
  String zadanie;
  while(softSerial.available()) 
  {
    zadanie = softSerial.readString(); //odebrany ciąg

    //obsługa wysyłania ustawień
    if(zadanie.startsWith("pobierz"))
    {
      wyslijUstawienia();      
    }
    //obsługa odbierania nowych ustawień
    else if(zadanie.startsWith("wyslij"))
    {
      odbierzUstawienia(zadanie);
    }
    //obsługa zapisu ustawień w EEPROM
    else if(zadanie.startsWith("zapisz"))
    {
      zapiszUstawienia();
    }
    //obsługa godziny szczytu na 5min
    else if(zadanie.startsWith("szczyt"))
    {
      godzinaSzczytu();
    }
  }


  //sprawdzenie poziomu wody i uruchomienie pompy w razie potrzeby:
  ato(zegar.second());

  
  //sprawdzenie temperatury co 3 minuty:
  //if((godzinoMinuta - p2GodzinoMinuta) > 0.05)
  //{
    //sprawdzenie temperatury i ustawienie grzania/chłodzenia:
    tempControl(curTemp());
    //p2GodzinoMinuta = godzinoMinuta;
  //}

  delay(50);
}


//funkcja wysyłająca aktualne ustawienia obiektów (kanałów)
void wyslijUstawienia()
{
  String ustawienia = 
    liczbaZnakowFloatToString(white.getMoc(),4) +";"+ 
    liczbaZnakowFloatToString(blue.getMoc(),4) +";"+
    liczbaZnakowFloatToString(uv.getMoc(),4) +";"+
    liczbaZnakowFloatToString(white.getGstart(),4) +";"+ 
    liczbaZnakowFloatToString(blue.getGstart(),4) +";"+
    liczbaZnakowFloatToString(uv.getGstart(),4) +";"+
    liczbaZnakowFloatToString(white.getGstop(),4) +";"+ 
    liczbaZnakowFloatToString(blue.getGstop(),4) +";"+
    liczbaZnakowFloatToString(uv.getGstop(),4) +";";
  softSerial.println(ustawienia);
}


//funkcja zapisująca aktualne ustawienia w EEPROM
void zapiszUstawienia()
{
  //porównanie przed zapisem, aby oszczędzać ograniczoną liczbę zapisów na EEPROM
  KanalLED porownanie(0,0,0,0);

  EEPROM.get(0, porownanie);
  if(!porownanie.jestRowny(white))
  {
    EEPROM.put(0, white);
  }

  EEPROM.get(sizeof(white), porownanie);
  if(!porownanie.jestRowny(blue))
  {
    EEPROM.put(sizeof(white), blue);
  }

  EEPROM.get(sizeof(white)+sizeof(blue), porownanie);
  if(!porownanie.jestRowny(uv))
  {
    EEPROM.put(sizeof(white)+sizeof(blue), uv);
  }
}


//funkcja odbierająca aktualne ustawienia obiektów (kanałów)
void odbierzUstawienia(String zadanie)
{
  //format wysyłany przez aplikację: 
  //wyslij0.66;0.85;0.90;9.50;8.80;8.50;20.5;21.2;21.5;
  zadanie = zadanie.substring(6);

  white.setMoc(zadanie.substring(5*0,5*0+4).toFloat());
  white.setGstart(zadanie.substring(15+5*0,15+5*0+4).toFloat());
  white.setGstop(zadanie.substring(30+5*0,30+5*0+4).toFloat());

  blue.setMoc(zadanie.substring(5*1,5*1+4).toFloat());
  blue.setGstart(zadanie.substring(15+5*1,15+5*1+4).toFloat());
  blue.setGstop(zadanie.substring(30+5*1,30+5*1+4).toFloat());

  uv.setMoc(zadanie.substring(5*2,5*2+4).toFloat());
  uv.setGstart(zadanie.substring(15+5*2,15+5*2+4).toFloat());
  uv.setGstop(zadanie.substring(30+5*2,30+5*2+4).toFloat());
}

//funkcja ustawiająca na 5 minut godzinę szczytu
void godzinaSzczytu()
{
  float szczyt;
  szczyt = ((white.getGstart()+white.getGstop())/2 +
            (blue.getGstart()+blue.getGstop())/2 +
            (uv.getGstart()+uv.getGstop())/2)/3;
  white.ustawKanal(szczyt);
  blue.ustawKanal(szczyt);
  uv.ustawKanal(szczyt);

  unsigned long czas;
  czas = millis();
  //czekaj 5 minut i nic nie rób
  while(millis() - czas < 5*60*1000)
  {
     delay(50);
     if(softSerial.readString().startsWith("szczyt"))
     {
      delay(100);
      break;
     }
  }
}


//funkcja zaokrągla float do String o określonej liczbie znaków (np 1.524 = 1.52)
String liczbaZnakowFloatToString(float przecinkowa, int znakow)
{
  String wynik;
  wynik = String(przecinkowa);
  while(wynik.length() != znakow)
  {
    if(wynik.length() > znakow)
    {
      wynik.remove(wynik.length()-1);
    }
    else
    {
      wynik+=0;
    }
  }
  return wynik;
}

//funkcja zwracająca aktualną temperaturę:
float curTemp()
{
  sensors.requestTemperatures();
  return(sensors.getTempC(sensor));
}

//funckja załączająca grzanie/chłodzenie w razie potrzeby:
void tempControl(float temp)
{
  if(temp < (tarTemp - flucTemp))
  {
    digitalWrite(pinOutHeat, HIGH);
    digitalWrite(pinOutWent, LOW);
  }
  else if(temp > (tarTemp + flucTemp))
  {
    digitalWrite(pinOutHeat, LOW);
    digitalWrite(pinOutWent, HIGH);
  }
  else
  {
    digitalWrite(pinOutHeat, LOW);
    digitalWrite(pinOutWent, LOW);
  }
}

//funkcja włączająca pompę dolewki w razie potrzeby:
void ato(byte curSec)
{
  if(curSecFi != curSec)
  {
    curSecFi = curSec;
    if(lastMeas != digitalRead(pinWtLv))
    {
      lastMeas = digitalRead(pinWtLv);
      numMeas = nMeasMax;
    }
    numMeas -= 1;

    if(numMeas == 0)
    {
      if(lastMeas)
      {
        digitalWrite(pinOutPump, LOW);
        Serial.println("ATO PUMP OFF");
      }
      else
      {
        digitalWrite(pinOutPump, HIGH);
        Serial.println("ATO PUMP ON");
      }
      numMeas = nMeasMax;
    }
  }
}
