#include <libraries/Trill/Trill.h>
#include <Arduino.h>
#include <PdArduino.h>
# include <libraries/math_neon/math_neon.h>

// Trill sensor and constants
Trill gTouchSensor;

//Pin assignment for port on Bella
const int onoffpin = 0;

// Threshold for trill sensor
const float THRESHOLD = 0.022;


// Time intervals for preventing rapid triggering
const int HIGH_NOTE_INTERVAL = 500;
const int HIGH2_NOTE_INTERVAL = 500;
const int VU_INTERVAL = 10000;
const int TRIANGLE_INTERVAL=1000;
const int FLUTE_INTERVAL=1000;
const int TAP_INTERVAL=200;

const int FADE_INTERVAL = 50; //brightness refresh period of the LED, higher the value the longer it will take to change brightness
const float BRIGHTNESS_STEP =0.1; //THe changes of brightness in each period


const int numReadings = 35; //The number of readings used to Smoothing out one accelerometer data more the readings will be smoothed, but the slower the output will respond to the input.
float xReadings[numReadings];//Array stores raw data from accelerometer ready to be Smoothing
float yReadings[numReadings];//Array stores raw data from accelerometer ready to be Smoothing
int xIndex = 0;//Index that keep track of the location of latest x reading in the array
int yIndex = 0;//Index that keep track of the location of latest y reading in the array
float xTotal = 0; //Pool of readings to be averaged
float yTotal = 0;//Pool of readings to be averaged

// Last trigger times for each button
int high2LastTime = 0;
int highLastTime = 0;
int bassLastTime = 0;
int vuLastTime = 0;
int slowLastTime = 0;
int triangleLastTime= 0;
int tap1LastTime=0;
int tap2LastTime=0;
int lastLedChange=0;
int fluteLastTime=0;
int smoothLastTime=0;


bool firstTriggered = false;//Triggering state of base track 

//Pins of accelerometer
int accelerometerPinX = 0;
int accelerometerPinY = 1;    

//state of LED pin
enum LightingState {
    LIGHTING_UP,
    STAY_BRIGHT,
    DARKEN,
    NOT_SHINING,
};

