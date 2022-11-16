#include <SPI.h>                              //Library voor  SPI Communicatie
#include <mcp2515.h>                          //Library voor CAN Communicatie
#include <LiquidCrystal.h>                    //Library voor LCD scherm

struct can_frame canMsg;                      //Maak de canMsg aan

MCP2515 mcp2515(10);                          //CS zit op pin 10

int modeButton = 8;                           //De mode button zit op pin 8
int lastMode = 0;                             //De vorige modus, ter vergelijking
int currentMode = 0;                          //Huidige modus signaal      
int MODE = 0;                                 //1:ECR 0:Bridge
int state = 0;                                //De state wordt ontvangen vanuit de machinekamer

int ERok = 0;                                 //Oke signal vanuit de machinekamer
int BRok = 0;                                 //Oke signal vanaf de brug
int i = 0;                                    //Teller control cycle
int controlcycle = 210;                       //Control cycle lengte
int ERfout = 0;                               //ER fout
int BRfout = 0;                               //Bridge fout

float throttleA = 0;                          //Nieuwe throttle signaal
float throttleB = 0;                          //Oude throttle signaal
float difference = 0;                         //Verschil tussen oud en nieuw throttle signaal
float throttleBridge = 0;                     //Throttle signaal ontvangen vanaf de brug
                                              
bool hasSentA = false;                        //maak hasSent intial = false

const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2; //Def de LCD pinnen
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);    //LCD pinnen

/*Variabelen voor het running average filter*/
const int numReadings = 10;
int readings[numReadings];                    //De gelezen waardes vanaf de analoge iput
int readIndex = 0;                            //The index of the current reading
int total = 0;                                //The running total
int average = 0;                              //Het gemiddelde
int inputPin = A0;                            //Analoge input zit op pin A0


void setup() {
  Serial.begin(9600);                         //Start seriele communicatie         
  SPI.begin();                                //Begin SPI communicatie
  lcd.begin(16, 2);                           //Initialiseer het aantal kolommen en rijen van de LCD
  lcd.clear();                                //Clear de LCD
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);  //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                    //Sets CAN at normal mode

  pinMode(modeButton, INPUT);                 //De mode button is een input

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {   //Voeg readings toe aan de reeks, tot een maximum van numReadings (10)
    readings[thisReading] = 0;
  }
  

}

void loop() {
/*Modus selectie*/
  lastMode = currentMode;                        //Maak de lasMode
    if(abs(throttleB-throttleBridge) <= 5){      //Wanneer de throttles op de bridge en in de ECR
    currentMode = digitalRead(modeButton);       //Maak een nieuwe currentMode
    if(lastMode == LOW && currentMode == HIGH){  //Vergelijk de last en current mode ... 
      MODE = !MODE;                              //... wanneer deze verschillen, inverteer de operating mode 
    }
  }
  if(BRfout == 1){                             //Wanneer de brug niet actief is ... 
    MODE = 1;                                  //... schakel over naar ECR control mode
  }

/*zend modus*/
  if(MODE == HIGH){                            //Wanneer de mode HIGH (ECR) is, ...
    if(hasSentA == false){                     //... en deze nog niet is verzonden
      canMsg.can_id  = 2;                      //ID = 2 (van de mode setting)
      canMsg.can_dlc = 1;                      //CAN data length is 1 byte
      canMsg.data[0] = MODE;                   //Update MODE value in [0]
 
      mcp2515.sendMessage(&canMsg);            //Sends the CAN message

      hasSentA = true;                         //Frame is verzonden
    }
  }
 
  if(MODE == LOW){                             //Wanneer de mode LOW (BRIDGE) is, ..
    if(hasSentA == true){                      //En dit nog niet is verzonden..
      canMsg.can_id  = 2;                      //ID = 2 (van de mode setting)
      canMsg.can_dlc = 1;                      //CAN data length is 1 byte
      canMsg.data[0] = MODE;                   //Update state value in [0]
 
      mcp2515.sendMessage(&canMsg);            //Sends the CAN message

      hasSentA = false;                        //Frame is verzonden   
    }
  }
  
/*ontvang dataframes via canbus*/
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK){ // To receive data (Poll Read)
    if(canMsg.can_id == 1){                    //Wanneer het een status signaal betreft ...
      state = canMsg.data[0];                  //Schrijf de waarde naar de status
    }else if(canMsg.can_id == 4){              //Wanneer het een throttle signaal vanaf de brug betreft ...
      throttleBridge = canMsg.data[0];         //Schrijf het in de daarvoor bestemde variabele
    }else if(canMsg.can_id == 5){              //Wanneer het een ER oke signaal betreft...
      ERok = canMsg.data[0];                   //Schrijf het naar ERok
    }else if(canMsg.can_id == 6){              //Wanneer het een Bridge oke signaal betreft...
      BRok = canMsg.data[0];                   //Schrijf het naar BRok
    }
  }

