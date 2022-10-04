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

#include <ESP32_Servo.h>                      // конфигурация сервомотора 

#include <FastLED.h>                   // конфигурация матрицы  
#include <FastLED_GFX.h>
#include <FastLEDMatrix.h>

#define NUM_LEDS 64                    // количество светодиодов в матрице  
CRGB leds[NUM_LEDS];                   // определяем матрицу (FastLED библиотека) 
#define LED_PIN             18         // пин к которому подключена матрица 
#define COLOR_ORDER         GRB        // порядок цветов матрицы 
#define CHIPSET             WS2812     // тип светодиодов            

#define  pump   16                     // пин насоса              
#define  wind   17                     // пин вентилятора           

char ssid[] = "XXXX";                            // Логин Wi-Fi    
char pass[] = "XXXXXXXXXXXX";                    // Пароль от Wi-Fi    
char auth[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXxX"; // Токен 
IPAddress blynk_ip(139, 59, 206, 133);            // конфигурация блинка 

Servo myservo;                                                          
int pos = 1;            // начальная позиция сервомотора        
int prevangle = 1;      // предыдущий угол сервомотора    

// Выберите плату расширения вашей сборки (ненужные занесите в комментарии)
#define MGB_D1015 1
//#define MGB_P8 1

#include <BH1750FVI.h>        // библиотека датчика освещенности 
BH1750FVI LightSensor_1;      // MGS-L75                      

#include <Adafruit_Sensor.h>  // библиотека датчика температуры, влажности и давления 
#include <Adafruit_BME280.h>  // MGS-THP80                         
Adafruit_BME280 bme280;       //

#include "MCP3221.h"
#include "SparkFun_SGP30_Arduino_Library.h"
#include <VEML6075.h>         // библиотека датчика ультрафиолета  

#ifdef MGS_CO30
SGP30 mySensor;
#endif
#ifdef MGS_GUVA
const byte DEV_ADDR = 0x4F;  // 0x5С , 0x4D (также попробуйте просканировать адрес: https://github.com/MAKblC/Codes/tree/master/I2C%20scanner)
MCP3221 mcp3221(DEV_ADDR);
#endif
#ifdef MGS_UV60
VEML6075 veml6075;
#endif                         

#define UPDATE_TIMER 1000
BlynkTimer timer_update;      // настройка таймера для обновления данных с сервера BLynk 

#ifdef MGB_D1015
Adafruit_ADS1015 ads(0x48);
const float air_value    = 83900.0;
const float water_value  = 45000.0;
const float moisture_0   = 0.0;
const float moisture_100 = 100.0;  // настройка АЦП на плате расширения I2C MGB-D10 
#endif
#ifdef MGB_P8
#define SOIL_MOISTURE    34 // A6
#define SOIL_TEMPERATURE 35 // A7
const float air_value    = 1587.0;
const float water_value  = 800.0;
const float moisture_0   = 0.0;
const float moisture_100 = 100.0;
#endif
//////////////////////////////////////////НАСТРОЙКИ/////////////////////////////////////////////////////////////////
void setup()
{
  myservo.attach(19);             // пин сервомотора 

  pinMode( pump, OUTPUT );        
  pinMode( wind, OUTPUT );       // настройка пинов насоса и вентилятора на выход 
  digitalWrite(pump, LOW);       // насос и вентилятор изначально выключены
  digitalWrite(wind, LOW);

  Serial.begin(115200);
  delay(512);
  Blynk.begin(auth, ssid, pass, blynk_ip, 8442);         // подключение к серверу Blynk 

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);    // конфигурация матрицы 

  LightSensor_1.begin();              // запуск датчика освещенности  (0x23) или (0x5C)
  LightSensor_1.setMode(Continuously_High_Resolution_Mode);

  bool bme_status = bme280.begin(); // (0x76) или (0x77)
  if (!bme_status)
    Serial.println("Could not find a valid BME280 sensor, check wiring!");  // проверка  датчика температуры, влажности и давления 

 #ifdef MGS_UV60
  if (!veml6075.begin())
    Serial.println("VEML6075 not found!");
#endif
#ifdef MGS_GUVA
  mcp3221.setVinput(VOLTAGE_INPUT_5V);
#endif
#ifdef MGS_CO30
  if (mySensor.begin() == false) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }
  mySensor.initAirQuality();
#endif  

  timer_update.setInterval(UPDATE_TIMER, readSendData);  // включаем таймер обновления данных  

#ifdef MGB_D1015
  ads.setGain(GAIN_TWOTHIRDS);  
  ads.begin();    // включем АЦП 
#endif
}