//arrays that stores the pin number, brightness and last update time and mode for LED
int ledPin[9] = {6, 0, 1, 8, 10, 3, 5, 7, 2};
float brightness[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int ledLastUpdate[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
LightingState lightState[9] = {NOT_SHINING, NOT_SHINING, NOT_SHINING, NOT_SHINING, NOT_SHINING, NOT_SHINING, NOT_SHINING, NOT_SHINING, NOT_SHINING};


unsigned long speedCount = 1; // speed of playing back samples
unsigned long buttonPressedTime = 0; //Duration of speed vaariaion button been pressed
bool isButtonPressed = false; //state of pressing speed variation button
bool light_disabled=false;// //state of on and off of the global pins


void setup()
{
	 // Set up On/Off button pin
	 pinMode(onoffpin, INPUT);
	// Set up a Trill Craft on I2C bus 1, using the default address.
	if(gTouchSensor.setup(1, Trill::CRAFT) != 0) {
		fprintf(stderr, "Unable to initialise Trill Craft\n");
		exit(1);
		return;
	}
	 // Set prescaler and update baseline for Trill sensor
	gTouchSensor.setPrescaler(1);
	delay(50);
	gTouchSensor.updateBaseline();
	delay(50);
	
	//reset the playing rate
	pdSendMessage("pdRate",1);
	
	//reset the array
	for (int i = 0; i < numReadings; i++) 
	{
	    xReadings[i] = 0;
	    yReadings[i] = 0;
	}


}

void loop()
{
	//Gainning current time
	unsigned int now = millis();
	
	// Ask for the latest readings from Trill Craft
	gTouchSensor.readI2C();
	
	//Start playing music if button on and off switch in the button or on the playground has been switch
	if (gTouchSensor.rawData[23]>THRESHOLD or digitalRead(12)==0)
	{
		pdSendMessage("pdOnoff",1);
		//reset the speed of playing to 1
		speedCount=1;
		if (gTouchSensor.rawData[23]>THRESHOLD)// lighting up the led only if the top on and off switch are triggerd
		{
			lightState[4]=LIGHTING_UP;
		}
		light_disabled=true;
		gTouchSensor.updateBaseline();
	}
	else
	{
		pdSendMessage("pdOnoff",0);
		light_disabled=false;
	}

	// Check values of selected sensors if sound effect sensor has been triggerd and not triggered repetitively,if triggered play respective sound effect
	if (gTouchSensor.rawData[28]>THRESHOLD && now-highLastTime >HIGH_NOTE_INTERVAL)
	{
		pdSendMessage("pdHighNotes",1);
		highLastTime =millis();
		lightState[1]=LIGHTING_UP;
	}	
	if (gTouchSensor.rawData[13]>THRESHOLD && now-high2LastTime >HIGH2_NOTE_INTERVAL)
	{
		pdSendMessage("pdHigh2",1);
		high2LastTime = millis();
		lightState[7]=LIGHTING_UP;

	}
	if (gTouchSensor.rawData[22]>THRESHOLD && now-fluteLastTime>FLUTE_INTERVAL)
	{
		pdSendMessage("pdFlu",1);
		Serial.println("flute");
		lightState[0]=LIGHTING_UP;
		fluteLastTime=millis();
	}
	if (gTouchSensor.rawData[10]>THRESHOLD && now-triangleLastTime >TRIANGLE_INTERVAL)
	{
		pdSendMessage("pdTriangle",1);
		Serial.println("Triangle");
		triangleLastTime=millis();
		lightState[3]=LIGHTING_UP;
	}

	if (gTouchSensor.rawData[1]>THRESHOLD && now-tap1LastTime >TAP_INTERVAL)
	{
		pdSendMessage("pdTap1.1",1);
		Serial.println("tap1");
		tap1LastTime=millis();
	}
	
	if (gTouchSensor.rawData[11]>THRESHOLD && now-tap2LastTime >TAP_INTERVAL)
	{
		pdSendMessage("pdTap2.1",1);
		Serial.println("tap2");
		tap2LastTime=millis();
	}
	
	//Record the duratiom when the speeding up pin has bin pressed and stay on top
	if (gTouchSensor.rawData[29] > 0.017 && now-buttonPressedTime >100)
	{
		if (!isButtonPressed)
		{
		  // Record the time when the button is first pressed
		  buttonPressedTime = millis();
		  isButtonPressed = true;
		  Serial.println("Button pressed.");
		  lightState[8]=LIGHTING_UP;
		}
		
		// Calculate the holding time continuously
		speedCount = speedCount+200;
		lightState[8]=LIGHTING_UP;
		
		// Set the maximum speed
		if (speedCount>15000)
		{
		speedCount=15000;
		}
		//map speedcount to the range of playback speed
		pdSendMessage("pdRate",map(speedCount,0,5000,1,1.2));
		pdSendMessage("pdScratch",gTouchSensor.rawData[29]);
	}
	else
	{
		if (isButtonPressed) 
		{
		// Clear the press state when the button is released
		isButtonPressed = false;
		Serial.println("Button released.");
		}
		//lowering the speed if button is not beening trigger
		speedCount=speedCount-0.01;
		pdSendMessage("pdRate",map(speedCount,0,5000,1,1.2));
		//preventing the speed of playing go below 1
		if (speedCount<=1)
		{
			speedCount=1;
	}
	}
	
	// Condition for triggering drum track
	if (gTouchSensor.rawData[19]>0.018 && now-vuLastTime>500)
	{
		pdSendMessage("pdVu",1);
		vuLastTime = millis();
		Serial.println("drum track up");
		lightState[2]=LIGHTING_UP;
		firstTriggered=true;
	}
	//drum trak will keep on after been trigger for once for a interval of time
	else if (now-vuLastTime<VU_INTERVAL && firstTriggered ==true && digitalRead(12)==1)
	{
		pdSendMessage("pdVu",1);
		lightState[2]=STAY_BRIGHT;
	}
	//drum trak disable
	else
	{
		pdSendMessage("pdVu", 0);
	}

	// Smoothing out readings from the accelerometer to reduce noise
	// Check if it's time to update the smoothed readings (every 5 milliseconds)
	if (now - smoothLastTime > 5)
	{
	    // Read raw accelerometer values
	    float xReading = analogRead(accelerometerPinX);
	    float yReading = analogRead(accelerometerPinY);
	
	    // Subtract the oldest readings from the total
	    xTotal -= xReadings[xIndex];
	    yTotal -= yReadings[yIndex];
	
	    // Store the new readings in the circular buffer
	    xReadings[xIndex] = xReading;
	    yReadings[yIndex] = yReading;
	
	    // Add the new readings to the total
	    xTotal += xReading;
	    yTotal += yReading;
	
	    // Update the location of the latest reading, ensuring the index doesn't overrun the array size
	    xIndex = (xIndex + 1) % numReadings;
	    yIndex = (yIndex + 1) % numReadings;
	
	    // Calculate the average readings
	    float xSmoothed = xTotal / numReadings;
	    float ySmoothed = yTotal / numReadings;
	
	    // Map the smoothed values to a desired range (adjust these values based on your specific sensor characteristics)
	    float xLocate = map(xSmoothed, 0.397, 0.472, 0, 7);
	    float yLocate = map(ySmoothed, 0.395, 0.47, 0, 7);
	
	    // Fine-tune the location by adding small offsets
	    xLocate += 0.01;
	    yLocate += 0.1;
	
	    // Send the smoothed and adjusted accelerometer values to a Pure Data patch
	    pdSendMessage("pdX", xLocate);
	    pdSendMessage("pdY", yLocate);
	
	    // Calculate and send the squared magnitude of the location vector to the Pure Data patch
	    pdSendMessage("clip", powf_neon(xLocate, 2) + powf_neon(yLocate, 2));
	
	    // Update the last time the smoothing was performed
	    smoothLastTime = millis();
	}

	
	
	// Loop through LED pins and update their state
	for (int i = 0; i < 9; i++) 
	{
    // Access the brightness value at index i
	    //PWM write to respecti1 led according to there mode
		if (lightState[i]==NOT_SHINING)
		{
			brightness[i]=0;
			pwmWrite(ledPin[i],brightness[i]);
		}
		//Constantly adding brightness every period when the led are in LIGHTING_UP mode
		if (lightState[i]==LIGHTING_UP && now-ledLastUpdate[i] >FADE_INTERVAL)
		{
			brightness[i]=brightness[i]+BRIGHTNESS_STEP;
			pwmWrite(ledPin[i],brightness[i]);
			ledLastUpdate[i]=millis();
		}
		//Constantly decreasing brightness every period when the led are in LIGHTING_UP mode
		if (lightState[i]==DARKEN && now-ledLastUpdate[i] >FADE_INTERVAL/2)
		{
			brightness[i]=brightness[i]-BRIGHTNESS_STEP;
			pwmWrite(ledPin[i],brightness[i]);
			ledLastUpdate[i]=millis();
		}
		if (lightState[i]==STAY_BRIGHT)
		{
			brightness[i]=1;
			pwmWrite(ledPin[i],100);
		}
		//Set led to there darken mode when they lighting up and exceed the maximum brightness
		if (brightness[i]>=1)
		{
			lightState[i]=DARKEN;
		}
		//Set led to there not shining mode when they darken to much 
		if (brightness[i]<=0)
		{
			lightState[i]=NOT_SHINING;
	}
	//set all the light off if on&off swithes are set to off
	if (light_disabled==true)
	{
		for (int i = 0; i < 9; i++)
		{
			pwmWrite(ledPin[i],0);
		}
	}
}
}


