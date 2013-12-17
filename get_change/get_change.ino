// get compass state

#include <Wire.h>
#include <HMC5883L.h>
#include <Streaming.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <RFBeeSendRev.h>
#include <RFBeeCore.h>


const long LIGHT_ON_TIME = 5;			// 5s, 亮灯时间， 改变这里

const long LIGHT_ON_TIME_WITHOUTBACK =  1000*20;//5*60*1000;           // 如果没有回来，5分钟后照常关灯

#define __Dbg           1

#define ST_IDLE         0               // 有拖鞋, 等待
#define ST_CHANGE       4               // 有变化
#define ST_ONLEAVE      1               // 离开区域，亮着灯
#define ST_ONHERE       2               // 在区域亮灯

#define HERE            0 
#define LEAVE           1


#define BEEPON()		digitalWrite(5, HIGH)
#define BEEPOFF()		digitalWrite(5, LOW)


// Store our compass as a variable.
HMC5883L compass;

const int numReadings = 10;

long readings[numReadings];             // the readings from the analog input
int index       = 0;                    // the index of the current reading
long total      = 0;                    // the running total
long average    = 0;                    // the average

int cnt_open = 0;


int state = ST_IDLE;

int val_origin = 0;                     // 初始值

long time_tmp=0;

int cnt_move = 0;

void beep()
{
    BEEPON();
    delay(20);
    BEEPOFF();
}


int isLeave()                    // 1:leave  0:here
{
    int tmp = average - val_origin;

#if __Dbg
    cout << "average    = " << average << endl;
    cout << "val_origin = " << val_origin << endl;
    cout << "div_tmp    = " << abs(tmp) << endl;
#endif

    if(abs(tmp) < 3)return LEAVE;
    return HERE;
}


void lightOn()
{

	unsigned char dtaUart[] = "ooooo";
	RFBEE.sendDta(5, dtaUart);
}

void lightOff()
{

	unsigned char dtaUart[] = "ccccc";
	RFBEE.sendDta(5, dtaUart);
}

int divOfReadings()
{
    int max = 0;
    int min = 5000;
    
    for(int i=0; i<numReadings; i++)
    {
        if(max <= readings[i])max = readings[i];
        if(min >= readings[i])min = readings[i];
    }
    

    return (max - min);
}

void stChg(int st)
{
    state = st;
    cnt_move = 0;
    
    char str[4][10] = {"IDLE", "ONLEAVE", "ONHERE", "CHANGE"};
    
#if __Dbg
    cout << "STATE CHANGE -> " << str[st] << endl;
#endif
}

int isMove()
{

#if __Dbg
    //cout << divOfReadings() << endl;
#endif
    if(divOfReadings()>10)return 1;
    return 0;
}

//1:leave   0:here
int waitStop()                             // wait for stop
{
    int cnt = 0;
    while(1)
    {
        pushDta();
        cnt = isMove() ? 0 : cnt+1;
        if(cnt > 50)
        {
#if __Dbg
            cout << "move stop" << endl;
#endif
            return isLeave();
        }
        delay(100);
    }
}

long timer_leave = 0;
long dt_leave = 0;


void stateMachine()
{
    
    int cnt1 = 0;
    
    
    switch(state)
    {
        case ST_IDLE:
        
        cnt_move = 0;
        if(isMove())
        {
		
            delay(67);
            pushDta();
            delay(67);
            
            if(!isMove())return;
            
			BEEPON();
			delay(20);

			BEEPOFF();
			
            lightOn();
            if(waitStop())          // if leave
            {
                stChg(ST_ONLEAVE);
                timer_leave = millis();
                
            }
            else
            {
                stChg(ST_ONHERE);
            }
        }
        
        break;
        
        case ST_CHANGE:
        
        cnt_move = 0;
        
        BEEPON();
        delay(20);
        BEEPOFF();
            
        if(waitStop())          // if leave
        {
            stChg(ST_ONLEAVE);
            timer_leave = millis();
                
        }
        else
        {
            stChg(ST_ONHERE);
        }

        
        break;
        
        case ST_ONLEAVE:

        if(isMove())
        {
            cnt_move++;
            if(cnt_move > 10)
            {
                stChg(ST_CHANGE);
                return;
            }
        }
        
        dt_leave = millis() - timer_leave;
        
        cout << "dt_leave = " << dt_leave << endl;
        
        if(dt_leave > (600000))            // 如果10分钟没有回来，可能是触发失败，关灯。
        {
#if __Dbg
            cout << "no back, turn off anyway" << endl;
#endif
            lightOff();
            stChg(ST_IDLE);
        }
        
        if(!isLeave())
        {
            while(1)
            {
                pushDta();
                delay(100);
                
                if(!isLeave())
                {
                    cnt1++;
                }
                else
                {
                    return ;
                }
                
                if(cnt1 > 10)
                {
                    stChg(ST_ONHERE);
                    BEEPON();
                    delay(20);
                    BEEPOFF();
                    return;
                }
            }
        }
       
        
        break;
        
        
        case ST_ONHERE:
        
        for(int i=0; i<(LIGHT_ON_TIME*10); i++)
        {
            pushDta();
            delay(100);
            
            if(isMove())
            {
                
                cnt_move++;
                
                if(cnt_move > 10)
                {
                    stChg(ST_CHANGE);
                    return;
                }
            }
            
            if(isLeave())
            {
                while(1)
                {
                    pushDta();
                    delay(100);
                    
                    if(isLeave())
                    {
                        cnt1++;
                    }
                    else
                    {
                        return ;
                    }
                    
                    if(cnt1 > 5)
                    {
                        stChg(ST_ONLEAVE);
                        timer_leave = millis();
                        return;
                    }
                }
            }
        
        }
#if __Dbg 
        cout << "goto bed" << endl;
#endif
        lightOff();
        stChg(ST_IDLE);

        break;
        
        default:
        
        ;
    }
}

