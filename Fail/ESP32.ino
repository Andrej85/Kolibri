#include "SparkFunLSM6DS3.h"
#include "Wire.h"
#include "SPI.h"

#define MAX_SAMPLES 16384     //STM
#define CS 5
#define dataReadyPin 25
#define inPin 12


#define yellow 32
#define Blue 33

#define batar 39

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//Create instance of the driver class
LSM6DS3 Sensor( SPI_MODE,  CS);


//calcs
long startLoop, intervalLoop; //for timer
int indexx;
signed short x[MAX_SAMPLES]; 
bool ROTATE_PULS[MAX_SAMPLES]; //Array of Puls

int CP_READ_RATE_VAL = 156;            //microseconds per reading measure (512/0.000160 ~ 6250 Hz)
int SAMPLES_RATE_VAL = 1024;


char c;
int Batar = 0;
int SleepT=0;
int AutoSleepT=0;
bool touch1detected = false;
bool touch2detected; // = true;
int threshold = 20;


unsigned long currentTime1;

  BluetoothSerial SerialBT;

 
  
void setup() {
  // put your setup code here, to run once:
  Serial.begin(1000000);
  SerialBT.begin("Kolibri_00003"); //Bluetooth device name
  delay(1000); //relax...1000
  SerialBT.println("Processor came out of reset.\n");

  pinMode(yellow, OUTPUT);
  pinMode(dataReadyPin, INPUT);
  pinMode(Blue, OUTPUT);
  digitalWrite(Blue, HIGH);


 touchAttachInterrupt(T2, gotTouch1, threshold);
    
 esp_sleep_enable_touchpad_wakeup(); //разрешение пробуждать от тача
  
  
  //Call .begin() to configure the IMUs
  
  if( Sensor.begin() != 0 )
  {
    SerialBT.println("Problem starting the sensor at 0x6B.");
  }
  else
  {
    SerialBT.println("Sensor at 0x6B started.");
    
    //Sensor.settings.accelSampleRate=3330;//Hz.  Can be: 13, 26, 52, 104, 208, 416, 833, 1660, 3330, 6660, 13330
    Sensor.settings.accelBandWidth=50;  //Hz.  Can be: 50, 100, 200, 400;
    Sensor.settings.fifoSampleRate = 10;
    Sensor.settings.accelFifoEnabled = 0;
    Sensor.settings.accelODROff = 1;
    //Sensor.fifoBegin();
    //Sensor.fifoClear();
Sensor.writeRegister(0x10, 0x87);//(65. 416Hz 16g 200 Hz)(74. 833Hz 16g 400 Hz)(77. 833Hz 16g 50 Hz) (84. 1660Hz 16g 400 Hz)(85. 1660Hz 16g 200 Hz)(86. 1660Hz 16g 100 Hz)(87. 1660Hz 16g 50 Hz)(94. 3300Hz 16g 400 Hz) (A4. 6600Hz 16g 400 Hz)(B4. 13300Hz 16g 400 Hz)(8F. 1660Hz 8g 50 Hz)
Sensor.writeRegister(0x17, 0x00);//LPF2  (84/50Р“С†)(A4/100Р“С†) (C4/9Р“С†)(E4 /400Р“С†)//HP  (04/4Р“С†)(24/100Р“С†)(44/9Р“С†)(64/400Р“С†)64


Sensor.writeRegister(0x19, 0x3C); //  enable functionalities of embedded functions and accelerometer filters
Sensor.writeRegister(0x13, 0x80); //XL_BW_SCAL_ODR  bandwidth determined by setting BW_XL[1:0]

Sensor.writeRegister(0x58, 0x10);//Enable accelerometer HP and LPF2 filters
Sensor.writeRegister(0x0D, 0x01);//drdy pin en=true
  }
}

 void callback(){
    }

