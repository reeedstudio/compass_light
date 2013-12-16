// get compass state

#include <Wire.h>
#include <HMC5883L.h>
#include <Streaming.h>

#define ST_IDLE         1               // 有拖鞋
#define ST_MOVE         2               // 在变化
#define ST_LEAVE        4               // 离开区域
#define ST_LIGHTON      8               // 亮灯



// Store our compass as a variable.
HMC5883L compass;

const int numReadings = 10;

long readings[numReadings];         // the readings from the analog input
int index = 0;                      // the index of the current reading
long total = 0;                     // the running total
long average = 0;                   // the average

int cnt_open = 0;


int state = ST_IDLE;

int val_origin = 0;                 // 初始值

int isThereSlipper()                // 1: have slipper, 0: no slipper
{
    int tmp = average - val_origin;
    if(abs(tmp) < 3)return 0;
    return 1;
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

int isMove()
{
    
}

void stateMachine()
{
    switch(state)
    {
        case ST_IDLE:
        
        
        break;
        
        case ST_MOVE:
        
        
        
        break;
        
        
        case ST_LEAVE:
        
        break;
        
        
        case ST_LIGHTON:
        
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
#if 1
    total= total - readings[index];         
    // read from the sensor:
    
    
    long tmp = getCompass();
    


    readings[index] = abs(tmp-average)>300 ? average : tmp;
    
    cout << readings[index] << '\t';
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
  
    cout << average << endl << endl;
    
    
    cout << "div = " << divOfReadings() << endl;
    
    delay(70);
    
#else

    cout << getCompass() << endl; 
    delay(100);            
    //of course it can be delayed longer.
#endif
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
        delay(100);
    }
    
    val_origin = total/10;
    
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