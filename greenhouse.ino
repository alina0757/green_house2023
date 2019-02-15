//************************\\//**************************\\
//**Greenhouse IoT Blynk**\\//**Created by Maxim Kotov**\\
//************************\\//**************************\\
////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_ADS1015.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <SPI.h>                       // конфигурация блинка // Blynk configuration 

#include <Servo.h>                     // конфигурация сервомотора // servo configuration

#include <FastLED.h>                   // конфигурация матрицы // LED matrix configuration   
#include <FastLED_GFX.h>
#include <FastLEDMatrix.h>
#define NUM_LEDS 64                    // количество светодиодов в матрице // number of LEDs 
CRGB leds[NUM_LEDS];                   // определяем матрицу (FastLED библиотека) // defining the matrix (fastLED library)
#define LED_PIN             18         // пин к которому подключена матрица // matrix pin
#define COLOR_ORDER         GRB        // порядок цветов матрицы // color order 
#define CHIPSET             WS2812     // тип светодиодов // LED type            

#define  pump   16                     // пин насоса // pump pin             
#define  wind   17                     // пин вентилятора // cooler pin          

char ssid[] = "MGBot";                            // Логин Wi-Fi  // Wi-Fi login   
char pass[] = "Terminator812";                    // Пароль от Wi-Fi // Wi-Fi password   
char auth[] = "4a7c974100bf4153a9f301c0854988fd"; // Токен // Authorization token
IPAddress blynk_ip(139, 59, 206, 133);            // конфигурация блинка // Blynk configuration

Servo myservo;                                                          
int pos = 1;            // начальная позиция сервомотора // servo start position       
int prevangle = 1;      // предыдущий угол сервомотора // previous angle of servo       

#include <BH1750FVI.h>        // добавляем библиотеку датчика освещенности // adding Light intensity sensor library  
BH1750FVI LightSensor_1;      // BH1750                      

#include <Adafruit_Sensor.h>  // добавляем библиотеку датчика температуры, влажности и давления // adding Temp Hum Bar sensor library
#include <Adafruit_BME280.h>  // BME280                         
Adafruit_BME280 bme280;       //

#include <VEML6075.h>         // добавляем библиотеку датчика ультрафиолета // adding Ultraviolet sensor library        
VEML6075 veml6075;            // VEML6075                          

#define UPDATE_TIMER 1000
BlynkTimer timer_update;      // настройка таймера для обновления данных с сервера BLynk // Blynk update timer configuration

Adafruit_ADS1015 ads(0x48);
const float air_value    = 83900.0;
const float water_value  = 45000.0;
const float moisture_0   = 0.0;
const float moisture_100 = 100.0;  // настройка АЦП на плате расширения I2C MGB-D10 // I2C MGB-D10 ADC configuration

//////////////////////////////////////////НАСТРОЙКИ/CONFIGURATION/////////////////////////////////////////////////////////////////
void setup()
{
  myservo.attach(19);             // пин сервомотора // servo pin

  pinMode( pump, OUTPUT );        
  pinMode( wind, OUTPUT );       // настройка пинов насоса и вентилятора на выход // pump and cooler pins configured on output mode
  digitalWrite(pump, LOW);       // устанавливаем насос и вентилятор изначально выключенными // turn cooler and pump off
  digitalWrite(wind, LOW);

  Serial.begin(115200);
  delay(512);
  Blynk.begin(auth, ssid, pass, blynk_ip, 8442);         // подключение к серверу Blynk // connection to Blynk server

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);    // конфигурация матрицы // matrix configuration

  LightSensor_1.begin();              // запуск датчика освещенности // turn the light intensity sensor on
  LightSensor_1.setMode(Continuously_High_Resolution_Mode);

  bool bme_status = bme280.begin();
  if (!bme_status)
    Serial.println("Could not find a valid BME280 sensor, check wiring!");  // проверка  датчика температуры, влажности и давления // checking the temp hum bar sensor

  if (!veml6075.begin())
    Serial.println("VEML6075 not found!");   // проверка работы датчика ультрафиолета  // checking the UV sensor

  timer_update.setInterval(UPDATE_TIMER, readSendData);  // включаем таймер обновления данных  // turn on the update timer

  ads.setGain(GAIN_TWOTHIRDS);  
  ads.begin();    // включем АЦП // turn the ADC on
}

