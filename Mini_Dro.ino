/*
STM32 adaption by Matthias Diro, tested with maple mini and heltec OLED 12864 I2c; adress: 0x3C 
Things to know:
 This adaption uses hardware I2C (hardwire.h), Port: I2c1. SCL1=PB6 and SDA1=PB7
 Use serial2 : Rx2=PA3 and Tx2=PA2 
 further details: STM32_README.txt
*/
/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_STM32.h>
#include "HardwareTimer.h"
#include "QuadDecoder.h"
#include "Button.h"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

//Hard timer 1 for X scale quadrature : T1C1 (PA8) and T1C2 (PA9)
HardwareTimer timer_X(1);
//Hard timer 3 for Y scale quadrature : T3C1 (PA6) and T3C2 (PA7)  
HardwareTimer timer_Y(3);
//Hard timer 2 for Z scale quadrature : T2C1 (PA0) and T2C2 (PA1)  
HardwareTimer timer_Z(2);

//Quad decoder Class
QuadDecoder Quad_X(512,false,false);
QuadDecoder Quad_Y(512,false,false);
QuadDecoder Quad_Z(512,false,false);

//Keyboard
Button MiniDroKeyboard;

#define IN_SW_X PB15
#define IN_SW_Y PB14
#define IN_SW_Z PB12
#define IN_SW_M PB13





typedef struct
{
  boolean Inverted_X;  
  boolean Inverted_Y;
  boolean Inverted_Z;
  boolean Diameter_Mode_Y;
  unsigned int  Resolution;
} sConfigDro;

enum eMS_Dro 
        {   
      State_Normal, 
      State_In_Config 

    };



// Variable
sConfigDro ConfigDro;
eMS_Dro MS_Dro;
unsigned long TestSys;

//******************  IT function
void IT_Overflow_X(){
if (timer_X.getDirection()){
  Quad_X.OverflowMinus();
} else{
  Quad_X.OverflowPlus();
}
}
void IT_Overflow_Y(){
if (timer_Y.getDirection()){
   Quad_Y.OverflowMinus();
} else{
  Quad_Y.OverflowPlus();
}
}
void IT_Overflow_Z(){
if (timer_Z.getDirection()){
   Quad_Z.OverflowMinus();
} else{
  Quad_Z.OverflowPlus();
}
}
void SysTick_Handler() 
{
  
  //TestSys++;
}



