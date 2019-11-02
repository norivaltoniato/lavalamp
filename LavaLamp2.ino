
/**
 * Lavalamp 3.0 
 * 
 * 2019 by Norival Toniato
 * 
 * Based on Mario Keller's work (http://www.youtube.com/watch?v=6jfAZXOzzec)
 * 
 * 
 * Hardware: 
 *    - NodeMCU (ESP8266)
 *    - 20 x APA102 RGB LEDs
 *    - 1 Rotary Encoder
 *    - 5V / 2A Powersupply
 *    
 *    NODEMCU PINS
 *    D1 = APA102 - Data
 *    D2 = APA102 - Clock
 *    D5 = Rotary Encoder SW
 *    D6 = Rotary Encoder DT
 *    D7 = Rotary Encoder CLK
 *    
 **/


 
/**** 
 *  Encoder
 */  
#include <MD_REncoder.h>
#define ENCODER_A   D6
#define ENCODER_B   D7
#define ENCODER_SW  D5
#define ENCODER_UP 1
#define ENCODER_DOWN 0
#define ACTION_CLICK 1
#define ACTION_DOUBLECLICK 2
#define ACTION_HOLD 3
#define ACTION_LONGHOLD 4
MD_REncoder R = MD_REncoder(ENCODER_A, ENCODER_B);
#define VALUE_MIN 2
#define VALUE_MAX 254
#define BRIGHTNESS_MIN 2
#define BRIGHTNESS_MAX 254


int encoderStep=3;
/**** 
 *  Init
 */  
int encoderPos = 0;
int encoderPinALast = LOW;
unsigned long lastMillis = 0;
int doubleClickLength = 500;
int debounceLimit = 100;
unsigned long debounceMilis = 0;
int pos = 0;
// Button timing variables
int debounce = 20; // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 250; // max ms between clicks for a double click event
int holdTime = 2000; // ms hold period: how long to wait for press+hold event
int longHoldTime = 5000; // ms long hold period: how long to wait for press+hold event

// Other button variables
boolean buttonVal = HIGH; // value read from button
boolean buttonLast = HIGH; // buffered value of the button's previous state
boolean DCwaiting = false; // whether we're waiting for a double click (down)
boolean DConUp = false; // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true; // whether it's OK to do a single click
long downTime = -1; // time the button was pressed down
long upTime = -1; // time the button was released
boolean ignoreUp = false; // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false; // when held, whether to wait for the up event
boolean holdEventPast = false; // whether or not the hold event happened already
boolean longHoldEventPast = false;// whether or not the long hold event happened already
boolean resetEncoderOnEnd = false;
boolean changeFunction = false;


#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

#define DEBUG         1   // Debug modes: 0-Off, 1-normal, 2-verbose
#define ACTION_CLICK 1
#define ACTION_DOUBLECLICK 2
#define ACTION_HOLD 3
#define ACTION_LONGHOLD 4
#define MODE_MAX 7

//pin 4 is the output pin 
#define DATA_PIN     D1
#define CLOCK_PIN   D2
#define NUM_LEDS    20
#define LED_TYPE    APA102
#define COLOR_ORDER BGR

#define A1 1
#define A2 2

#define KNOB_MODE_VALUE 0
#define KNOB_MODE_BRIGHT 1
#define KNOB_MAX 1


CRGB leds[NUM_LEDS];

int brightness = 125;

//some initial values
int a0,a1 =0;
int oldvalue = -1;
int oldbright = -1;

int value = 30;
int mode = 0;
int knobMode = 0;

void setup() {
  setupEnvironment();
  setupLeds();
  setupEncoder();
  (DEBUG > 0) && Serial.println ( "Setup concluido" );
}



void setupEncoder() {
  R.begin();
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  pinMode(ENCODER_SW, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), loopEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), loopEncoder, CHANGE);
//  attachInterrupt(digitalPinToInterrupt(ENCODER_SW), checkButton, FALLING);
}
/**
 * just check the current mode and switch to according function
 *
 **/