//////////////////////////////////////////////////////ЧТЕНИЕ И ЗАПИСЬ ДАННЫХ ДАТЧИКОВ/SENSOR DATA SEND/READ////////////////////////////////////////////////
void readSendData() {

  veml6075.poll();
  float uva = veml6075.getUVA();
  float uvb = veml6075.getUVB();
  float uv_index = veml6075.getUVIndex();
  Blynk.virtualWrite(V11, uva); delay(25);      // Отправка данных на сервер Blynk УФ-А  // Uv-A data send
  Blynk.virtualWrite(V12, uvb); delay(25);      // Отправка данных на сервер Blynk УФ-Б  // Uv-B data send
  Blynk.virtualWrite(V13, uv_index); delay(25); // Отправка данных на сервер Blynk УФ-И  // Uv-Index data send

  float t = bme280.readTemperature();
  float h = bme280.readHumidity();
  float p = bme280.readPressure() / 100.0F;
  Blynk.virtualWrite(V14, t); delay(25);        // Отправка данных на сервер Blynk  Температура // Temperature data send
  Blynk.virtualWrite(V15, h); delay(25);        // Отправка данных на сервер Blynk  Влажность   // Humidity data send
  Blynk.virtualWrite(V16, p); delay(25);        // Отправка данных на сервер Blynk  Давление    // Pressure data send

  float l = LightSensor_1.getAmbientLight();
  Blynk.virtualWrite(V17, l); delay(25);        // Отправка данных на сервер Blynk  Освещенность // Light intensity data send

  float adc0 = (float)ads.readADC_SingleEnded(0) * 6.144 * 16;
  float adc1 = (float)ads.readADC_SingleEnded(1) * 6.144 * 16;

  float t1 = ((adc1 / 1000));
  float h1 = map(adc0, air_value, water_value, moisture_0, moisture_100);

  Blynk.virtualWrite(V7, t1); delay(25);        // Отправка данных на сервер Blynk  Температура почвы  // Soil temperature data send
  Blynk.virtualWrite(V9, h1); delay(25);        // Отправка данных на сервер Blynk  Влажность почвы    // Soil humidity data send

}
/////////////////////////////////////////////////ГЛАВНЫЙ ЦИКЛ/MAIN LOOP///////////////////////////////////////////////////////////////////////////////
void loop()
{
  Blynk.run();                                          // запуск Blynk  // turn Blynk on
  timer_update.run();
}
////////////////////////////////////////////////ОКНА/WINDOWS///////////////////////////////////////////////////////////////////////////////////
BLYNK_WRITE(V0)
{
  int angle = param.asInt();
  if (prevangle < angle) {
    for (pos = prevangle; pos <= angle; pos += 1)
    {
      myservo.write(pos);
      delay(15);                                        // если угол задан больше предыдущего, то доводим до нужного угла в ++ // if the current angle>previous angle then going clockwise
    }
    prevangle = angle;
  }
  else if (prevangle > angle) {
    for (pos = prevangle; pos >= angle; pos -= 1)
    {
      myservo.write(pos);
      delay(15);                                       // если угол задан меньше предыдущего, то доводим до нужного угла в -- // if the current angle<previous angle then going counter-clockwise
    }
    prevangle = angle;
  }
}
////////////////////////////////////////////////ЦВЕТ/COLOUR/////////////////////////////////////////////////////////////////////////////////
BLYNK_WRITE(V2)
{
  int r = param[0].asInt();                            // установка  R  set up
  int g = param[1].asInt();                            //            G
  int b = param[2].asInt();                            // параметров B  parameters
  fill_solid( leds, NUM_LEDS, CRGB(r, g, b));          // заполнить всю матрицу выбранным цветом
  FastLED.show();
}
//////////////////////////////////////////////ЯРКОСТЬ/BRIGHTNESS///////////////////////////////////////////////////////////////////////////////
BLYNK_WRITE(V4)
{
  int power = param.asInt();
  FastLED.setBrightness(power);                     // устанавливаем яркость по значению слайдера (0-255) /setting the brightness (0-255)
  FastLED.show();                                   // отобразить полученные данные // show the result
}
//////////////////////////////////////////////НАСОС/PUMP///////////////////////////////////////////////////////////////
BLYNK_WRITE(V5)
{
  int buttonstate1 = param.asInt ();
  if (buttonstate1 == 1) {
    digitalWrite(pump, HIGH);          // включить если нажата кнопка "Насос" // turn on the pump if button = 1 
  }
  else    {
    digitalWrite(pump, LOW);
  }
}
//////////////////////////////////////////ВЕНТИЛЯТОР/COOLER/////////////////////////////////////////////////////////////
BLYNK_WRITE(V6)
{
  int buttonstate2 = param.asInt ();
  if (buttonstate2 == 1) {
    digitalWrite(wind, HIGH);         // включить, если нажата кнопка "Вентилятор" // turn on the cooler if button = 1 
  }
  else    {
    digitalWrite(wind, LOW);
  }
}