void setup()   {                
  Serial2.begin(115200);
  Serial2.println("\nTest");

  MS_Dro = State_Normal; 

  //Test led PC13
  pinMode(PC13, OUTPUT);  //
  digitalWrite(PC13, LOW);

  //Switch
  pinMode(IN_SW_X, INPUT_PULLUP);
  pinMode(IN_SW_Y, INPUT_PULLUP);  
  pinMode(IN_SW_Z, INPUT_PULLUP);  
  pinMode(IN_SW_M, INPUT_PULLUP);

  /* Systick used by I2C at 1Khz... */ 
  systick_attach_callback(SysTick_Handler);

  //Config general
  Restore_Config();

  
  //## configure timer_X as quadrature encoder ##
  pinMode(PA8, INPUT_PULLUP);  //channel A
  pinMode(PA9, INPUT_PULLUP);  //channel B
  timer_X.setMode(0, TIMER_ENCODER); //set mode, the channel is not used when in this mode. 
  timer_X.pause(); //stop... 
  timer_X.setPrescaleFactor(1); //normal for encoder to have the lowest or no prescaler. 
  timer_X.setOverflow(0xFFFF);    
  timer_X.setCount(0);          //reset the counter. 
  timer_X.setEdgeCounting(TIMER_SMCR_SMS_ENCODER3); //or TIMER_SMCR_SMS_ENCODER1 or TIMER_SMCR_SMS_ENCODER2. This uses both channels to count and ascertain direction. 
  timer_X.attachInterrupt(0, IT_Overflow_X); //Overflow interrupt  
  timer_X.resume();                 //start the encoder... 
  //timer_X.getCount();

  //## configure timer_Y as quadrature encoder ##
  pinMode(PA6, INPUT_PULLUP);  //channel A
  pinMode(PA7, INPUT_PULLUP);  //channel B
  timer_Y.setMode(0, TIMER_ENCODER); //set mode, the channel is not used when in this mode. 
  timer_Y.pause(); //stop... 
  timer_Y.setPrescaleFactor(1); //normal for encoder to have the lowest or no prescaler. 
  timer_Y.setOverflow(0xFFFF);    
  timer_Y.setCount(0);          //reset the counter. 
  timer_Y.setEdgeCounting(TIMER_SMCR_SMS_ENCODER3); //or TIMER_SMCR_SMS_ENCODER1 or TIMER_SMCR_SMS_ENCODER2. This uses both channels to count and ascertain direction. 
  timer_Y.attachInterrupt(0, IT_Overflow_Y); //Overflow interrupt  
  timer_Y.resume();                 //start the encoder... 
  //timer_Y.getCount();

  //## configure timer_Z as quadrature encoder ##
  pinMode(PA0, INPUT_PULLUP);  //channel A
  pinMode(PA1, INPUT_PULLUP);  //channel B
  timer_Z.setMode(0, TIMER_ENCODER); //set mode, the channel is not used when in this mode. 
  timer_Z.pause(); //stop... 
  timer_Z.setPrescaleFactor(1); //normal for encoder to have the lowest or no prescaler. 
  timer_Z.setOverflow(0xFFFF);    
  timer_Z.setCount(0);          //reset the counter. 
  timer_Z.setEdgeCounting(TIMER_SMCR_SMS_ENCODER3); //or TIMER_SMCR_SMS_ENCODER1 or TIMER_SMCR_SMS_ENCODER2. This uses both channels to count and ascertain direction. 
  timer_Z.attachInterrupt(0, IT_Overflow_Z); //Overflow interrupt  
  timer_Z.resume();                 //start the encoder... 
  //timer_Z.getCount();

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  delay(2000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);
  // Clear the buffer.
  display.clearDisplay();
  display.display();
  delay(1000);
}

void loop() 
{
  eButtonState eState;
  unsigned int CurrentSelection=1;
  while (1) 
  {
    Quad_X.CounterValue(timer_X.getCount());
    Quad_Y.CounterValue(timer_Y.getCount());
    Quad_Z.CounterValue(timer_Z.getCount());

    //Update Keyboard
    MiniDroKeyboard.Update();
    eState = MiniDroKeyboard.GiveMeTheButtonState(); 
    //Masterstate
    switch ( MS_Dro )
    {
      case State_Normal:
        //Normal mode 
        Display_Test();
        if(eState == X_KeyShortPressed) Quad_X.SetZeroActiveMode(); 
        if(eState == Y_KeyShortPressed) Quad_Y.SetZeroActiveMode();
        if(eState == Z_KeyShortPressed) Quad_Z.SetZeroActiveMode();
        if(eState == X_LongPressed) Quad_X.SwitchMode(); 
        if(eState == Y_LongPressed) Quad_Y.SwitchMode();
        if(eState == Z_LongPressed) Quad_Z.SwitchMode();
        if(eState == M_LongPressed) MS_Dro = State_In_Config ; 
      break;
      case State_In_Config:
        //Config mode
        if(eState == M_LongPressed) 
        { 
          MS_Dro = State_Normal ;
          Dispatch_Config();
        }
        if(eState == X_KeyShortPressed)
        {
          if(CurrentSelection==1)CurrentSelection=6;
          else CurrentSelection--;     
        }
        if(eState == Z_KeyShortPressed)
        {
          if(CurrentSelection==6)CurrentSelection=1;
          else CurrentSelection++;     
        }
        if(eState == Y_KeyShortPressed)
        {
          switch(CurrentSelection)
          {
            case 1:
              ConfigDro.Inverted_X = !ConfigDro.Inverted_X;  
            break;  
            case 2:
              ConfigDro.Inverted_Y = !ConfigDro.Inverted_Y;
            break;
            case 3:
              ConfigDro.Inverted_Z = !ConfigDro.Inverted_Z;
            break;
            case 4:
              ConfigDro.Diameter_Mode_Y = !ConfigDro.Diameter_Mode_Y;
            break;                        
          }
            
        }
        
        Display_Config(CurrentSelection);
      break;   
    }  
  }  
}

