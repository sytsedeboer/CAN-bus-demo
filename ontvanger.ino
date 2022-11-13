#include <SPI.h>              //Library for using SPI Communication 
#include <mcp2515.h>          //Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
#include <LiquidCrystal.h>

struct can_frame canMsg;

MCP2515 mcp2515(10);

int ledgroen = 9;
int ledrood = 8;
int x = 0;
int y = 0;

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


void setup()
{
  Serial.begin(9600);                //Begins Serial Communication at 9600 baudrate
  SPI.begin();                       //Begins SPI communication
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                  //Sets CAN at normal mode

  pinMode(ledgroen, OUTPUT);
  pinMode(ledrood, OUTPUT);

  digitalWrite(ledgroen, LOW);
  digitalWrite(ledrood, LOW);

  lcd.begin(16, 2);  // set up the LCD's number of columns and rows: 
  lcd.clear();

}


void loop()
{
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK){ // To receive data (Poll Read)
 
    Serial.println(canMsg.can_id, HEX); // print ID
    
    if(canMsg.can_id == 1){
      int x = canMsg.data[0];
      Serial.println(x);
      if(x >= 1){
        digitalWrite(ledgroen, HIGH);
        lcd.setCursor(0,0);
        lcd.print("groen aan");
        } else{
          digitalWrite(ledgroen, LOW);
          lcd.setCursor(0,0);
          lcd.print("groen uit");
      }
    }
    else if(canMsg.can_id == 2){
      int y = canMsg.data[0];
      Serial.println(y);
      if(y >= 1){
        digitalWrite(ledrood, HIGH);
        lcd.setCursor(0,1);
        lcd.print("rood aan");
        } else{
          digitalWrite(ledrood, LOW);
          lcd.setCursor(0,1);
          lcd.print("rood uit");
    }
         
    }  

    }


    
}
