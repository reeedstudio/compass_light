// demo of rfbee send and recv 
// loovee 2013
#include <Arduino.h>
#include <EEPROM.h>
#include <RFBeeSendRev.h>
#include <RFBeeCore.h>

unsigned char dtaUart[100];             // serial data buf
unsigned char dtaUartLen = 0;           // serial data length

/*********************************************************************************************************
** Function name:           setup
** Descriptions:            setup
*********************************************************************************************************/
void setup(){

    pinMode(10, OUTPUT);
    
    pinMode(A2, OUTPUT);                // mosfet output low
    pinMode(A3, OUTPUT);
    digitalWrite(A2, LOW);
    digitalWrite(A3, LOW);
    
    
    RFBEE.init();
    Serial.begin(115200);
    Serial.println("ok");
	
	pinMode(5, OUTPUT);
	digitalWrite(5, LOW);
}

unsigned char rxData1[200];               // data len
unsigned char len1;                       // len
unsigned char srcAddress1;
unsigned char destAddress1;
char rssi1;
unsigned char lqi1;
int result1;

unsigned char cntGetDta = 5;


void openLight()
{
	digitalWrite(5, HIGH);
}

void closeLight()
{
	digitalWrite(5, LOW);
}

/*********************************************************************************************************
** Function name:           loop
** Descriptions:            loop
*********************************************************************************************************/
void loop()
{
    if(RFBEE.isDta())                       // rfbee get data
    {
        result1 = receiveData(rxData1, &len1, &srcAddress1, &destAddress1, (unsigned char *)&rssi1 , &lqi1);
        for(int i = 0; i< len1; i++)
        {
            Serial.write(rxData1[i]);
			
			if('o' == rxData1[i])			// get open
			{
				openLight();
			}
			else if('c' == rxData1[i])		// get close
			{
				closeLight();
			}
        }
		

    }


}

void serialEvent() 
{
    while (Serial.available()) 
    {
        dtaUart[dtaUartLen++] = (unsigned char)Serial.read();  
    }

}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/