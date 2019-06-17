#include <TheThingsNetwork.h>

#include <Streaming.h>

#include<SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
#define relay 3
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

 float Celcius=0;
 float Fahrenheit=0;
 #define TdsSensorPin A1
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,temperature = 25;

SoftwareSerial mySerial(9,10);
#define SensorPin A0            //pH meter Analog output to Arduino Analog Input 2
#define Offset 0.00            //deviation compensate
#define LED 13
#define samplingInterval 20
#define led 12

#define printInterval 800
#define ArrayLenth  40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex = 0;

#define freqPlan TTN_FP_EU868
#define debugSerial Serial

TheThingsNetwork ttn(mySerial, debugSerial, freqPlan);


void setup() {

//configure mySerial, this could also be a 
//software serial. 
//pinMode(LED, OUTPUT);
mySerial.begin(115200);
pinMode(relay, OUTPUT);
pinMode(led,OUTPUT);
digitalWrite(led,HIGH);
sensors.begin();
//configure the main RX0 and TX0 port on arduino
Serial.begin(115200);
//try getting the version of the board firmware
sendCommand("at+version\r\n");
delay(5000);
////set conn config
setConnConfig("dev_eui", "373934357A375416");
delay(5000);
setConnConfig("app_eui", "70B3D57ED001D646");
delay(5000);
setConnConfig("app_key", "53BB23C04AE2EFFF3F88112F2B43E677");
delay(5000);
//join the connection
sendJoinReq();
delay(5000);
pinMode(TdsSensorPin,INPUT);
//send data to gateway


}


//string to hold the response of a command in rak811
String response = "";
//the famous arduino loop function. runs continuosly
void loop() {
 static unsigned long samplingTime = millis();
 static unsigned long printTime = millis();
  static unsigned long analogSampleTimepoint = millis();
 static float pHValue, voltage;
 sensors.requestTemperatures(); 
  Celcius=sensors.getTempCByIndex(0);
  Fahrenheit=sensors.toFahrenheit(Celcius);
  Serial.print(" C  ");
  Serial.print(Celcius);
  if(Celcius>=27){
     digitalWrite(relay, HIGH);
  }
  else{
    digitalWrite(relay, LOW);
  }
 if (millis() - samplingTime > samplingInterval)
 {
   pHArray[pHArrayIndex++] = analogRead(SensorPin);
   if (pHArrayIndex == ArrayLenth)pHArrayIndex = 0;
   voltage = avergearray(pHArray, ArrayLenth) * 5.0 / 1024 + 0.05;
   pHValue = 3.5 * voltage + Offset;
   samplingTime = millis();
 }
 if (millis() - printTime > printInterval)  //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
 {
   //Serial.print("Voltage:");
   //Serial.print(voltage, 2);
   Serial.print("pH value: ");
   Serial.println(pHValue, 2);
   //Serial.print(" alkilinity value:");
   //Serial.println(pow(10,(((pHValue- 4.1333)/1.7177) )));
   //digitalWrite(LED, digitalRead(LED) ^ 1);
   printTime = millis();
 }
  if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT)
         analogBufferIndex = 0;
   }
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
   }
  
  sendData(1,2,pHValue,Celcius,tdsValue);
  delay(5000);
  Serial.println("In loop...");
//  delay(10000);
}  
/**
 * Function to send a command to the 
 * lora node and wait for a response
 */
void sendCommand(String atComm){
response = "";
Serial.println(atComm);
mySerial.print(atComm);
Serial.println("Hello");
  while(mySerial.available()){
    char ch = mySerial.read();
    Serial.println(ch);
    response += ch;
  }
  Serial.println(response);
}

/**
 * send the rak811 to sleep for time 
 * specified in millis paramteer
 */
void sleep(unsigned long milliseconds=3000){
  sendCommand("at+sleep\r\n");
  delay(milliseconds);
  //send any charcater to wakeup;
  sendCommand("***\r\n");
}

/**
 * reset board after the specified time delay millisenconds
 * <mode> = 0 Reset and restart module
= 1 Reset LoRaWAN or LoraP2P stack and Module will reload LoRa
configuration from EEPROM
 */
void resetChip(int mode, unsigned long delaySec=10000){
  delay(delaySec);
  String command = (String)"at+reset=" + mode + (String)"\r\n";
  sendCommand(command);
}

/**
 * Reload the default parameters of LoraWAN or LoraP2P setting
 */
void reload(unsigned long delaySec=10000){
  delay(delaySec);
  sendCommand("at+reload\r\n");
}

/**
 * Function to set module mode
 * <mode> = 0 LoraWAN Mode (default mode)
= 1 LoraP2P Mode
 */
void setMode(int mode){
  String command = (String)"at+mode=" + mode + (String)"\r\n";
  sendCommand(command);  
  delay(10000);
}