void loop(){
  changeFunction = false;
//  int action = checkButton();
//  if (action >= ACTION_HOLD) {
//    (DEBUG > 0) && Serial.print ( "Action triggered state:" );
//    (DEBUG > 0) && Serial.println ( action );  
////    return;
//    fireHoldAction();
//  }

//  changeMode();
  switch(mode) {
  case 0:
    color();
    break;
  case 1:
    rainbow();
    break; 
  case 2:
    rainbow2();
    break;
  case 3:
    rainbow3();
    break;
  case 4:
    rainbow4();
    break; 
  case 5:
    rainbow5();
    break;
  case 6:   
    bubbles(); 
    break;
//  case 7:
//    lotery();     
//    break;    
  case 7:
    white();     
    break;    
  }

}

void loopEncoder() {
  uint8_t x = R.read();
  if ((x == DIR_CW) || (x == DIR_CCW)) {
    bool isCW = (x == DIR_CW);
    if (isCW) {
      (DEBUG > 1) && Serial.println ( "CW" );
      if (knobMode==KNOB_MODE_VALUE) {
        value+=encoderStep;
      } else if (knobMode==KNOB_MODE_BRIGHT) {
        brightness+=encoderStep;
      }
    } else {
      (DEBUG > 1) && Serial.println ( "CCW" );
      if (knobMode==KNOB_MODE_VALUE) {
        value-=encoderStep;
      } else if (knobMode==KNOB_MODE_BRIGHT) {
        brightness-=encoderStep;
      }
    }

    if (knobMode==KNOB_MODE_VALUE) {
      if (resetEncoderOnEnd) {
        if (value > VALUE_MAX) {
          value = VALUE_MIN;
        } else if (value < VALUE_MIN) {
          value = VALUE_MAX;
        }
      } else {
        if (value > VALUE_MAX) {
          value = VALUE_MAX;
        } else if (value < VALUE_MIN) {
          value = VALUE_MIN;
        }
      }
      (DEBUG > 0) && Serial.print ( "Value: " );
      (DEBUG > 0) && Serial.println ( value );
    } else if (knobMode==KNOB_MODE_BRIGHT) {
      if (brightness > BRIGHTNESS_MAX) {
        brightness = BRIGHTNESS_MAX;
      } else if (brightness < BRIGHTNESS_MIN) {
        brightness = BRIGHTNESS_MIN;
      }
      FastLED.setBrightness( brightness );
      FastLED.show();
      (DEBUG > 0) && Serial.print ( "Brightness: " );
      (DEBUG > 0) && Serial.println ( brightness );
    }
  }
}

void fireClick() {
  mode++;
  if (mode>MODE_MAX) mode=0;
  changeFunction=true;
  (DEBUG > 0) && Serial.print ( "Click: " );
  (DEBUG > 0) && Serial.println ( mode );
}

void fireDoubleClick() {
  knobMode++; 
  if (knobMode>KNOB_MAX) {
    knobMode = 0;
  } 
  if (knobMode==KNOB_MODE_VALUE) {
    (DEBUG > 1) && Serial.print ( "[2] Value mode" );
  } else if (knobMode==KNOB_MODE_BRIGHT) {
    (DEBUG > 1) && Serial.print ( "[2] Brightness mode " );
  }  
}

void fireHold() {
  (DEBUG > 1) && Serial.print ( "[2] Brightness mode " );
  encoderStep++; 
  if (encoderStep>3) {
    encoderStep = 1;
  } 
  FastLED.setBrightness(50);
  for(int i = 0; i < encoderStep; i++ ){
    for(int j = 0; j < NUM_LEDS; j++ ){
      leds[j] = CRGB::Black;
    }
    FastLED.show();
    delay(200);
    for(int j = 0; j < NUM_LEDS; j++ ){
      leds[j] = CRGB::White;
    }
    FastLED.show();
    delay(100);
  }
  FastLED.setBrightness(brightness);
  FastLED.show();
}


