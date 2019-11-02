/**
 * Lavalamp 2.0 
 * http://arduino.cc/forum/index.php/topic,151444.0.html
 * http://www.youtube.com/watch?v=6jfAZXOzzec
 * 
 * 2013 by Mario Keller
 * 
 * Hardware: 
 *    - Arduino pro mini 328 5V / 16MHz
 *    - 20 x WS2811 RGB Controller with 2812 RGB LED 
 *    - 2 x 10k potentiometer 
 *    - 5V / 2A Powersupply
 *
 **/

#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

#define DEBUG         1   // Debug modes: 0-Off, 1-normal, 2-verbose
#define ACTION_CLICK 1
#define ACTION_DOUBLECLICK 2
#define ACTION_HOLD 3
#define ACTION_LONGHOLD 4
#define MODE_MAX 8

//pin 4 is the output pin 
#define DATA_PIN     D1
#define CLOCK_PIN   D2
#define NUM_LEDS    20
#define LED_TYPE    APA102
#define COLOR_ORDER GRB

#define BUTTON_PIN D2

#define A1 1
#define A2 2

#define KNOB_MODE_VALUE 0
#define KNOB_MODE_BRIGHT 1


CRGB leds[NUM_LEDS];

int brightness = 200;

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
int pos = 0;


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
  (DEBUG > 0) && Serial.println ( "Setup concluido" );
}

/**
 * just check the current mode and switch to according function
 *
 **/
void loop(){

  changeMode();
  value = getVal();
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
  case 7:
    lotery();     
    break;    
  case 8:
    white();     
    break;    
  }

}


void setupEnvironment() {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(DATA_PIN, OUTPUT);
  Serial.begin (9600);
  delay( 1000 ); // power-up safety delay
  (DEBUG > 1) && Serial.println ( DATA_PIN );
}

void setupLeds() {
  (DEBUG > 1) && Serial.println ( "SetupLeds" );
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
}

bool changeMode() {
  (DEBUG > 1) && Serial.println ( mode );
  int buttonState = checkButton();
  (DEBUG > 1) && Serial.println ( buttonState );
  bool result = false;
  if (buttonState == ACTION_CLICK) {
    mode++;
    if (mode==MODE_MAX+1) mode=0;
    result = true;
    (DEBUG > 1) && Serial.println ( "CLICK" );
  } else if (buttonState == ACTION_DOUBLECLICK) {
    if (knobMode == KNOB_MODE_BRIGHT) {
      knobMode = KNOB_MODE_VALUE;
    } else {
      knobMode = KNOB_MODE_BRIGHT;
    }
    (DEBUG > 1) && Serial.println ( "DOUBLE_CLICK" );
  }
  
  return result;
}

/**
 * read the second knob on A1 to get the current value for
 * some modes (speed, color, brightness)
 **/
int getVal() {
//  if (knobMode == KNOB_MODE_VALUE) {
//    a1 = readAnalog(a1,A0);
//  
//    //my potentiometers provide values from 1023 to 0 instead of 0 to 1023
//    //so I have to swap the mapping
//    int x =  map(a1,996,20,0,255);
//  
//    //eliminate jumping values to prevent flickering colors or brightness
//    if(abs(oldvalue - x) > 1 ) {
//      oldvalue = x;     
//    }
//    value = x;
//  }
  return value;  
}

void setBright() {
  if (knobMode == KNOB_MODE_BRIGHT) {
    int a1 = readAnalog(a1,A0);
  
    //my potentiometers provide values from 1023 to 0 instead of 0 to 1023
    //so I have to swap the mapping
    int x =  map(a1,20,128,2,255);
  
    //eliminate jumping values to prevent flickering colors or brightness
    if(abs(oldbright - x) > 1 ) {
      oldbright = x;     
    }
    FastLED.setBrightness( x );
  }
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
  while(1) {
    
    //gat current value of second knob for brightness
    value = getVal();
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
    if( changeMode() ) break;
    yield();

  }
}

/**
 * set all LEDs to "white" (same value for all colors)
 **/
void white() {
  int oldvalue = -1;
  FastLED.setTemperature( Candle ); // first temperature

  
  //run infinite until mode changes
  while(1) {
    
    //gat current value of second knob for brightness
    value = getVal();
    value =  map(value,0,255,255,2);
    
    
    //only reset LEDs if value has changed
    if(value != oldvalue) {
      for(int j = 0; j < NUM_LEDS; j++ ){
        setRGB(value,value,value,j);
      }
      FastLED.show();
    }
    if( changeMode() ) break;
    yield();

  }
}

/**
 * set all LEDs to the same color
 **/
void color() {
  while(1) {
    value = getVal();
    value =  map(value,0,255,0,255);
    setBright();
    for(int j = 0; j < NUM_LEDS; j++ ){
      setHue2(value,j); 
    }

    FastLED.show();
    if( changeMode()) break;
    yield();

  }
}

/**
 * run through the color-circle (HUE value) 
 * all LEDs are set to the same color value
 * delay() ist set by the value of the second potentiometer
 **/
void rainbow()
{ 
  while(1) {
    for(int i = 0; i < 255; i++)
    {
      for(int j=0;j< NUM_LEDS; j++)
        setHue2(i,j);
      FastLED.show();
      setBright();
      delay(getVal());
      if( changeMode() ) return;
    }

    if( changeMode() ) return;
  }
}


/**
 * run through the color-circle (HUE value) 
 * every row is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbow2(){
  while(1) {
    for(int i = 255; i >= 0; i--)
    {
      for (int col = 0; col < 4; col++) {
        for(int row = 0; row < 5; row++) {
          int v = ((10*row) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      setBright();
      delay(getVal()/3);
      if( changeMode() ) return;
    }

    if( changeMode() ) return;
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
  while(1) {
    for(int i = 255; i >= 0; i--)
    {
      int v = i;
      for (int row = 0; row < 5; row++) {
        for(int col = 0; col < 4; col++) {
          v = (v + 12) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      setBright();
      delay(getVal()/3);
      if( changeMode() ) return;
    }

    if( changeMode() ) return;
  }

}

/**
 * run through the color-circle (HUE value) 
 * every column is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbow4(){
  while(1) {
    //counting down creates a clockwise rotation
    for(int i = 255; i >= 0; i--)
    {
      for (int row = 0; row < 5; row++) {
        for(int col = 0; col < 4; col++) {
          int v = ((10*col) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      setBright();
      delay(getVal()/3);
      if( changeMode() ) return;
    }

    if( changeMode() ) return;
  }

}

/**
 * run through the color-circle (HUE value) 
 * every column is set to a new color
 * delay() ist set by the value of the second potentiometer
 *
 **/
void rainbow5(){
  while(1) {
    //counting up creates a counter clockwise rotation
    for(int i = 0; i < 256; i++)
    {
      for (int row = 0; row < 5; row++) {
        for(int col = 0; col < 4; col++) {
          int v = ((10*col) + i) % 255;
          setHue2(v,(5*col + row));        
        } 
      }

      FastLED.show();
      setBright();
      delay(getVal()/3);
      if( changeMode() ) return;
    }

    if( changeMode() ) return;

  }

}

/**
 * create a array of "bubbles" in different colors on all columns
 * the climb up the lamp with different speed
 * speed is controlled by the value of the second potentiometer
 **/
void bubbles() {
  
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
  while(1) {
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
        if( changeMode() ) return;
      }
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
      if( changeMode() ) return;
    }
    setBright();
    int x = map(getVal(), 0, 255, 10,255);
    delay(x);
    if( changeMode() ) break;
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
  buttonVal = digitalRead(BUTTON_PIN);
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