/**
 * Function to send data to a lora gateway;
 * <type> = 0 send unconfirmed packets
= 1 send confirmed packets
<port> = 1-223 port number from 1 to 223
<data>= <hex value> hex value(no space). The Maximum length of <data> 64 bytes
 */
 
void sendData(int type, int port, float data,float data1,float data2){
  uint32_t pH = data * 100;
  uint32_t cel=data1*100;
  String ph_temp;
  String alk2;
  uint32_t tds=data2;
  String cel1;
  String tds1;
  Serial.println(pH);
  if (pH<=9 || (pH>99 && pH<=999)){
    ph_temp = "0"+(String)pH;
  }
  else if (pH>9 || pH>999){
    ph_temp = (String)pH;
  }
  float alk=(pow(10,(((data- 4.1333)/1.7177) )));
  Serial.println(alk);
  uint32_t alk1=alk*100;
  if (alk1<=9 || (alk1>99 && alk1<=999)){
    alk2 = "0"+(String)alk1;
  }
  else if (pH>9 || pH>999){
    alk2 = (String)alk1;
  }
   if (cel<=9 || (cel>99 && cel<=999)){
    cel1 = "0"+(String)cel;
  }
  else if (pH>9 || pH>999){
    cel1 = (String)cel;
  }
   if (tds<=9 || (tds>99 && tds<=999)){
    tds1 = "0"+(String)tds;
  }
  else if (tds>9 || tds>999){
    tds1 = (String)tds;
  }
  
  
  Serial.println(alk1);
  mySerial.print("at+send=1,2,"+(String)ph_temp+"20"+(String)alk2+"20"+(String)cel1+"20"+(String)tds1+"20\r\n");
  delay(500);
  Serial.println("Data is sent...");
}

/**
 * Function to set the connection config
 * < dev_addr >:<address>
<address>-------------------4 bytes hex number representing the
device address from 00000001 –
FFFFFFFE

<dev_eui>:<eui>
<eui>-------------------------- 8-byte hexadecimal number
representing the device EUI

<app_eui>:<eui>
<eui>----------------------------8-byte hexadecimal number
representing the application EUI

<app_key>:<key>
- 11 -
<key>----------------------------16-byte hexadecimal number
representing the application key

<nwks_key>:<key>
<key>-------------------------16-byte hexadecimal number
representing the network session key

<apps_key>:<key>
<key>------------------------ 16-byte hexadecimal number representing
the application session key

<tx_power>:<dbm>
<dbm>------------------- LoRaWAN Tx Power

<adr>:<status>
<status>----------------------------- string value representing
the state, either “on” or “off”.

<dr>:<data rate>
<data rate>-----------------------decimal number representing the
data rate, from 0 and 4, but within
the limits of the data rate range for
the defined channels.

< public_net >:<status>
<status>------------------- string value representing
the state, either “on” or “off”.

< rx_delay1 >:<delay>
<delay>-------------------decimal number representing
the delay between the transmission
and the first Reception window
in milliseconds, from 0 to 65535.
 */
void setConnConfig(String key, String val){
  sendCommand("at+set_config=" + key + ":" + val + "\r\n");
  delay(10000);
  Serial.println("Set Successfully....");
}

/**
 * Function to join the connection with the specified parameters
 * you should get OK
at+recv=3,0,0 as response.
 */
 void sendJoinReq(){
  Serial.println("Entered joinReq....");
  String mode = "otaa";
  String hello=(String)"at+join="+mode+(String)"\r\n";
  sendCommand(hello);
  delay(10000);
  Serial.println("Joined");
 }
 double avergearray(int* arr, int number)
{
 int i;
 int max, min;
 double avg;
 long amount = 0;
 if (number <= 0)
 {
   Serial.println("Error number for the array to avraging!/n");
   return 0;
 }
 if (number < 5) //less than 5, calculated directly statistics
 {
   for (i = 0; i < number; i++)
   {
     amount += arr[i];
   }
   avg = amount / number;
   return avg;
 }
 else
 {
   if (arr[0] < arr[1])
   {
     min = arr[0]; max = arr[1];
   }
   else
   {
     min = arr[1]; max = arr[0];
   }
   for (i = 2; i < number; i++)
   {
     if (arr[i] < min)
     {
       amount += min;      //arr<min
       min = arr[i];
     }
     else
     {
       if (arr[i] > max)
       {
         amount += max;  //arr>max
         max = arr[i];
       }
       else
       {
         amount += arr[i]; //min<=arr<=max
       }
     }//if
   }//for
   avg = (double)amount / (number - 2);
 }//if
 return avg;
}
int getMedianNum(int bArray[], int iFilterLen)
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++)
      {
      for (i = 0; i < iFilterLen - j - 1; i++)
          {
        if (bTab[i] > bTab[i++])
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i++];
        bTab[i+1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}