void setupEnvironment() {
//  pinMode(ENCODER_SW, INPUT);
  pinMode(DATA_PIN, OUTPUT);
  Serial.begin (9600);
  delay( 1000 ); // power-up safety delay
}

void setupLeds() {
  (DEBUG > 1) && Serial.println ( "SetupLeds" );
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(brightness);
  FastLED.show();
}

/**
 * make analogRead() produce "smoothed" values by having the
 * current value only have a small influence on the returned value
 **/
int readAnalog(int prevvalue, int pin) {

  //what we do here is to use the previous value (7/8) and 
  //the current value (1/8) to create the new value
  prevvalue = ( prevvalue *7) +analogRead(pin);
  prevvalue = prevvalue / 8;
  
  //some spacial adjustments for my configuration.
  //limiting the returned values
  if(prevvalue < 20 ) prevvalue = 20;
  if(prevvalue > 996 ) prevvalue = 996;

  return prevvalue;
}


/**
 * set all LEDs to "white" (same value for all colors)
 **/
void lotery() {
  int oldvalue = -1;
  for(int j = 0; j < NUM_LEDS; j++ ){
    leds[j] = CRGB::Black;
  }
  
  //run infinite until mode changes
  while(!changeFunction) {
    //gat current value of second knob for brightness
    if (value==255) {
      //only reset LEDs if value has changed
      if(value != oldvalue) {
        for(int j = 0; j < NUM_LEDS; j++ ){
          setRGB(value,value,value,j);
        }
        FastLED.show();
      }
    } else {
        for(int j = 0; j < NUM_LEDS; j++ ){
          leds[j] = CRGB::White;
        }
        delay(30);
        for(int j = 0; j < NUM_LEDS; j++ ){
          leds[j] = CRGB::Black;
        }
        delay(30);
    }
  }
}

/**
 * set all LEDs to "white" (same value for all colors)
 **/
void white() {
  (DEBUG > 1) && Serial.println ( "WHITE" );
  int oldvalue = -1;
  FastLED.setTemperature( Candle ); // first temperature
  int myValue;

  
  //run infinite until mode changes
  for(int j = 0; j < NUM_LEDS; j++ ){
    leds[j] = CRGB::White;  
  }
  FastLED.show();
  while(!changeFunction) {
    //only reset LEDs if value has changed
    if(value != oldvalue) {
      FastLED.setBrightness( value );
      FastLED.show();
      brightness = value;
    }
    myDelay(value);
    (DEBUG > 1) && Serial.println ( "Debug" );
  }
}

/**
 * set all LEDs to the same color
 **/
void color() {
  (DEBUG > 1) && Serial.println ( "COLOR" );
  resetEncoderOnEnd = true;
  while(!changeFunction) {
    for(int j = 0; j < NUM_LEDS; j++ ){
      setHue2(revertValue(),j); 
    }
    FastLED.show();
    myDelay(value);
  }
}

int revertValue() {
  return map(value,0,255,255,2);   
}

/**
 * run through the color-circle (HUE value) 
 * all LEDs are set to the same color value
 * delay() ist set by the value of the second potentiometer
 **/
void rainbow()
{ 
  (DEBUG > 1) && Serial.println ( "RAINBOW" );
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for(int i = 0; i < 255; i++)
    {
      for(int j=0;j< NUM_LEDS; j++)
        setHue2(i,j);
      FastLED.show();
      myDelay(revertValue());
//      delay(revertValue());
//      if( changeFunction ) return;
    }
  }
}

bool myDelay(int delayLength) {
  unsigned long targetMilis;
  targetMilis = millis() + delayLength;
  int event;
  while (targetMilis > millis()) {
    event = checkButton();
    if (event!=0) {
      switch (event) {
      case 1: 
        fireClick();
        break;
      case 2: 
        fireDoubleClick();
        break;
      case 3: 
        fireHold();
        break;
      case 4:
        fireHold();
        break;
      }
    }
    yield();
  } 
  return false;
}

