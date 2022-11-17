#include <SPI.h>                              //Library voor  SPI Communicatie
#include <mcp2515.h>                          //Library voor CAN Communicatie

struct can_frame canMsg;                      //Maak de canMsg aan

MCP2515 mcp2515(10);                          //CS van can module zit op pin 10

int rood = 7;                                 //rode led op pin 7
int groen = 6;                                //groene led op pin 6
int blauw = 5;                                //blauwe led op pin 5
int geel = 4;                                 //gele led op pin 4

int mode = 0;                                 //mode 1 = ECR control 0: Bridge control
int state = 0;                                //State 1:running 0:stopped

float throttleA = 0;                          //oude stand potmeter
float throttleB = 0;                          //nieuwe stand potmeter
float difference = 0;                         //verschil oude stand en nieuwe stand potmeter

bool hasSent = false;                         //maak variabele hasSent intial = false

int i = 0;                                    // maak i variabele initial = 0
int controlcycle = 200;                       // controlcycle constante

/*Variabelen voor het running average filter*/
const int numReadings = 10;                   
int readings[numReadings];                    // the readings from the analog input
int readIndex = 0;                            // the index of the current reading
int total = 0;                                // the running total
int average = 0;                              // the average
int inputPin = A0;

void setup() {
  Serial.begin(9600);                         // begin serial communication met een baudrate van 9600 bits/sec
  SPI.begin();                                //Begin SPI communicatie
  
  mcp2515.reset();                            //reset CAN module
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);  //Sets CAN at speed 500KBPS and clock 8MHz
  mcp2515.setNormalMode();                    //Sets CAN at normal mode

  pinMode(rood, OUTPUT);                      //definieer variabele rood als output
  pinMode(groen, OUTPUT);                     //definieer variabele groen als output 
  pinMode(blauw, OUTPUT);                     //definieer variabele blauw als output
  pinMode(geel, OUTPUT);                      //definieer variabele geel als output

  digitalWrite(rood, LOW);                    //rood lampje uit
  digitalWrite(groen, LOW);                   //groen lampje uit
  digitalWrite(blauw, LOW);                   //blauw lampje uit
  digitalWrite(geel, LOW);                    //geel lampje uit
}


void loop() {
  /*led output selectors*/ 
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK){ // To receive data (Poll Read)    //lees can bus uit als de can module geen error heeft
    if(canMsg.can_id == 2){                                                                 //lees can_id 2
      mode = canMsg.data[0];                                                                //variabele mode = can message data [0]
      if(mode == 1){                                                                        //check of mode = 1
        digitalWrite(blauw, LOW);                                                           // geel lampje aan, blauw lampje uit
        digitalWrite(geel, HIGH); 
      }else if(mode == 0){                                                                  //check of mode = 0
        digitalWrite(geel, LOW);                                                            //geel lampje uit, blauw lampje aan
        digitalWrite(blauw, HIGH);
        
      }
    }

    if(canMsg.can_id == 1){                                                                 //lees can_id 1
      state = canMsg.data[0];                                                               //variabele state = can message data [0]
      if(state == 1){                                                                       //check of state = 1
        digitalWrite(groen, HIGH);                                                          //groen lampje aan, rood lampje uit
        digitalWrite(rood, LOW);                                                                                                                   
      }else if(state == 1){                                                                 //check of state = 0
        digitalWrite(groen, LOW);                                                           //groen lampje uit, rood lampje aan     
        digitalWrite(rood, HIGH);
      }
    }  
  }


  /*Running average filter*/
  total = total - readings[readIndex];                                                      //subtract the last reading:
  readings[readIndex] = analogRead(inputPin);                                               //read from the sensor:
  total = total + readings[readIndex];                                                      //add the reading to the total:
  readIndex = readIndex + 1;                                                                //advance to the next position in the array:
  if (readIndex >= numReadings) {                                                           //if we're at the end of the array...
    readIndex = 0;                                                                          //...wrap around to the beginning:
  }
  average = total / numReadings;                                                            //calculate the average: definieer als variabele average
 
  throttleA = average/10.2;                                                                 //variabele throttleA = average/10.2
  difference = abs(throttleA - throttleB);                                                  //variabele difference ==throttleA-throttleB, absolute waarde
  if(difference >= 1){                                                                      //check of difference groter is dan 1
    throttleB = throttleA;                                                                  //maak throttleB gelijk aan ThrottleA
    
    canMsg.can_id  = 4;                                                                     //CAN ID = 4                    
    canMsg.can_dlc = 1;                                                                     //CAN data length as 1
    canMsg.data[0] = throttleB;                                                             //Update throttleB value in [0]
 
    mcp2515.sendMessage(&canMsg);                                                           //Sends the CAN message   
  }

  /*bridge ok signaal zender*/
  i = i+1;                                  //tel 1 bij variabele i op
  if(i >= controlcycle){                    //check of i groter is dan variabele controlcycle
    canMsg.can_id  = 6;                     //CAN ID = 6
    canMsg.can_dlc = 1;                     //CAN data length is 1 byte
    canMsg.data[0] = 1;                     //Update "oke" als 1 in [0]
 
    mcp2515.sendMessage(&canMsg);           //Sends the CAN message
    Serial.println("bridge ok");
    i = 0;                                  //variabele i = 0
  }

  delay(5);                                 //maak een delay van 5 ms voor het begin van de volgende cylcus
}
