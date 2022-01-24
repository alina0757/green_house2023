/*
**********************************************************************
  тестовый код для  проверки всех устройств "Умной теплицы" ЙоТик М2
  смотреть в мониторе порта
  Created by Maxim Kotov
  //********************************************************************
*/

#include <Adafruit_ADS1015.h>
#include <Wire.h>
#include <ESP32_Servo.h>                      // конфигурация сервомотора // servo configuration

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

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);    // конфигурация матрицы // matrix configuration

  LightSensor_1.begin();              // запуск датчика освещенности // turn the light intensity sensor on
  LightSensor_1.setMode(Continuously_High_Resolution_Mode);

  bool bme_status = bme280.begin();
  if (!bme_status)
    Serial.println("Could not find a valid BME280 sensor, check wiring!");  // проверка  датчика температуры, влажности и давления // checking the temp hum bar sensor

  if (!veml6075.begin())
    Serial.println("VEML6075 not found!");   // проверка работы датчика ультрафиолета  // checking the UV sensor

  ads.setGain(GAIN_TWOTHIRDS);
  ads.begin();    // включем АЦП // turn the ADC on
}

void loop()
{
  veml6075.poll();
  float uva = veml6075.getUVA();
  float uvb = veml6075.getUVB();
  float uv_index = veml6075.getUVIndex();
  Serial.println("UV-А = " + String(uva, 1) + " μW/cm2");
  Serial.println("UV-B = " + String(uvb, 1) + " μW/cm2");
  Serial.println("UV-index = " + String(uv_index, 1));
  delay(1512);
  float t = bme280.readTemperature();
  float h = bme280.readHumidity();
  float p = bme280.readPressure() / 100.0F;
  Serial.println("air temerature = " + String(t, 1) + " °C");
  Serial.println("air humidity = " + String(h, 1) + " %");
  Serial.println("pressure = " + String(p, 1) + " hPa");
  delay(1512);
  float l = LightSensor_1.getAmbientLight();
  Serial.println("light intensity = " + String(l, 1) + " lx");
  delay(1512);
  float adc0 = (float)ads.readADC_SingleEnded(0) * 6.144 * 16;
  float adc1 = (float)ads.readADC_SingleEnded(1) * 6.144 * 16;
  float t1 = (adc1 / 1000); //1023.0 * 5.0) - 0.5) * 100.0;
  float h1 = map(adc0, air_value, water_value, moisture_0, moisture_100);
  Serial.println("soil temperature = " + String(t1, 1) + " °C");
  Serial.println("soil humidity = " + String(h1, 1) + " %");
  delay(1512);
  myservo.write(10);
  Serial.println("servo position = 10°");
  delay(1512);
  myservo.write(170);
  Serial.println("servo position = 170°");
  delay(1512);
  fill_solid( leds, NUM_LEDS, CRGB(255, 0, 0));          // заполнить всю матрицу выбранным цветом
  FastLED.show();
  Serial.println("matrix is full of red");
  delay(1512);
  fill_solid( leds, NUM_LEDS, CRGB(0, 255, 0));          // заполнить всю матрицу выбранным цветом
  FastLED.show();
  Serial.println("matrix is full of green");
  delay(1512);
  fill_solid( leds, NUM_LEDS, CRGB(0, 0, 255));          // заполнить всю матрицу выбранным цветом
  FastLED.show();
  Serial.println("matrix is full of blue");
  delay(1512);
  digitalWrite(pump, HIGH);          // включить если нажата кнопка "Насос" // turn on the pump if button = 1
  digitalWrite(wind, HIGH);         // включить, если нажата кнопка "Вентилятор" // turn on the cooler if button = 1
  Serial.println("pump and cooler on");
  delay(1512);
  digitalWrite(wind, LOW);
  digitalWrite(pump, LOW);
  Serial.println("pump and cooler off");
  delay(1512);
}