/*Running average filter*/
  total = total - readings[readIndex];          // subtract the last reading
  readings[readIndex] = analogRead(inputPin);   // read from the sensor
  total = total + readings[readIndex];          // add the reading to the total
  readIndex = readIndex + 1;                    // advance to the next position in the array
  if (readIndex >= numReadings) {               // if we're at the end of the array...
    readIndex = 0;                              // ...wrap around to the beginning:
  }
  average = total / numReadings;                // calculate the average

/*Zend ECR throttle signaal*/
  throttleA = average/10.2;                     //Verschaal naar 0-100%
  difference = abs(throttleA - throttleB);      //Het verschil tussen de vorige en de huidige setting
  if(difference >= 1){                          //Wanneer deze groot genoeg is...
    throttleB = throttleA;                      //... maak een nieuwe setting aan
    
    canMsg.can_id  = 3;                         //ID = 3 (van de ECR throttle setting)
    canMsg.can_dlc = 1;                         //CAN data length van 1 byte
    canMsg.data[0] = throttleB;                 //Update state value in [0]
 
    mcp2515.sendMessage(&canMsg);               //Sends the CAN message
  }

/*Control cycle*/  
  i = i + 1;                                    //Tel door in de controle cycle
  if(i <= 5){                                   //in het begin van de cycle ...
    ERok = 0;                                   //... maak ERok en BRok 0
    BRok = 0;
  }else if(i >= controlcycle){                  //Aan het eind van de cycle ...
    if(ERok == 0){                              //... controleer of het oke signaal is ontvangen van de machinekamer
      ERfout = 1;
    }else if(ERok == 1){
      ERfout = 0;
    }
    if(BRok == 0){                              //... controleer of het oke signaal is ontvangen van de brug
      BRfout = 1;
    }else if(BRok == 1){
      BRfout = 0;
    }
  i = 0;                                        //laat de cycle weer opnieuw beginnen
  }

/*LCD weergave*/
  if(BRfout == 0){                              //Wanneer de brug oke is
    if(MODE == HIGH){                           //Wanneer de mode HIGH (ECR control) is ...
      lcd.setCursor(11,1);
      lcd.print(throttleB);                     //... geef de throttle setting van de ECR weer
      lcd.setCursor(15,1);  
      lcd.print("%");
      lcd.setCursor(0,0);
      lcd.print("ECR mode   ");                 //Geef weer dat de ECR mode actief is
     }else{                                     //Wanneer de mode LOW (BRIDGE control) is ...
        lcd.setCursor(11,1);
        lcd.print(throttleBridge);              //... geef de throttle setting van de Bridge weer
        lcd.setCursor(15,1);
        lcd.print("%");
        lcd.setCursor(0,0);
        lcd.print("BRIDGE mode");               //Geef weer dat de BRIDGE mode actief is
       }
  }else if(BRfout == 1){                        //Wanneer de brug faalt
    lcd.setCursor(0,0);
    lcd.print("BRIDGE fail  ");                 //Laat zien: bridge fail
    lcd.setCursor(11,1);
    lcd.print(throttleB);                       //Weergeef throttle setting van de ECR
    lcd.setCursor(15,1);
    lcd.print("%");
  }

  if(ERfout == 0){                              //Wanneer de machinekamer oke is
    if(state == HIGH){                          //Wanneer de status HIGH (running) is, ...
      lcd.setCursor(0,1);
      lcd.print("motor on  ");                  //... geef dit weer
    }else{                                      //Wanneer deze LOW (stopped) is, ...
      lcd.setCursor(0,1);     
      lcd.print("motor off");                   //... geef dat ook weer
    }
  }else if(ERfout == 1){                        //Wanneer de machinekamer faalt
    lcd.setCursor(0,1);
    lcd.print("ER fail  ");                     //Laat zien: ER fail
  }
  

  delay(5);                                     //Kleine delay voor stabiliteit van het programma

}
