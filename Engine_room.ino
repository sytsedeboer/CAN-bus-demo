#include <SPI.h>                              //Library voor  SPI Communicatie
#include <mcp2515.h>                          //Library voor CAN Communicatie

struct can_frame canMsg;                      //Maak de canMsg aan

MCP2515 mcp2515(10);                          //CS zit op pin 10

int rood = 7;                                 //rode led op pin 7
int groen = 6;                                //groene led op pin 6
int blauw = 5;                                //blauwe led op pin 5
int geel = 4;                                 //gele led op pin 4

int Start = 8;                                //Start button op pin 8
int Stop = 9;                                 //Stop button op pin 9
int START = 0;                                //variabele start signaal
int STOP = 0;                                 //variabele stop signaal
int state = 0;                                //State 1:running 0:stopped
int mode = 0;                                 //mode wordt ontvangen vanuit de ECR 1:ECR control 2:BRIDGE control
int motorPin = 3;                             //base van motor transistor

int i = 0;                                    //Teller voor control cycle
int controlcycle = 200;                       //Control cycle lengte

float throttleBridge = 0;                     //Ontvangen throttle signaal van de brug
float throttleECR = 0;                        //Ontvangen throttle signaal van de ECR
float throttle = 0;                           //Throttle setting

bool hasSent = false;                         //maak hasSent intial = false


void setup() {
  Serial.begin(9600);                
  SPI.begin();                                //Begin SPI communicatie
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);  //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                    //Sets CAN at normal mode

  pinMode(Start, INPUT);                      //Start button is een input         
  pinMode(Stop, INPUT);                       //Stop button is een input
      
  pinMode(rood, OUTPUT);                      //rood is een output (led)       
  pinMode(groen, OUTPUT);                     //groen is een output (led) 
  pinMode(blauw, OUTPUT);                     //blauw is een output (led) 
  pinMode(geel, OUTPUT);                      //geel is een output (led) 

  digitalWrite(rood, LOW);                    //Start met rode led uit
  digitalWrite(groen, LOW);                   //Start met groene led uit
  digitalWrite(blauw, LOW);                   //Start met blauwe led uit
  digitalWrite(geel, LOW);                    //Start met gele led uit

  canMsg.can_id  = 1;                         //ID = 1 (state signaal)
  canMsg.can_dlc = 1;                         //CAN data length is 1 byte
  canMsg.data[0] = state;                     //Update state value in [0] is nu nog 0!
 
  mcp2515.sendMessage(&canMsg);               //Zend de intitial state
  
}

void loop() {
/*Verander state*/
  START = digitalRead(Start);                 //Geef de variabele START de waarde van de knop
  STOP = digitalRead(Stop);                   //Geef de variabele STOP de waarde van de knop

  if(START == 1 && STOP != 1){                //Start knop ingedrukt en stop knop niet ..
    state = 1;                                //... de state wordt 1(running)
  }else if(STOP == 1){                        //Stop knop wordt ingedrukt ...
    state = 0;                                //... de state wordt 0(stopped)
  }
  if(state == 1){                             //Wanneer de state 1(running) is ...
    digitalWrite(groen, HIGH);                //... laat alleen de groene led branden
    digitalWrite(rood, LOW);
  }else if(state == 0){                       //Wanneer de state 0(stopped) is ...
      digitalWrite(groen, LOW);               //... laat alleen de rode led branden
      digitalWrite(rood, HIGH);
    }

/*Zend de state*/
  if(state == HIGH){                          //Wanneer de state HIGH(running) is ...
    if(hasSent == false){                     //... en dit nog niet is verzonden:
      canMsg.can_id  = 1;                     //ID = 1 (state signaal)
      canMsg.can_dlc = 1;                     //CAN data length is 1 byte
      canMsg.data[0] = state;                 //Update state value in [0]
 
      mcp2515.sendMessage(&canMsg);           //Sends the CAN message

      hasSent = true;                         //Frame is verzonden
    }
  }
  if(state == LOW){                           //Wanneer de state LOW(stopped) is ...
    if(hasSent == true){                      //... en dit nog niet is verzonden:
      canMsg.can_id  = 1;                     //ID = 1 (state signaal)
      canMsg.can_dlc = 1;                     //CAN data length is 1 byte
      canMsg.data[0] = state;                 //Update state value in [0]
 
      mcp2515.sendMessage(&canMsg);           //Sends the CAN message

      hasSent = false;                        //Frame is verzonden
    }
  }

/*Ontvang en order can dataframes*/
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK){ // To receive data (Poll Read)
    if(canMsg.can_id == 2){                   //Wanneer het een bericht betreffende de mode is
      mode = canMsg.data[0];                  //Schrijf de waarde naar de mode variabele
      if(mode == 1){                          //Wanneer de mode 1(ECR) is:
        digitalWrite(blauw, LOW);             
        digitalWrite(geel, HIGH);             //Laat alleen de gele led branden
      }else if(mode == 0){                    //Wanneer de mode 0(BRIDGE0 is:
        digitalWrite(geel, LOW);
        digitalWrite(blauw, HIGH);            //Laat alleen de blauwe led branden
      }
    }else if(canMsg.can_id == 3){             //Wanneer het een bericht betreffende de ECR throttle setting is
      throttleECR = canMsg.data[0];           //Schrijf de waarde naar throttleECR
    }else if(canMsg.can_id == 4){             //Wanneer het een bericht betreffende de BRIDGE throttle setting is
      throttleBridge = canMsg.data[0];        //Schrijf de waarde naar throttleBridge
    }
  }

/*Stuur de motor aan vanuit het juiste signaal*/
  if(mode == 1){                              //Wanneer de mode 1(ECR) is
    throttle = throttleECR;                   //laat throttleECR de throttle waarde bepalen
  }else if(mode == 0){                        //Wanneer de mode 0(BRIDGE0 is
    throttle = throttleBridge;                //laat throttleBridge de throttle waarde bepalen
  }
  if(state == HIGH){                          //Wanneer de state HIGH(running) is
    analogWrite(motorPin, throttle);          //Laat de motor draaien volgens de throttle waarde
  }else if(state == LOW){
    analogWrite(motorPin, LOW);               //Anders is de motor gestopt
  }

/*Control cycle*/
  i = i + 1;                                  //tel door
  if(i >= controlcycle){                      //Wanneer de control cycle is doorlopen: ...
    canMsg.can_id  = 5;                       //ID = 5 (ER oke)
    canMsg.can_dlc = 1;                       //CAN data length is 1 byte
    canMsg.data[0] = 1;                       //Oke in [0]
 
    mcp2515.sendMessage(&canMsg);             //Sends the CAN message
    i = 0;
  }
  
  delay(5);                                   //Kleine delay voor de stabiliteit van het programma
}