void parseMenu(char c)
{
char cc;
  switch (c)
  {
   case '0':                            //СЃС‚Р°СЂС‚ РґРёР°РіРЅРѕСЃС‚РёРєРё
           //while(!SerialBT.available())
    while (cc!=' '){
           startDiagnostic(); 
  if(SerialBT.available()) cc=SerialBT.read();
                   }
           break;
   case '1':                            //РїР°СЂР°РјРµС‚СЂС‹
           readParameters(); 
           while(!SerialBT.available());
           break;
   case '2':
           while(!SerialBT.available())
           readAccelData(); 
           break;        
   //..
       case '5':
           setSAMPLES_RATE_VAL();
           break;
  //..
    case '7':
           setBW_RATE_VAL();
           break;           
  }
} 

void startDiagnostic(){
 
   
  
startLoop = millis();
   //***************reading data**************************************            
               for (indexx=0; indexx<SAMPLES_RATE_VAL; indexx++)
               { 
                while (digitalRead (dataReadyPin) == LOW) ; //////////???????????????
  //accel data
  x[indexx] = Sensor.readRawAccelX();
  //while( ( sensor.fifoGetStatus() & 0x8000 ) == 0 ) {};  //Wait for watermark
  //while( ( myIMU.fifoGetStatus() & 0x1000 ) == 0 ) {
  //x[indexx] = Sensor.calcAccel(Sensor.fifoRead());
  
  //speed sensor data
  ROTATE_PULS[indexx] = digitalRead(inPin) ;
        //invert for fish aye
        if (ROTATE_PULS[indexx]==0)ROTATE_PULS[indexx]=1;  
        else
        ROTATE_PULS[indexx]=0;
  
  //CP_READ_RATE
  //delayMicroseconds(CP_READ_RATE_VAL);
    
               }
  intervalLoop = millis()-startLoop;               
   //*****************sending data************************************
  
  //Serial1.println();
  //Serial1.print(intervalLoop);             
  SerialBT.print("{");
  for (indexx=0; indexx < SAMPLES_RATE_VAL; indexx++)
  {
    
  SerialBT.print(x[indexx], DEC);
  SerialBT.print(",");
  SerialBT.print(ROTATE_PULS[indexx], DEC);   
  SerialBT.print(";");
  }
  SerialBT.println(intervalLoop);  
  SerialBT.print ("}");
  //intervalLoop = millis()-startLoop;   
  //Serial1.println(intervalLoop);  

                   



  
}

void readParameters(){
  SerialBT.print("accelEnabled = ");
  SerialBT.println(Sensor.settings.accelEnabled);  
  SerialBT.print("accelODROff = ");
  SerialBT.println(Sensor.settings.accelODROff);  
  SerialBT.print("accelRange = ");
  SerialBT.println(Sensor.settings.accelRange);  
  SerialBT.print("accelSampleRate = ");
  SerialBT.println(Sensor.settings.accelSampleRate);  
  SerialBT.print("accelBandWidth = ");
  SerialBT.println(Sensor.settings.accelBandWidth);  
  SerialBT.print("accelFifoEnabled = ");
  SerialBT.println(Sensor.settings.accelFifoEnabled);  
  SerialBT.print("accelFifoDecimation = ");
  SerialBT.println(Sensor.settings.accelFifoDecimation);  
  SerialBT.print("FifoSR = ");
  SerialBT.println(Sensor.settings.fifoSampleRate);
  SerialBT.println(Sensor.settings.fifoModeWord);
}

void readAccelData() {
 SerialBT.print("\nAccelerometer:\n");
  SerialBT.print(" X1 = ");
  SerialBT.print(Sensor.readFloatAccelX(), 4);
  SerialBT.print("  Y1 = ");
  SerialBT.print(Sensor.readFloatAccelY(), 4);
  SerialBT.print("  Z1 = ");
  SerialBT.println(Sensor.readFloatAccelZ(), 4);
   
}

void printMenu(){
SerialBT.println();
SerialBT.println(F("******************************************"));
SerialBT.println(F("**********BT Oscillo MENU*****************"));
SerialBT.println(F("******************************************"));
SerialBT.println();
SerialBT.println(F("0) Start Diagnostic"));
SerialBT.println(F("1) ReadParameters"));
SerialBT.println(F("2) Read Accel data"));
SerialBT.println(F("******************************************"));
SerialBT.println(F("-- Configuration --"));
SerialBT.println(F("5) setSAMPLES_RATE_VAL"));
SerialBT.println(F("7) setBW_RATE_VAL")); 
//SerialBT.println(Bat); 
}


