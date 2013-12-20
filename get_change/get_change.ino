// get compass state

#include <Wire.h>
#include <HMC5883L.h>
#include <Streaming.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <RFBeeSendRev.h>
#include <RFBeeCore.h>


const long LIGHT_ON_TIME = 3;			                            // 5s, 亮灯时间， 改变这里

const long LIGHT_ON_TIME_WITHOUTBACK =  1000*30*60;//5*60*1000;     // 如果没有回来，xx分钟后照常关灯

#define __Dbg           1                                           // 是否需要调试信息，如果不需要，改成0

// 几种状态
#define ST_IDLE         0               // 有拖鞋静止在附近, 等待
#define ST_CHANGE       4               // 变化
#define ST_ONLEAVE      1               // 离开区域，亮着灯
#define ST_ONHERE       2               // 在区域亮灯

#define HERE            0               // 原地
#define LEAVE           1               // 离开


#define BEEPON()		digitalWrite(5, HIGH)       // 蜂鸣器开
#define BEEPOFF()		digitalWrite(5, LOW)        // 蜂鸣器关


// Store our compass as a variable.
HMC5883L compass;

const int numReadings = 10;

// 用的是一个平滑滤波，参考这里：http://arduino.cc/en/Tutorial/Smoothing
long readings[numReadings];             // the readings from the analog input
int index       = 0;                    // the index of the current reading
long total      = 0;                    // the running total
long average    = 0;                    // the average


int state = ST_IDLE;                    // 状态

int val_origin = 0;                     // 初始值

int cnt_move = 0;

// 蜂鸣器响20ms
void beep()
{
    BEEPON();
    delay(20);
    BEEPOFF();
}


// 判断是否离开，如果传感器值和初始值相差小于3，认为离开了
// 返回1：离开， 返回0： 没有里开
int isLeave()        
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

// 发送命令亮灯，发送五个o
void lightOn()
{

	unsigned char dtaUart[] = "ooooo";
	RFBEE.sendDta(5, dtaUart);
}

// 发送命令关灯，发送五个c
void lightOff()
{

	unsigned char dtaUart[] = "ccccc";
	RFBEE.sendDta(5, dtaUart);
}


// 状态切换
void stChg(int st)
{
    state = st;
    cnt_move = 0;
    
    char str[4][10] = {"IDLE", "ONLEAVE", "ONHERE", "CHANGE"};
    
#if __Dbg
    cout << "STATE CHANGE -> " << str[st] << endl;
#endif
}

// 数据缓冲区最大值和最小值的差
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

// 是否移动，如果divOfReadings()返回大于10，认为有移动
int isMove()
{

#if __Dbg
    cout << divOfReadings() << endl;
#endif
    if(divOfReadings()>10)return 1;
    return 0;
}

// 等待静止，如果3s内 isMove()返回的数据为0，说明已经静止了
int waitStop()                             // wait for stop
{
    int cnt = 0;
    while(1)
    {
        pushDta();
        cnt = isMove() ? 0 : cnt+1;
        if(cnt > 30)
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

// 状态机函数，管理状态之间的切换
void stateMachine()
{
    
    int cnt1 = 0;
    
    
    switch(state)
    {
        // ------------------------------------- 静止 --------------------------------
        case ST_IDLE:
        
        cnt_move = 0;
        if(isLeave() || isMove())               // 是否离开或者有移动
        {
		
			int flg_tmp=0;
			
			for(int i=0; i<10; i++)             
			{
				pushDta();
				delay(67);
				if(isLeave() || isMove())       // 再次判断是否离开或者有移动，以防止干扰
				{
					flg_tmp++;                  // 检测到一次，flg_tmp自增
					
					if(flg_tmp > 2)             // 当flg_tmp大于2的时候，认为真的离开或者真的有移动
					break;
				}
			}
			
			if(flg_tmp<3)return;                // 误判，返回
            
			BEEPON();                           // 蜂鸣器响，作为提示
			delay(20);
			BEEPOFF();
			
            lightOn();                          // 开灯
            
            if(waitStop())                      // 等待静止
            {
                stChg(ST_ONLEAVE);              // 状态切换到灯亮，离开状态
                timer_leave = millis();
                
            }
            else
            {
                stChg(ST_ONHERE);               // 状态切换到灯亮，没有离开状态     
            }
        }
        
        break;
        
        // ------------------------------------- 变化 --------------------------------
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
        
        // ------------------------------------- 灯亮，离开 --------------------------------
        case ST_ONLEAVE:

        if(isMove())                                            // 是否有移动
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
        
        if(dt_leave > (LIGHT_ON_TIME_WITHOUTBACK))            // 如果xxx时间没有回来，可能是触发失败或者睡不着看电视去了，关灯。
        {
#if __Dbg
            cout << "no back, turn off anyway" << endl;
#endif
            lightOff();
            stChg(ST_IDLE);
        }
        
        if(!isLeave())                                      // 判断是否离开，（或者回来了没）
        {                                                   // 下面的代码主要是为了保证是不是判断错了
            while(1)                                        // 如果发现应该是ST_ONHERE的，那么切换过去
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
        
        // ------------------------------------- 灯亮，没有离开 --------------------------------
        case ST_ONHERE:
        
        for(int i=0; i<(LIGHT_ON_TIME*10); i++)             // 开始倒计时， 准备关灯
        {
            pushDta();
            delay(100);
            
            if(isMove())                                    // 如果有动
            {
                
                cnt_move++;
                
                if(cnt_move > 10)
                {
                    stChg(ST_CHANGE);
                    return;
                }
            }
            
            if(isLeave())                                   // 突然离开了
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
        lightOff();                                             // 顺利计时完毕，关灯睡觉
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
	
	int cnt_1 = 0;
    
	while(1)                                // 等待拖鞋回来
	{
		if(!isLeave())
		{
			cnt_1++;
		}
		
		if(cnt_1 > 10)
		{
			break;
		}
		
		pushDta();
		delay(100);
	}
	
	waitStop();
	beep();
	
}

// Our main program loop.


void loop()
{


    pushDta();                                      // 采集一个数据
    
    stateMachine();                                 // 状态机
    
    delay(100);                                     // 延时

}

// 采集数据，采用了平滑滤波算法
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

//初始化电子罗盘
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

// 获得电子罗盘数据，0-3600， 表示0-360度，为了提高精度，乘10了
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