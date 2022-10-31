//######################################
//########## OpenRemoteSender ##########
//######################################

//########## librarys ##########
# include "SPI.h"
# include "printf.h"
# include "RF24.h"
# include "MCP3XXX.h"

//########## objects, arrays, variabeles ##########
bool blink = LOW;

//RF24
RF24 radio(20, 17);
uint8_t address[][6] = {"00001"};

//MCP3008
MCP3008 adc1;
MCP3008 adc2;

//array for control data
//channelInputData, channelLowerLimit, channelUpperLimit, channelZeroPoint, channelDeadZone, channelFiltered
int controlData[16][6] = {{0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0},
                          {0,0,1023,511,0,0}};

//array for channel data
//servoLowerLimit, servoUpperLimit, servoZeroPoint, servoInverse
int channelData[16][4] = {{0,180,90,0},
                          {60,120,90,1},
                          {35,130,70,1},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0},
                          {0,180,90,0}};
struct ServoData {
  byte sD0=0;
  byte sD1=0;
  byte sD2=0;
  byte sD3=0;
  byte sD4=0;
  byte sD5=0;
  byte sD6=0;
  byte sD7=0;
  byte sD8=0;
  byte sD9=0;
  byte sD10=0;
  byte sD11=0;
  byte sD12=0;
  byte sD13=0;
  byte sD14=0;
  byte sD15=0;
};
ServoData servoData;

//array for moving average filter
#define windowSize 5
int mafData[10][windowSize+2] = {};  // index (0), sum (1), readings[windowSize] (3-windowSize-1)


//########## methods ##########

int analogLinear(int inputChannel) {
  switch(channelData[inputChannel][3]) {
    case 0:
      if(controlData[inputChannel][5] < (controlData[inputChannel][3]-controlData[inputChannel][4])) {
        return map(controlData[inputChannel][5], controlData[inputChannel][1], (controlData[inputChannel][3]-controlData[inputChannel][4]), channelData[inputChannel][0], (channelData[inputChannel][2]-1));
      }else if(controlData[inputChannel][5] > (controlData[inputChannel][3]+controlData[inputChannel][4])) {
        return map(controlData[inputChannel][5], (controlData[inputChannel][3]+controlData[inputChannel][4]), controlData[inputChannel][2], (channelData[inputChannel][2]-1), channelData[inputChannel][1]);
      }else {
        return channelData[inputChannel][2];
      }
      break;
    case 1:
      if(controlData[inputChannel][5] < (controlData[inputChannel][3]-controlData[inputChannel][4])) {
        return map(controlData[inputChannel][5], controlData[inputChannel][1], (controlData[inputChannel][3]-controlData[inputChannel][4]), channelData[inputChannel][1], (channelData[inputChannel][2]+1));
      }else if(controlData[inputChannel][5] > (controlData[inputChannel][3]+controlData[inputChannel][4])) {
        return map(controlData[inputChannel][5], (controlData[inputChannel][3]+controlData[inputChannel][4]), controlData[inputChannel][2], (channelData[inputChannel][2]-1), channelData[inputChannel][0]);
      }else {
        return channelData[inputChannel][2];
      }
      break;
    default:
      Serial.print("Falscher Wert Servoinvertierung Kanal: ");
      Serial.println(inputChannel);
  }
}

int digital2Way(int referenceChannel, int inputChannel) {
  switch(channelData[inputChannel][3]) {
    case 0:
      if(digitalRead(inputChannel) == HIGH) {
        return channelData[referenceChannel][1];
      }else {
        return channelData[referenceChannel][0];
      }
      break;
    case 1:
      if(digitalRead(inputChannel) == HIGH) {
        return channelData[referenceChannel][0];
      }else {
        return channelData[referenceChannel][1];
      }
      break;
    default:
      Serial.print("Falscher Wert Servoinvertierung Kanal: ");
      Serial.println(referenceChannel);
  }
}

int digital3Way(int referenceChannel, int inputChannel) {
  switch(channelData[inputChannel][3]) {
    case 0:
      if(digitalRead(inputChannel) == HIGH && digitalRead(inputChannel+1) == LOW) {
        return channelData[referenceChannel][1];
      }else if(digitalRead(inputChannel) == LOW && digitalRead(inputChannel+1) == HIGH) {
        return channelData[referenceChannel][0];
      }else {
        return channelData[referenceChannel][2];
      }
      break;
    case 1:
      if(digitalRead(inputChannel) == HIGH && digitalRead(inputChannel+1) == LOW) {
        return channelData[referenceChannel][0];
      }else if(digitalRead(inputChannel) == LOW && digitalRead(inputChannel+1) == HIGH) {
        return channelData[referenceChannel][1];
      }else {
        return channelData[referenceChannel][2];
      }
      break;
    default:
      Serial.print("Falscher Wert Servoinvertierung Kanal: ");
      Serial.println(referenceChannel);
  }
}