void setSAMPLES_RATE_VAL(){
//64 128 256 512 1024 2048 4096 8192 16384
SerialBT.println(F("setSAMPLES_RATE_VAL"));
SerialBT.println(F("1) 64 \t 2) 128 \t 3) 256"));
SerialBT.println(F("4) 512 \t 5) 1024 \t 6) 2048"));
SerialBT.println(F("7) 4096 \t 8) 8192 \t 9) 16384"));



while(!SerialBT.available());
c = SerialBT.read();
switch (c){
  case('1'): if (MAX_SAMPLES>=64)SAMPLES_RATE_VAL=64;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break;
  case('2'): if (MAX_SAMPLES>=128)SAMPLES_RATE_VAL=128;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break;          
  case('3'): if (MAX_SAMPLES>=256)SAMPLES_RATE_VAL=256;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break;          
  case('4'): if (MAX_SAMPLES>=512)SAMPLES_RATE_VAL=512;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break;
  case('5'): if (MAX_SAMPLES>=1024)SAMPLES_RATE_VAL=1024;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break; 
  case('6'): if (MAX_SAMPLES>=2048)SAMPLES_RATE_VAL=2048;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break; 
  case('7'): if (MAX_SAMPLES>=4096)SAMPLES_RATE_VAL=4096;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break;
  case('8'): if (MAX_SAMPLES>=8192)SAMPLES_RATE_VAL=8192;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break;
  case('9'): if (MAX_SAMPLES>=16384)SAMPLES_RATE_VAL=16384;else SAMPLES_RATE_VAL=MAX_SAMPLES;
          break;
}
} 

void setBW_RATE_VAL(){
//0x0F - 3200 Hz, 0x0E - 1600 Hz, 0x0D - 800 Hz, 0x0C - 400 Hz, 0x0B - 200 Hz, 0x0A - 100 Hz..
SerialBT.println(F("setBW_RATE_VAL"));
SerialBT.println(F("1) 13320 Hz  \t 2) 6660 Hz \t 3) 3330 Hz"));
SerialBT.println(F("4) 1660 Hz  \t 5) 833 Hz \t 6) 416 Hz"));
//Seril1.println(F("7) 208 Hz  \t 8) 104 Hz"));

while(!SerialBT.available());
c = SerialBT.read();
switch (c){


  case('1'): Sensor.writeRegister(0x10, 0xB4);// 13320Hz 16g 400 Hz 
          break;
  case('2'): Sensor.writeRegister(0x10, 0xA5);// 6660Hz 16g 200 Hz 
          break;          
  case('3'): Sensor.writeRegister(0x10, 0x96);// 3300Hz 16g 100  Hz 
          break;    
  case('4'): Sensor.writeRegister(0x10, 0x87);// 1660Hz 16g 10 Hz
          break;          
  case('5'): Sensor.writeRegister(0x10, 0x77);// 833Hz 16g 50 Hz 
          break;  
  case('6'): Sensor.writeRegister(0x10, 0x67);// 416Hz 16g 50 Hz
          break;          
 // case('7'): Sensor.writeRegister(0x10, 0xB6);// 13300Hz 16g 100 Hz
 //         break;   
 // case('8'): Sensor.writeRegister(0x10, 0xB7);// 13300Hz 16g 50 Hz
  //        break;          
   
   }
   
  }


  
void loop()
{
printMenu();

AutoSleepT=0;

while (!SerialBT.available()){

   if (analogRead(batar) < 2350)
  {digitalWrite(yellow, HIGH);}
  else
  {digitalWrite(yellow, LOW);}

   delay(1000);//ms
   AutoSleepT++;
if(AutoSleepT>120)//1 время автоматического переключения в сон
     esp_deep_sleep_start();
  
};
parseMenu(SerialBT.read());

} 

void gotTouch1(){
  delay(1000);//ms
if(digitalRead(T1)>5){
  
   esp_deep_sleep_start();
}
  
 }



 
