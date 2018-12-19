#include <FastLED.h>

/* hw_ui_pixelcontroller: A simple pixel controller application with 
 * a strictly "hardware based" user interface, in the sense that the behavior 
 * of the attached pixel string can be controlled via the presence or absence
 * of at most 3 M-M jumper wires with no additional components needed. This is
 * convenient when you wish to modify the behavior on a semi-regular basis
 * but don't have access to a suitable computer at such location for example.
 * 
 * The 3 jumpers may be connected between ground and either 0, 1, or both 
 * brightness select pins (BSEL_PINx), as well as between ground and any one
 * function select (FSEL_PIN) pin. The behaviors triggered by such are as follows:
 * 
 * - The connection of a ground jumper to the lower BSEL_PIN increases 
 *   the brightness from the base brightness of 2^6-1 units by 2^6 units,
 *   out of a max of 2^8-1 units
 *   
 * - The connection of a ground jumper to the upper BSEL_PIN increases
 *   the brightness from the current level by an additional 2^7 units.
 *   
 * - The connection of a ground jumper to an FSEL_PIN, which includes N pins
 *   starting from and inclusive of the BASE_FSEL_PIN, where N equals the
 *   number of defined patterns (in this case 6) will cause the controller to
 *   continuously display the pattern at index (pin_number - BASE_FSEL_PIN). 
 *   
 * - If no FSEL_PINs are grounded, each pattern will be displayed for one minute, 
 *   cycling through all patterns sequentially.
 * 
 * The default configuration is setup for a set of 50 WS2811 pixels,
 * but may easily be changed below.
 * 
 * Copyright (c) 2018 Nash Kaminski, released under MIT license
 * Portions Copyright (c) 2013 FastLED, released under MIT license
 */

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    12
#define BASE_FSEL_PIN 2
#define BSEL_PIN1 10
#define BSEL_PIN2 11
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    50
CRGB leds[NUM_LEDS];

#define FRAMES_PER_SECOND  120

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// List of patterns. Each is defined as a separate function pointer below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { red, multi, blue, green, white, rainbow };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gCycle = 0; // Should the pattern be advanced automatically?
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

int8_t functionSelect(void){
  for(int i=0; i<ARRAY_SIZE(gPatterns); i++){
    //Find the first FSEL pin that is low, if none are low, return -1
    if(!digitalRead(BASE_FSEL_PIN+i)){
      return i;
    }
  }
  return -1;
}

void setup() {
  // 1 second powerup delay.
  delay(1000);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  // All function select pins to input with pullup mode
  for(int i=BASE_FSEL_PIN; i<BASE_FSEL_PIN+ARRAY_SIZE(gPatterns); i++){
    pinMode(i, INPUT);
    digitalWrite(i, HIGH);
  }
  
  // Same for brightness select pins
  pinMode(BSEL_PIN1, INPUT);
  digitalWrite(BSEL_PIN1, HIGH);
  pinMode(BSEL_PIN2, INPUT);
  digitalWrite(BSEL_PIN2, HIGH);
}
  
void loop()
{
  uint8_t brightness;
  int8_t function;
  
  // Set the function if an FSEL pin is grounded, else cycle through all.
  function = functionSelect();
  if(function == -1){
    if(!gCycle){
     gCycle = 1;
     gCurrentPatternNumber = 0;
     nextPattern();
    }
  } else {
    gCycle = 0;
    gCurrentPatternNumber = function;
  }

  // Set the brightness base on the BSEL pins
  brightness = (((~digitalRead(BSEL_PIN2)) & 0x01) << 7) | (((~digitalRead(BSEL_PIN1)) & 0x01) << 6) | 0x3F;
  FastLED.setBrightness(brightness);
  
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest, needs to be relatively high for flicker-free dithering
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do periodic tasks
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 60 ) { nextPattern(); } // change patterns periodically, noop if gCycle == 0
}



void nextPattern()
{
  // add one to the current pattern number and wrap around at the end, but only if gCycle is set
  if(!gCycle)
    return;
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void red() {
  // All Red
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(0,255,0);
  }
}

void green() {
  // All Green
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(255,0,0);
  }
}

void blue() {
  // All Blue
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(0,0,255);
  }
}

void white() {
  // All Blue
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(255,255,255);
  }
}

void multi() {
  // Multi color
  for( int i = 0; i < NUM_LEDS; i++) {
    switch(i % 5){
      case 0:
      leds[i] = CRGB(80,255,0);
      break;
      case 1:
      leds[i] = CRGB(0,0,255);
      break;
      case 2:
      leds[i] = CRGB(255,0,0);
      break;
      case 3:
      leds[i] = CRGB(0,100,255);
      break;
      case 4:
      leds[i] = CRGB(0,255,0);
      break;
    }
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}