void Display_Test()
{
  char buffer_x[16];
  char buffer_y[16];
  char buffer_z[16];


  
  sprintf(buffer_x,"%4.3f",Quad_X.GetValue());
  sprintf(buffer_y,"%4.3f",Quad_Y.GetValue());
  sprintf(buffer_z,"%4.3f",Quad_Z.GetValue());

  
  
  display.clearDisplay();
  //Constant text
  display.setTextColor(BLACK,WHITE);
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println(" Mini Dro V1.0 GPA ");
  display.setTextSize(1);
  display.setTextColor(WHITE); 
  display.println("  ");

  display.setTextSize(2);
  if(Quad_X.RelativeModeActived()) display.setTextColor(BLACK,WHITE);
  else display.setTextColor(WHITE);  
  display.print("X:");
  display.setTextColor(WHITE);
  display.println(buffer_x);
  if(Quad_Y.RelativeModeActived()) display.setTextColor(BLACK,WHITE);
  else display.setTextColor(WHITE);  
  display.print("Y:");
  display.setTextColor(WHITE);
  display.println(buffer_y);
  if(Quad_Z.RelativeModeActived()) display.setTextColor(BLACK,WHITE);
  else display.setTextColor(WHITE);  
  display.print("Z:"); 
  display.setTextColor(WHITE);
  display.println(buffer_z);
  display.display();
}


void Display_Config(unsigned int CurrentSelection)
{
 
  
  display.clearDisplay();
  //Constant text
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println(" ** DRO CONFIG **");
  if(CurrentSelection==1)display.setTextColor(BLACK,WHITE); 
  display.print("X inverted : ");
  if(ConfigDro.Inverted_X)display.println("true");
  else display.println("false");
  if(CurrentSelection==1)display.setTextColor(WHITE);
  if (CurrentSelection==2)display.setTextColor(BLACK,WHITE); 
  display.print("Y inverted : ");
  if(ConfigDro.Inverted_Y)display.println("true");
  else display.println("false");
  if(CurrentSelection==2)display.setTextColor(WHITE);
  if (CurrentSelection==3)display.setTextColor(BLACK,WHITE); 
  display.print("Z inverted : ");
  if(ConfigDro.Inverted_Z)display.println("true");
  else display.println("false");
  if(CurrentSelection==3)display.setTextColor(WHITE);
  if (CurrentSelection==4)display.setTextColor(BLACK,WHITE);
  display.print("Diameter mode : ");
  if(ConfigDro.Diameter_Mode_Y)display.println("true");
  else display.println("false");
  if(CurrentSelection==4)display.setTextColor(WHITE);
  if (CurrentSelection==5)display.setTextColor(BLACK,WHITE);
  display.println("Resolution : 512");
  if(CurrentSelection==5)display.setTextColor(WHITE);
  if (CurrentSelection==6)display.setTextColor(BLACK,WHITE);
  display.println("--> Save & Exit");
  if(CurrentSelection==5)display.setTextColor(WHITE);
  display.display();
}

void Restore_Config()
{
  ConfigDro.Inverted_X = false;
  ConfigDro.Inverted_Y = false;  
  ConfigDro.Inverted_Z = false;
  ConfigDro.Diameter_Mode_Y = false;
  ConfigDro.Resolution = 512;

  //Dispatch the config
  Dispatch_Config();
}
void Dispatch_Config()
{
  Quad_X.SetSens( ConfigDro.Inverted_X );  
  Quad_Y.SetSens( ConfigDro.Inverted_Y );
  Quad_Z.SetSens( ConfigDro.Inverted_Z );
  Quad_Y.SetDiameterMode(ConfigDro.Diameter_Mode_Y);
  Quad_X.SetResolution(ConfigDro.Resolution);
  Quad_Y.SetResolution(ConfigDro.Resolution);
  Quad_Z.SetResolution(ConfigDro.Resolution);
}



