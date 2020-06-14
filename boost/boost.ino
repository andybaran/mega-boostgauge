/*
 * Pressure Sensor being used purchased from https://www.amazon.com/gp/product/B07L9S65B3/ref=ppx_yo_dt_b_asin_title_o06_s00?ie=UTF8&psc=1
 * Item model number  038906051C
 * Manufacturer Part Number  038906051C
 * OEM Part Number   0281002401, 038 906 051 C, 038906051C 
 * 
 * TODO : Figure out curve for temperature sensor and log air temp
 */

#include "SparkFunBME280.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // OLED_RESET pin is not hooked up

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);
float atmpressure;
BME280 mySensor;
float out_voltage;
float Pabs;
float inHgPabs;
int roundedPabs;
int mapsen = A0; // Set MAP sensor input on Analog port 0 
unsigned long previousMillis = 0;
const long interval = 250;
int min = 0;
int max = 0;

typedef union {
  int psiPabsInt;
  byte psiPabsByte[4];
} psiPabs;

psiPabs psiDifference;


void setup() { 
  //SeraSerial1.begin(115200);//, SERIAL_8N1, 16, 17);
  Serial.begin(115200);
  Wire.begin();

  //Verify BME280 is connected
  if (mySensor.beginI2C() == false) //Begin communication over I2C
  {
    Serial.println("BME280 did not respond. Please check wiring.");
    while(1); //Freeze
  }

  //Calibrate BME280 for "Game mode" according to honeyewll datasheet
  mySensor.setFilter(4);
  mySensor.setPressureOverSample(3);

  //Verify display is connected
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  //delay(2000);
} 
    
void loop() { 

  //non blocking delay using millis instead of delay()
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      atmpressure = mySensor.readFloatPressure() * 0.0001450377;
      out_voltage = analogRead(mapsen) * (5.0/1023);
  
      Pabs =  311.1111111 * (out_voltage/4.97) - 1.333333333;
  
      // pressure in PSI from Bosch MAP minus BME280 + .5 as that seemed to be a consistent difference, trusting Bosch as more accurate
      Pabs = round((Pabs / 6.894)) - round(atmpressure +.5); // Convert kPa to PSI and subtract ambient so we can calculate vacuum as well
      
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      
      //if value is less than atmostpheric pressure convert to inhg and display vacuum
      if (Pabs < 0) {
        inHgPabs = -1*(Pabs*2.036020657601236); //convert to inhg
        psiDifference.psiPabsInt = round(inHgPabs);
        display.print(psiDifference.psiPabsInt);
        display.print(" inHG\n");
      }
      
      else {
        psiDifference.psiPabsInt = round(Pabs);
        display.print(psiDifference.psiPabsInt);
        display.print(" PSI\n");
      }


      roundedPabs = int(round(Pabs));
      
      //update max and min
      if (roundedPabs > max) {max = roundedPabs;}
      if (roundedPabs < min) {min = roundedPabs;}
      
      display.print(String("\nmax: ") + String(max));
      display.print(String("\nmin: ") + String(min));
      display.display(); 

      //Comment these lines when car is connected and started
      //int rando = 11;//int(random(10,20));
      //psiDifference.psiPabsInt = psiDifference.psiPabsInt + rando;
      
      //Raspberry PI is expecting raw value with negatives being inHG
      psiDifference.psiPabsInt = roundedPabs;
      
      if (Serial.availableForWrite() > 0) {
        Serial.write(psiDifference.psiPabsByte,4);
      } else {
        Serial.println("The buffers are full");
      }
  }
}