//////////////////////////////////////////////////////ЧТЕНИЕ И ЗАПИСЬ ДАННЫХ ДАТЧИКОВ/////////////////////////////////////////////////
void readSendData() {
#ifdef MGS_UV60
  veml6075.poll();
  float uva = veml6075.getUVA();
  float uvb = veml6075.getUVB();
  float uv_index = veml6075.getUVIndex();
  Blynk.virtualWrite(V11, uva); delay(25);      // Отправка данных на сервер Blynk УФ-А  
  Blynk.virtualWrite(V12, uvb); delay(25);      // Отправка данных на сервер Blynk УФ-Б  
  Blynk.virtualWrite(V13, uv_index); delay(25); // Отправка данных на сервер Blynk УФ-И  
#endif
  float t = bme280.readTemperature();
  float h = bme280.readHumidity();
  float p = bme280.readPressure() / 100.0F;
  Blynk.virtualWrite(V14, t); delay(25);        // Отправка данных на сервер Blynk  Температура 
  Blynk.virtualWrite(V15, h); delay(25);        // Отправка данных на сервер Blynk  Влажность   
  Blynk.virtualWrite(V16, p); delay(25);        // Отправка данных на сервер Blynk  Давление
#ifdef MGS_GUVA
  float sensorVoltage;
  float sensorValue;
  float UV_index;
  sensorValue = mcp3221.getVoltage();
  sensorVoltage = 1000 * (sensorValue / 4096 * 5.0); // напряжение на АЦП
  UV_index = 370 * sensorVoltage / 200000; // Индекс УФ (эмпирическое измерение)
  Blynk.virtualWrite(V14,  sensorVoltage); delay(25);       
  Blynk.virtualWrite(V15,  UV_index); delay(25);  
#endif
#ifdef MGS_CO30
  mySensor.measureAirQuality();
  Blynk.virtualWrite(V14,  mySensor.CO2); delay(25);       
  Blynk.virtualWrite(V15,  mySensor.TVOC); delay(25);  
#endif
  
  float l = LightSensor_1.getAmbientLight();
  Blynk.virtualWrite(V17, l); delay(25);        // Отправка данных на сервер Blynk  Освещенность 
#ifdef MGB_D1015
  float adc0 = (float)ads.readADC_SingleEnded(0) * 6.144 * 16;  
  float adc1 = (float)ads.readADC_SingleEnded(1) * 6.144 * 16;
  float t1 = ((adc1 / 1000));
  float h1 = map(adc0, air_value, water_value, moisture_0, moisture_100); // преобразование данных в диапазон 0-100
  Blynk.virtualWrite(V7, t1); delay(25);        // Отправка данных на сервер Blynk  Температура почвы  
  Blynk.virtualWrite(V9, h1); delay(25);        // Отправка данных на сервер Blynk  Влажность почвы  
#endif
#ifdef MGB_P8
  float adc0 = analogRead(SOIL_MOISTURE);
  float adc1 = analogRead(SOIL_TEMPERATURE);
  float t1 = ((adc1 / 4095.0 * 5.0) - 0.3) * 100.0; // АЦП разрядность (12) = 4095
  float h1 = map(adc0, air_value, water_value, moisture_0, moisture_100);
  Blynk.virtualWrite(V7, t1); delay(25);        // Отправка данных на сервер Blynk  Температура почвы  
  Blynk.virtualWrite(V9, h1); delay(25);        // Отправка данных на сервер Blynk  Влажность почвы  
#endif
}
/////////////////////////////////////////////////ГЛАВНЫЙ ЦИКЛ//////////////////////////////////////////////////////////////////////////////
void loop()
{
  Blynk.run();                                          // запуск Blynk  
  timer_update.run();
}
////////////////////////////////////////////////ОКНА///////////////////////////////////////////////////////////////////////////////////
BLYNK_WRITE(V0)
{
  int angle = param.asInt();
  if (prevangle < angle) {
    for (pos = prevangle; pos <= angle; pos += 1)
    {
      myservo.write(pos);
      delay(15);                                        // если угол задан больше предыдущего, то доводим до нужного угла в ++
    }
    prevangle = angle;
  }
  else if (prevangle > angle) {
    for (pos = prevangle; pos >= angle; pos -= 1)
    {
      myservo.write(pos);
      delay(15);                                       // если угол задан меньше предыдущего, то доводим до нужного угла в -- 
    }
    prevangle = angle;
  }
}
////////////////////////////////////////////////ЦВЕТ//////////////////////////////////////////////////////////////////////////////
BLYNK_WRITE(V2)
{
  int r = param[0].asInt();                            // установка  R  
  int g = param[1].asInt();                            //            G
  int b = param[2].asInt();                            // параметров B  
  fill_solid( leds, NUM_LEDS, CRGB(r, g, b));          // заполнить всю матрицу выбранным цветом
  FastLED.show();
}
//////////////////////////////////////////////ЯРКОСТЬ///////////////////////////////////////////////////////////////////////////
BLYNK_WRITE(V4)
{
  int power = param.asInt();
  FastLED.setBrightness(power);                     // устанавливаем яркость по значению слайдера (0-255) 
  FastLED.show();                                   // отобразить полученные данные 
}
//////////////////////////////////////////////НАСОС/////////////////////////////////////////////////////////////
BLYNK_WRITE(V5)
{
  int buttonstate1 = param.asInt ();
  if (buttonstate1 == 1) {
    digitalWrite(pump, HIGH);          // включить если нажата кнопка "Насос" 
  }
  else    {
    digitalWrite(pump, LOW);
  }
}
//////////////////////////////////////////ВЕНТИЛЯТОР/////////////////////////////////////////////////////////
BLYNK_WRITE(V6)
{
  int buttonstate2 = param.asInt ();
  if (buttonstate2 == 1) {
    digitalWrite(wind, HIGH);         // включить, если нажата кнопка "Вентилятор" 
  }
  else    {
    digitalWrite(wind, LOW);
  }
}
