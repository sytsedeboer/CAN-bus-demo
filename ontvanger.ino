#include <SPI.h>                            //Library voor SPI Communicatie
#include <mcp2515.h>                        //Library voor CAN Communicatie
#include <LiquidCrystal.h>                  //Library voor het gebruik van het LCD scherm

struct can_frame canMsg;                    //Maak canMsg

MCP2515 mcp2515(10);                        //CS zit op pin 10

int ledgroen = 9;                           //De groene led zit op pin 9
int ledrood = 8;                            //De rode led zit op pin 8
int x = 0;                                  //Def int x
int y = 0;                                  //Def in y

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2; //Def de LCD pinnen
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);   //LCD pinnen


void setup()
{
  Serial.begin(9600);                
  SPI.begin();                               //Begin SPI communicatie
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                   //Sets CAN at normal mode

  pinMode(ledgroen, OUTPUT);                 //ledgroen als ouput
  pinMode(ledrood, OUTPUT);                  //ledrood als output

  digitalWrite(ledgroen, LOW);
  digitalWrite(ledrood, LOW);

  lcd.begin(16, 2);                          // set up the LCD's number of columns and rows: 
  lcd.clear();

}


void loop()
{
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK){ // To receive data (Poll Read)
 
    Serial.println(canMsg.can_id, HEX);       // print ID
    
    if(canMsg.can_id == 1){
      int x = canMsg.data[0];
      Serial.println(x);
      if(x >= 1){
        digitalWrite(ledgroen, HIGH);          //ledgroen aan en print groen aan, wanneer dit ontvangen word van de SB node
        lcd.setCursor(0,0);
        lcd.print("groen aan");
        } else{
          digitalWrite(ledgroen, LOW);         //ledgroen uit en print groen uit, wanneer dit ontvangen word van de SB node
          lcd.setCursor(0,0);
          lcd.print("groen uit");
      }
    }
    else if(canMsg.can_id == 2){
      int y = canMsg.data[0];
      Serial.println(y);
      if(y >= 1){
        digitalWrite(ledrood, HIGH);            //ledrood aan en print rood aan, wanneer dit ontvangen word van de BB node
        lcd.setCursor(0,1);
        lcd.print("rood aan");
        } else{
          digitalWrite(ledrood, LOW);          //ledrood uit en print rood uit, wanneer dit ontvangen word van de BB node
          lcd.setCursor(0,1);
          lcd.print("rood uit");
    }
         
    }  

    }


    
}
