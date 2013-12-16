// get compass state

#include <Wire.h>
#include <HMC5883L.h>
#include <Streaming.h>

#define ST_IDLE         0               // 有拖鞋, 等待
#define ST_ONLEAVE      1               // 离开区域，亮着灯
#define ST_ONHERE       2               // 在区域亮灯



#define HERE            0 
#define LEAVE           1

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



int isLeave()                    // 1:leave  0:here
{
    int tmp = average - val_origin;
    
    cout << "average    = " << average << endl;
    cout << "val_origin = " << val_origin << endl;
    cout << "div_tmp    = " << abs(tmp) << endl;
    if(abs(tmp) < 3)return LEAVE;
    return HERE;
}


void lightOn()
{
    cout << "Light On" << endl;
}

void lightOff()
{
    cout << "Light Off" << endl;
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
    
    char str[3][10] = {"IDLE", "ONLEAVE", "ONHERE"};
    cout << "STATE CHANGE -> " << str[st] << endl;
}

int isMove()
{
    if(divOfReadings()>20)return 1;
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
            cout << "move stop" << endl;
            return isLeave();
        }
        delay(100);
    }
}


void stateMachine()
{
    switch(state)
    {
        case ST_IDLE:
        
        if(isMove())
        {
            lightOn();
            if(waitStop())          // if leave
            {
                stChg(ST_ONLEAVE);
            }
            else
            {
                stChg(ST_ONHERE);
            }
        }
        
        break;
        
        
        
        case ST_ONLEAVE:

        if(isMove())
        {
            stChg(ST_IDLE);
            return;
        }
        
        break;
        
        
        case ST_ONHERE:
        
        for(int i=0; i<50; i++)
        {
            pushDta();
            delay(100);
            
            if(isMove())
            {
                stChg(ST_IDLE);
                return;
            }
        }
        
        cout << "goto bed" << endl;
        lightOff();
        stChg(ST_IDLE);

        break;
        
        default:
        
        ;
    }
}

void setup()
{
    // Initialize the serial port.
    Serial.begin(115200);
    
    delay(1000);
    
    initCompass();
}

// Our main program loop.


void loop()
{

    //cout << pushDta() << endl;
    
    //cout << "div = " << divOfReadings() << endl;
    
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

    //cout << readings[index] << endl;
    // add the reading to the total:
    
    total = total + readings[index];   

    //cout << "total = " << total << endl; 
    
    // advance to the next position in the array:  
    
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
        Serial.println(compass.getErrorText(error));
    }
    
    error = compass.setMeasurementMode(MEASUREMENT_CONTINUOUS);     // Set the measurement mode to Continuous
    
    if(error != 0)                                                  // If there is an error, print it out.
    {
        Serial.println(compass.getErrorText(error));
    }
    
    for (int thisReading = 0; thisReading < numReadings; thisReading++)
    {
        readings[thisReading] = getCompass(); 
        total += readings[thisReading];
        delay(70);
    }
    
    
    for(int i=0; i<20; i++)
    {
        pushDta();
        delay(70);
    }
    
    val_origin = average;
    
    cout << "val_origin = " << val_origin << endl;
    
    cout <<"init ok" << endl;
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