void setup()
{

	pinMode(10, OUTPUT);
	pinMode(5, OUTPUT);
	digitalWrite(5, LOW);
    RFBEE.init();
    
    pinMode(A2, OUTPUT);            // mosfet output low
    pinMode(A3, OUTPUT);
    digitalWrite(A2, LOW);
    digitalWrite(A3, LOW);
	
    lightOff();
    delay(1500);
    
    beep();

#if __Dbg
    Serial.begin(115200);
#endif

    initCompass();
    
    
    beep();
    delay(100);
    beep();
    
    
    delay(500);
}

// Our main program loop.



void loop()
{


    pushDta();
    
    stateMachine();

    delay(100);

}


int pushDta()
{


    total= total - readings[index];         
    // read from the sensor:
    
    long tmp = getCompass();
 
    readings[index] = abs(tmp-average)>300 ? average : tmp;

    
    total = total + readings[index];   

    index = index + 1;          

    //cout << "index = " << index << endl;

    // if we're at the end of the array...
    if (index >= numReadings)              
    index = 0;                           

    // calculate the average:
    average = total / numReadings;
    
    return average;

}

void initCompass()
{
    
    Wire.begin();                                                   // Start the I2C interface.
    
    delay(5);
    
    int error = compass.setScale(1.3);                              // Set the scale of the compass.
    
    if(error != 0)                                                  // If there is an error, print it out.
    {
        //Serial.println(compass.getErrorText(error));
    }
    
    error = compass.setMeasurementMode(MEASUREMENT_CONTINUOUS);     // Set the measurement mode to Continuous
    
    if(error != 0)                                                  // If there is an error, print it out.
    {
        //Serial.println(compass.getErrorText(error));
    }
    
    for (int thisReading = 0; thisReading < numReadings; thisReading++)
    {
        readings[thisReading] = getCompass();
        total += readings[thisReading];
        delay(100);
    }
    
    
    
    for(int i=0; i<40; i++)
    {
        pushDta();
        delay(100);
    }
    
    val_origin = average;
    
#if __Dbg
    cout << "val_origin = " << val_origin << endl;
    cout <<"init ok" << endl;
#endif
}

int getCompass()
{
    MagnetometerRaw raw = compass.readRawAxis();
    // Retrived the scaled values from the compass (scaled to the configured scale).
    MagnetometerScaled scaled = compass.readScaledAxis();

    // Values are accessed like so:
    int MilliGauss_OnThe_XAxis = scaled.XAxis;// (or YAxis, or ZAxis)

    // Calculate heading when the magnetometer is level, then correct for signs of axis.
    float heading = atan2(scaled.YAxis, scaled.XAxis);

    // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
    // Find yours here: http://www.magnetic-declination.com/
    // Mine is: -2??37' which is -2.617 Degrees, or (which we need) -0.0456752665 radians, I will use -0.0457
    // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
    float declinationAngle = -0.0457;
    heading += declinationAngle;

    // Correct for when signs are reversed.
    if(heading < 0)
    heading += 2*PI;

    // Check for wrap due to addition of declination.
    if(heading > 2*PI)
    heading -= 2*PI;

    // Convert radians to degrees for readability.
    float headingDegrees = heading * 180/M_PI;

    // Output the data via the serial port.
    
    int degree = headingDegrees*10;
    
    return degree;
}