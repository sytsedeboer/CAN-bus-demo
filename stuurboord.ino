#include <SPI.h>            //Library voor  SPI Communicatie
#include <mcp2515.h>        //Library voor CAN Communicatie

struct can_frame canMsg;

MCP2515 mcp2515(10);        //CS zit op pin 10

int knop = 4;
int SB = 0;
bool hasSent = false;

void setup() {
  while (!Serial);
  Serial.begin(9600);
  SPI.begin();               //Begin SPI communicatie

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();

  pinMode(knop, INPUT);

}

void loop() {
  SB = digitalRead(knop);

  if(SB == HIGH){
    if(hasSent == false){
      
 
      canMsg.can_id  = 1;           //1
      canMsg.can_dlc = 1;               //CAN data length as 1
      canMsg.data[0] = SB;               //Update SB value in [0]
 
      mcp2515.sendMessage(&canMsg);     //Sends the CAN message

      hasSent = true;
      Serial.println(SB);
      Serial.println(hasSent);
    }
  }

   if(SB == LOW){
    if(hasSent == true){
      
      canMsg.can_id  = 1;           //CAN id as 1
      canMsg.can_dlc = 1;               //CAN data length as 1
      canMsg.data[0] = SB;               //Update SB value in [0]
 
      mcp2515.sendMessage(&canMsg);     //Sends the CAN message

      hasSent = false;
      Serial.println(SB);
      Serial.println(hasSent);
      
    }
   }
    delay(5);
  }


  