/**
 * run through the color-circle (HUE value) 
 * every row is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbow2(){
  (DEBUG > 1) && Serial.println ( "RAINBOW2" );
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for(int i = 255; i >= 0; i--) {
      for (int col = 0; col < 4; col++) {
        for(int row = 0; row < 5; row++) {
          int v = ((10*row) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      myDelay(revertValue()/3);
      if( changeFunction ) continue;
    }
  }

}

/**
 * run through the color-circle (HUE value) 
 * every LED in a row is set to the next color
 * this creates a spiral effect of colors climbing up the lamp
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbow3(){
  (DEBUG > 1) && Serial.println ( "RAINBOW3" );
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    for(int i = 255; i >= 0; i--) {
      int v = i;
      for (int row = 0; row < 5; row++) {
        for(int col = 0; col < 4; col++) {
          v = (v + 12) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      myDelay(revertValue()/3);
      if( changeFunction ) continue;
    }
  }

}

/**
 * run through the color-circle (HUE value) 
 * every column is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbow4(){
  (DEBUG > 1) && Serial.println ( "RAINBOW4" );
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    //counting down creates a clockwise rotation
    for(int i = 255; i >= 0; i--) {
      for (int row = 0; row < 5; row++) {
        for(int col = 0; col < 4; col++) {
          int v = ((10*col) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      myDelay(revertValue()/3);
      if( changeFunction ) continue;
    }
  }

}

/**
 * run through the color-circle (HUE value) 
 * every column is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbow5(){
  (DEBUG > 1) && Serial.println ( "RAINBOW5" );
  resetEncoderOnEnd = false;
  while(!changeFunction) {
    //counting up creates a counter clockwise rotation
    for(int i = 0; i < 256; i++) {
      for (int row = 0; row < 5; row++) {
        for(int col = 0; col < 4; col++) {
          int v = ((10*col) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      myDelay(revertValue()/3);
      if( changeFunction ) continue;
    }
  }
}

/**
 * create a array of "bubbles" in different colors on all columns
 * the climb up the lamp with different speed
 * speed is controlled by the value of the second potentiometer
 **/
void bubbles() {
  (DEBUG > 1) && Serial.println ( "BUBBLES" );
  resetEncoderOnEnd = false;
  
  //how many bubbles do we need
  #define NUM_BUBBLES 12

  //prepare arrays with column, row, color and speed of each bubble
  byte bubblecol[NUM_BUBBLES];
  byte bubblerow[NUM_BUBBLES];
  byte bubblespeed[NUM_BUBBLES];
  byte bubblehue[NUM_BUBBLES];

  //initialize the bubble-positions
  byte numbubbles = 0;
  for(int i=0;i<NUM_BUBBLES;i++) {
    //columns number of "0" means "inactive" bubble
    bubblecol[i] = 0;
    //starting position is "under" the first row
    bubblerow[i] = -1;
  }
 
  int stepcounter =0;

  //run infinite until mode has changed
  while(!changeFunction) {
    //first check if all bubbles are "active"
    //if not, create a new bubble
    if(numbubbles < NUM_BUBBLES) {
      //find fist "incactive bubble"
      for(int i=0;i<NUM_BUBBLES;i++) {
        if(bubblecol[i]==0) {
          //count up number of active bubbles
          numbubbles++;
          //set columns of the new bubble
          bubblecol[i]=random(1,5);
          //start "under" the first row
          bubblerow[i] = -1;
          //set speed
          bubblespeed[i]=random(1,11);
          //set color
          bubblehue[i]=random(0,256);
          break;
        }
        if( changeFunction ) continue;
      }
      if( changeFunction ) continue;
    }
    stepcounter++;
    
    //now run to all "active" bubbles an let them climb up one row
    //but only if the should, according to their speed
    for(int i=0;i<NUM_BUBBLES;i++) {
      if(bubblecol[i]!=0) {
        if(stepcounter % bubblespeed[i] == 0) {
          bubblerow[i]++;
          //check if bubble has reached upper border and has to be deleted
          if(bubblerow[i] > 4) {
            bubblecol[i] = 0;
            bubblerow[i] = 0;
            numbubbles--;
            setRGB(0,0,0,(5*(bubblecol[i]-1) + 4));
          } 
          else {
            if(bubblerow[i] > 0) setRGB(0,0,0,(5*(bubblecol[i]-1) + bubblerow[i]-1));
            setHue2(bubblehue[i],(5*(bubblecol[i]-1) + bubblerow[i]));
            FastLED.show();
          }
        }
      }
      if( changeFunction ) continue;
    }
    myDelay(revertValue());
  }  
}

