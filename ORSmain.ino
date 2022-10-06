# include <SPI.h>
# include "printf.h"
# include "RF24.h"

//RF24
RF24 radio(7, 8);
uint8_t address[][6] = {"00001"};

int potValue = 0;
int PWMValue = 90;

//Moving Average Filter
#define WINDOW_SIZE 10
int INDEX = 0;
int VALUE = 0;
int SUM = 0;
int READINGS[WINDOW_SIZE];
int AVERAGED = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  
  adc_init();
  adc_gpio_init(26);
  adc_select_input(0);
  
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {
      }
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(PWMValue));
  radio.openWritingPipe(address[0]);
  radio.stopListening();
}

void loop() {
  Serial.println(F("Start the loop!"));

  //Daten aus ADC einlesen
  potValue = adc_read();

  //Moving Average Filter
  SUM = SUM - READINGS[INDEX];       // Remove the oldest entry from the sum
  READINGS[INDEX] = potValue;           // Add the newest reading to the window
  SUM = SUM + potValue;                 // Add the newest reading to the sum
  INDEX = (INDEX+1) % WINDOW_SIZE;   // Increment the index, and wrap to 0 if it exceeds the window size

  AVERAGED = SUM / WINDOW_SIZE;      // Divide the sum of the window by the window size for the result
  
  //ADC Wert auf PWM Bereich mappen
  PWMValue = map(AVERAGED, 10, 4095, 0, 180);
  
  radio.write(&PWMValue, sizeof(int));
  Serial.println(potValue);
  Serial.println(AVERAGED);
  Serial.println(PWMValue);
  //delay(10);
}