int mafFiltering(int b, int a) {
  int z;
  mafData[a][1] = mafData[a][1] - mafData[a][mafData[a][0]];        // Remove the oldest entry from the sum
  mafData[a][mafData[a][0]] = b;                                    // Add the newest reading to the window
  mafData[a][1] = mafData[a][1] + b;                                // Add the newest reading to the sum
  mafData[a][0] = mafData[a][0]+1;                                  // Increment the index, and wrap to 0 if it exceeds the window size
  if(mafData[a][0] >= windowSize+2) {
    mafData[a][0] = 2;
  }
  return z = mafData[a][1] / windowSize;                            // Divide the sum of the window by the window size for the result
}


//########## setup code ##########
void setup() {
  //GPIO setup
  pinMode(25, OUTPUT);
  pinMode(0, INPUT_PULLDOWN);
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);
  pinMode(3, INPUT_PULLDOWN);
  pinMode(4, INPUT_PULLDOWN);
  pinMode(5, INPUT_PULLDOWN);
  pinMode(6, INPUT_PULLDOWN);
  pinMode(7, INPUT_PULLDOWN);

  //serial setup
  Serial.begin(115200);
  delay(1000);
  adc1.begin(21);
  adc2.begin(22);
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {
      }
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(servoData));
  radio.openWritingPipe(address[0]);
  radio.stopListening();
  
  
  
  //preparing the index in mafData
  for(int i=0; i<16; i++) {
    mafData[i][0] = 2;
  }
}


//########## loop code ##########
void loop() {
  //blink the onboard led
  digitalWrite(25, blink);
  blink = !blink;

  //read data from both adcs
  for (int i=0; i<8; i++) {
    controlData[i][0] = adc1.analogRead(i);
  }
  for (int i=0; i<2; i++) {
    controlData[i+8][0] = adc2.analogRead(i);
  }

  //moving average filter
  for(int i=0; i<10; i++) {
    controlData[i][5] = mafFiltering(controlData[i][0], i);
  }

  //mapping data from analog range to servo range with limits, zeropoint, deadzone and invert
  //servoData.sD0 = analogLinear(0);
  servoData.sD1 = analogLinear(1);
  servoData.sD2 = analogLinear(2);
  //servoData.sD3 = analogLinear(3);
  //servoData.sD4 = analogLinear(4);
  //servoData.sD5 = analogLinear(5);
  //servoData.sD6 = analogLinear(6);
  //servoData.sD7 = analogLinear(7);
  //servoData.sD8 = analogLinear(8);
  //servoData.sD9 = analogLinear(9);
  //servoData.sD10 = digital2Way(10, 0);
  //servoData.sD11 = digital2Way(11, 1);
  //servoData.sD12 = digital2Way(12, 2);
  //servoData.sD13 = digital2Way(13, 3);
  //servoData.sD14 = digital3Way(14, 4);
  //servoData.sD15 = digital3Way(15, 6);
  
  //sending data to the radio
  radio.write(&servoData, sizeof(servoData));

  //debuggingzone
  /*for(int i=0; i<16; i++) {
    Serial.print("Channel Data [");
    Serial.print(i);
    Serial.print("][0]: ");
    Serial.println(channelData[i][0]);
  }*/
  Serial.println("##########");
  for(int i=0; i<10; i++) {
    Serial.print("Control Data [");
    Serial.print(i);
    Serial.print("][5]: ");
    Serial.println(controlData[i][5]);
  }
  Serial.println("##########");
  Serial.print("sD0: ");
  Serial.print(servoData.sD0);
  Serial.print(" sD1: ");
  Serial.print(servoData.sD1);
  Serial.print(" sD2: ");
  Serial.print(servoData.sD2);
  Serial.print(" sD3: ");
  Serial.print(servoData.sD3);
  Serial.print(" sD4: ");
  Serial.print(servoData.sD4);
  Serial.print(" sD5: ");
  Serial.print(servoData.sD5);
  Serial.print(" sD6: ");
  Serial.print(servoData.sD6);
  Serial.print(" sD7: ");
  Serial.print(servoData.sD7);
  Serial.print(" sD8: ");
  Serial.print(servoData.sD8);
  Serial.print(" sD9: ");
  Serial.print(servoData.sD9);
  Serial.print(" sD10: ");
  Serial.print(servoData.sD10);
  Serial.print(" sD11: ");
  Serial.print(servoData.sD11);
  Serial.print(" sD12: ");
  Serial.print(servoData.sD12);
  Serial.print(" sD13: ");
  Serial.print(servoData.sD13);
  Serial.print(" sD14: ");
  Serial.print(servoData.sD14);
  Serial.print(" sD15: ");
  Serial.println(servoData.sD15);
  //delay(10);
}