/**
 * function to set a specific LED in the chain to a given value of the HVS color-circle
 *
 * this is simplified version of HVS to RGB comversion
 * see: http://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB
 *
 * in all cases saturation and value are "1" so we do not have to calculate all parts
 *
 * we also use a range of 0 to 255 instead of 0 to 360 for HUE
 **/
void setHue2(int h, int pin) {
  //this is the algorithm to convert from RGB to HSV
  double r=0; 
  double g=0; 
  double b=0;

  double hf=h/42.5;

  //calculate with 255 instead of 360° for the whole color-circle so we have to divide by 42.5 instead of 60°
  int i=(int)floor(h/42.5);
  double f = h/42.5 - i;
  double qv = 1 - f;
  double tv = f;

  switch (i)
  {
  case 0: 
    r = 1;
    g = tv;
    break;
  case 1: 
    r = qv;
    g = 1;
    break;
  case 2: 
    g = 1;
    b = tv;
    break;
  case 3: 
    g = qv;
    b = 1;
    break;
  case 4:
    r = tv;
    b = 1;
    break;
  case 5: 
    r = 1;
    b = qv;
    break;
  }

  //set each component to a integer value between 0 and 255
  leds[pin].r = constrain((int)255*r,0,255);
  leds[pin].g = constrain((int)255*g,0,255);
  leds[pin].b = constrain((int)255*b,0,255);

}

/**
 * function to set a specific LED in the chain to concrete R, G and B values
 *
 **/
void setRGB(int r, int g, int b, int pin) {
  //set each component to a integer value between 0 and 255
  leds[pin].r = r;
  leds[pin].g = g;
  leds[pin].b = b;

}

int checkButton() {
  int event = 0;
  // Read the state of the button
  buttonVal = digitalRead(ENCODER_SW);
  // Button pressed down
  if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce) {
    downTime = millis();
    ignoreUp = false;
    waitForUp = false;
    singleOK = true;
    holdEventPast = false;
    longHoldEventPast = false;
    if ((millis() - upTime) < DCgap && DConUp == false && DCwaiting == true) DConUp = true;
    else DConUp = false;
    DCwaiting = false;
  }
  // Button released
  else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce) {
    if (not ignoreUp) {
      upTime = millis();
      if (DConUp == false) DCwaiting = true;
      else {
        event = 2;
        DConUp = false;
        DCwaiting = false;
        singleOK = false;
      }
    }
  }
  // Test for normal click event: DCgap expired
  if ( buttonVal == HIGH && (millis() - upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true) {
    event = 1;
    DCwaiting = false;
  }
  // Test for hold
  if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
    // Trigger "normal" hold
    if (not holdEventPast) {
      event = 3;
      waitForUp = true;
      ignoreUp = true;
      DConUp = false;
      DCwaiting = false;
      //downTime = millis();
      holdEventPast = true;
    }
    // Trigger "long" hold
    if ((millis() - downTime) >= longHoldTime) {
      if (not longHoldEventPast) {
        event = 4;
        longHoldEventPast = true;
      }
    }
  }
  buttonLast = buttonVal;
  return event;
}
