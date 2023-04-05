#include <Adafruit_NeoPixel.h>
#include <FastLED.h> 
#include <DMXSerial.h>

#include "water_torture.h"
int buttonPin = 7;    // momentary push button on pin 0
int oldButtonVal = 0;
#define NUM_LEDS 20
#define N_STEPS   8
#define LEFT_IN   A0  //Left audio channel
#define RIGHT_IN  A1  //Right audio channel
#define LEFT_OUT  5
#define RIGHT_OUT 6
#define PEAK_FALL 6 // Rate of peak falling dot
#define PEAK_FALL1 6 // Rate of peak falling dot
#define PEAK_FALL2 4 // Rate of peak falling dot
#define PEAK_FALL3 6 // Rate of peak falling dot
#define PEAK_FALL4 20 // Rate of peak falling dot
#define SAMPLES   32//100// Length of buffer for dynamic level adjustment  
#define COLOR_ORDER GRB
#define COLOR_ORDER2 BGR
#define LED_TYPE WS2812B
#define maxsteps 16
#define NUM_LEDS_HALF (NUM_LEDS/2)
#define DC_OFFSET  -512 // DC offset in mic signal - if unusure, leave 0
#define NOISE    1 //20 //10 // Noise/hum/interference in mic signal
#define BRIGHTNESS 60
#define COLOR_START 0
#define COLOR_START2 179
#define COLOR_START3 255
#define COLOR_FROM 0
#define COLOR_TO 255
#define GRAVITY           -9.81              // Downward (negative) acceleration of gravity in m/s^2
#define h0                2                  // Starting height, in meters, of the ball (strip length)
#define NUM_BALLS         2                // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way)
#define SPEED .20       // Amount to increment RGB color by each cycle
#define BG 0
#define SPARKING 50
#define COOLING  55
#define FRAMES_PER_SECOND 60
#define TOP     (NUM_LEDS + 4) // Allow dot to go slightly off scale
#define INPUT_FLOOR 10         //100 //Lower range of analogRead input
#define INPUT_CEILING 120   //Max range of analogRead input, the lower the value the more sensitive (1023 = max)
#define EEPROM_MODE_ADDR 0
#define EEPROM_BRIGHTNESS_ADDR 1
#define EEPROM_COLOR_ADDR 2
#define EEPROM_CYCLE_SPEED_ADDR 5
#define qsubd(x, b)  ((x>b)?wavebright:0)                     // A digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0)       
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define NSAMPLES 64
#include <EEPROM.h>

//DMX Start
#define DMXSTART 45   // DMX Startchannel
//DMX Channel 1 -> Brightness
// DMX Channel 2 -> TopColor
// DMX Channel 3 -> BottomColor
// DMX Channel 4 -> PeakTopColor
// DMX Channel 5 -> PeakBottomColor
// DMX Channel 6 -> Program
/*
int dmxTopColor;
int dmxBottomColor;
int dmxPeakTopColor;
int dmxPeakBottomColor;*/
int dmxProgram;
//int dmxBright;

//DMX End

unsigned int sampleLeft, sampleRight;
bool gReverseDirection = false;
uint8_t timeval = 20; 
bool     samplepeak = 0;  
uint16_t sampleavg = 0;                                                         // Average of the last 64 samples.
uint16_t oldsample = 0;                                                         // Previous sample is used for peak detection and for 'on the fly' values.
CRGBPalette16 currentPalette(OceanColors_p);
CRGBPalette16 targetPalette(OceanColors_p);
TBlendType    currentBlending = LINEARBLEND;      
int samplecount = 0;
// Global visual variables used in display and other routines.
bool thisdir = 0;      
bool volatile changeParams = false;

//Parameters:
uint8_t brightness, newBrightness;
uint16_t color, newColor;
uint8_t cycleSpeed, newCycleSpeed;
unsigned long oldtime = 0;
unsigned long newtime = 0;
uint8_t numdots = 4;                                          // Number of dots in use.
uint8_t faderate = 2;                                         // How long should the trails be. Very low value = longer trails.
uint8_t hueinc = 16;                                          // Incremental change in hue between each dot.
uint8_t thishue = 0;                                          // Starting hue.
uint8_t curhue = 0; 
uint8_t thisbright = 255;                                     // How bright should the LED/display be.
uint8_t basebeat = 5; 
uint8_t max_bright = 255; 
uint8_t thisbeat =  23;
uint8_t thatbeat =  28;
uint8_t thisfade =   3;                                     // How quickly does it fade? Lower = slower fade rate.
uint8_t thissat = 255;                                     // The saturation, where 255 = brilliant colours.
uint8_t thisbri = 255; 
int peakspersec = 0;
// matrix config
int thisdelay = 10;
int thisdelays = 50;                                           // A delay value for the sequence(s)
int thishues = 95;
int thissats = 255;
int thisdirs = 0;
bool huerots = 0;   
int peakcount = 0;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

// Twinkle
float redStates[NUM_LEDS];
float blueStates[NUM_LEDS];
float greenStates[NUM_LEDS];
float Fade = 0.96;
uint8_t bgcol = 0; 

unsigned int samplearray[NSAMPLES];
unsigned long samplesum = 0;
//unsigned int sampleavg = 0;
uint8_t myfade = 255;
//config for balls
float h[NUM_BALLS] ;                         // An array of heights
float vImpact0 = sqrt( -2 * GRAVITY * h0 );  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
float vImpact[NUM_BALLS] ;                   // As time goes on the impact velocity will change, so make an array to store those values
float tCycle[NUM_BALLS] ;                    // The time since the last time the ball struck the ground
int   pos[NUM_BALLS] ;                       // The integer position of the dot on the strip (LED index)
long  tLast[NUM_BALLS] ;                     // The clock time of the last ground strike
float COR[NUM_BALLS] ; // Coefficient of Restitution (bounce damping)

//Ripple variables
int center = 0;
int step = -1;
int maxSteps = 16;
float fadeRate = 0.80;
int diff;
uint8_t colour;  
//background color
uint32_t currentBg = random(256);
uint32_t nextBg = currentBg;
int          myhue =   0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LEFT_OUT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS, LEFT_OUT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS, RIGHT_OUT, NEO_GRB + NEO_KHZ800);

// Water torture

WaterTorture water_torture1 = WaterTorture(&strip1);
WaterTorture water_torture2 = WaterTorture(&strip2);

// Modes
enum 
{
} MODE;
bool reverse = true;
int BRIGHTNESS_MAX = 80;
//int brightness = 20;

// cycle variables
int CYCLE_MIN_MILLIS = 2;
int CYCLE_MAX_MILLIS = 1000;
int cycleMillis = 20;
bool paused = false;
long lastTime = 0;
bool boring = true;

uint32_t red = strip1.Color(255, 0, 0);
uint32_t orange = strip1.Color(255, 90, 0);
uint32_t yellow = strip1.Color(255, 255, 0);
uint32_t green = strip1.Color(0, 255, 0);
uint32_t blue = strip1.Color(0, 0, 255);
uint32_t purple = strip1.Color(75, 0, 130);
uint32_t white = strip1.Color(125, 125, 125);

  // Vu meter 4  
float
  greenOffset = 30,
  blueOffset = 150;  

int nPatterns = 26;
int lightPattern = 2;

byte
//  peak      = 0,      // Used for falling dot
//  dotCount  = 0,      // Frame counter for delaying dot-falling speed
  volCount  = 0;      // Frame counter for storing past volume data
int
  reading,
  vol[SAMPLES],       // Collection of prior volume samples
  lvl       = 10,      // Current "dampened" audio level
  minLvlAvg = 0,      // For dynamic adjustment of graph low & high
  maxLvlAvg = 512;    

const int sampleWindow = 10; // Sample window width in mS (50 mS = 20Hz)
const int sampleWindow1 = 10; // Sample window width in mS (50 mS = 20Hz)
int maximum = 110;
int maximum1 = 110;
int peak; 
int peak1;
int dotCount;
int dotCount1;
unsigned int sample;
unsigned int sample1;
//intbool gReverseDirection = false;

CRGB leds[NUM_LEDS];

void setup(){
  
 // Serial.begin(9600);
  uint8_t cycleSpeedEeprom = EEPROM.read(EEPROM_CYCLE_SPEED_ADDR);
  if(cycleSpeedEeprom > 11 || cycleSpeedEeprom == 0)
    cycleSpeed = 8;
  else
    cycleSpeed = cycleSpeedEeprom;
    
   DMXSerial.init(DMXReceiver);    // DMX initializing
   
   FastLED.addLeds<WS2812B, LEFT_OUT, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
   FastLED.addLeds<WS2812B, RIGHT_OUT, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
//   FastLED.setBrightness(  BRIGHTNESS );
   LEDS.addLeds<LED_TYPE, LEFT_OUT, COLOR_ORDER>(leds, NUM_LEDS);
   LEDS.addLeds<LED_TYPE, RIGHT_OUT, COLOR_ORDER>(leds, NUM_LEDS);
  // edit //
  
  strip1.begin();
  strip1.setBrightness(BRIGHTNESS);
  strip1.show(); // Initialize all pixels to 'off'
  strip2.begin();
  strip2.setBrightness(BRIGHTNESS);
  strip2.show(); // Initialize all pixels to 'off'
  

  // initialize the BUTTON pin as an input
    pinMode(buttonPin, INPUT);
    digitalWrite(buttonPin, HIGH);  // button pin is HIGH, so it drops to 0 if pressed
 
 // setup for balls//   
    
    for (int i = 0 ; i < NUM_BALLS ; i++) {    // Initialize variables
    tLast[i] = millis();
    h[i] = h0;
    pos[i] = 0;                              // Balls start on the ground
    vImpact[i] = vImpact0;                   // And "pop" up at vImpact0
    tCycle[i] = 0;
    COR[i] = 0.90 - float(i)/pow(NUM_BALLS,2); 
    }


}


// Pattern 1 - V U METER
    void VU() {

   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1023;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

   int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;
   
   for(int i; i <= led; i++)
   {
     int color = map(i, COLOR_START, strip1.numPixels(), COLOR_FROM, COLOR_TO);
     strip1.setPixelColor(i, Wheel(color));
     
   }
   
   for(int i = strip1.numPixels() ; i > led; i--)
   {
     strip1.setPixelColor(i, 0); 
     
   }
  strip1.show();
 
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,Wheel(map(peak,0,strip1.numPixels()-1,0,255)));

   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 1023;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow1)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;
   
   for(int i; i <= led; i++)
   {
     int color = map(i, COLOR_START, strip2.numPixels(), COLOR_FROM, COLOR_TO);
     strip2.setPixelColor(i, Wheel(color));
   }
   
   for(int i = strip2.numPixels() ; i > led; i--)
   {
     strip2.setPixelColor(i, 0); 
     
   }
  strip2.show();

  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,Wheel(map(peak1,0,strip2.numPixels()-1,60,255)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL1) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = 0;
    }
}
    }

 


 void VU14() {                                                                 // Display ripples triggered by peaks.
 EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                       // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                                 // Reset the counter every second.
  }

  soundmems();

  EVERY_N_MILLISECONDS(20) {
   ripple();
     }

   show_at_max_brightness_for_power();

} // loop()


void soundmems() {                                                  // Rolling average counter - means we don't have to go through an array each time.
  newtime = millis();
  int tmp = analogRead(LEFT_IN);// - 512;
  sample = abs(tmp);

  int potin = map(analogRead(LEFT_IN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];        // Add the new sample and remove the oldest sample in the array 
  sampleavg = samplesum / NSAMPLES;                                 // Get an average
  samplearray[samplecount] = sample;                                // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                       // Update the counter for the array

  if (newtime > (oldtime + 800)) digitalWrite(13, LOW);             // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    digitalWrite(13, HIGH);
    oldtime = newtime;
  }
}  


void ripple3() {
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(bgcol, 255, sampleavg*2);  // Set the background colour.

  switch (step) {

    case -1:                                                          // Initialize ripple variables.
      center = random(NUM_LEDS);
      colour = (peakspersec*10) % 255;                                             // More peaks/s = higher the hue colour.
      step = 0;
      bgcol = bgcol+8;
      break;

    case 0:
      leds[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                                    // At the end of the ripples.
      // step = -1;
      break;

    default:                                                             // Middle of the ripples.
      leds[(center + step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade/step*2);       // Simple wrap from Marc Miller.
      leds[(center - step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade/step*2);
      step ++;                                                         // Next step.
      break;  
  } // switch step
} // ripple()
    

    void VU12() {
  uint8_t i;
  uint16_t minLvl, maxLvl;
  int n, height;

  n = analogRead(LEFT_IN);             // Raw reading from mic
  n = abs(n - 512 - DC_OFFSET);        // Center on zero
  n = (n <= NOISE) ? 0 : (n - NOISE);  // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;      // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top

  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for (i = 0; i < NUM_LEDS; i++) {
    if (i >= height) {
      strip1.setPixelColor(i, 0, 0, 0);
    } else {
      strip1.setPixelColor(i, Wheel(
        map(i, 0, strip1.numPixels() - 1, (int)greenOffset, (int)blueOffset)
      ));
    }
  }
  // Draw peak dot  
  if(peak > 0 && peak <= NUM_LEDS-1) strip1.setPixelColor(peak,Wheel(map(peak,0,strip1.numPixels()-1,30,150)));
  
   strip1.show(); // Update strip



   n = analogRead(RIGHT_IN);             // Raw reading from mic
  n = abs(n - 512 - DC_OFFSET);        // Center on zero
  n = (n <= NOISE) ? 0 : (n - NOISE);  // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;      // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top

  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for (i = 0; i < NUM_LEDS; i++) {
    if (i >= height) {
      strip2.setPixelColor(i, 0, 0, 0);
    } else {
      strip2.setPixelColor(i, Wheel(
        map(i, 0, strip2.numPixels() - 1, (int)greenOffset, (int)blueOffset)
      ));
    }
  }
  // Draw peak dot  
  if(peak > 0 && peak <= NUM_LEDS-1) strip2.setPixelColor(peak,Wheel(map(peak,0,strip2.numPixels()-1,30,150)));
  
   strip2.show(); // Update strip
 
// Every few frames, make the peak pixel drop by 1:
 
    if(++dotCount >= PEAK_FALL4) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    }
  strip.show();  // Update strip

  vol[volCount] = n;
  if (++volCount >= SAMPLES) {
    volCount = 0;
  }

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl) {
      minLvl = vol[i];
    } else if (vol[i] > maxLvl) {
      maxLvl = vol[i];
    }
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) {
    maxLvl = minLvl + TOP;
  }
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

    void VU13() {
  
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;
 
 
 
  n   = analogRead(LEFT_IN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
 
  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peak)     peak   = height; // Keep 'peak' dot at top
 
 
  // Color pixels based on rainbow gradient
  for(i=0; i<NUM_LEDS_HALF; i++) {
    if(i >= height) {              
      strip1.setPixelColor(NUM_LEDS_HALF-i-1,   0,   0, 0);  //Hintergrund LEDs
      strip1.setPixelColor(NUM_LEDS_HALF+i,   0,   0, 0);    //Hintergrund LEDs
    }
    else {
      //uint32_t color = Wheel(map(i,0,NUM_LEDS_HALF-1,30,150));
      uint32_t color = Wheel(180);
      strip1.setPixelColor(NUM_LEDS_HALF-i-1,color);
      strip1.setPixelColor(NUM_LEDS_HALF+i,color);
    }
    
  } 
 
  // Draw peak dot  
  if(peak > 0 && peak <= NUM_LEDS_HALF-1) {
    //uint32_t color = Wheel(map(peak,0,NUM_LEDS_HALF-1,30,150));
    uint32_t color = Wheel(70);
    strip1.setPixelColor(NUM_LEDS_HALF-peak-1,color);
    strip1.setPixelColor(NUM_LEDS_HALF+peak,color);
  }

      strip1.show(); // Update strip




     n   = analogRead(RIGHT_IN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
 
  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peak)     peak   = height; // Keep 'peak' dot at top
 
 
  // Color pixels based on rainbow gradient
  for(i=0; i<NUM_LEDS_HALF; i++) {
    if(i >= height) {              
      strip2.setPixelColor(NUM_LEDS_HALF-i-1,   0,   0, 0);
      strip2.setPixelColor(NUM_LEDS_HALF+i,   0,   0, 0);
    }
    else {
      //uint32_t color = Wheel(map(i,0,NUM_LEDS_HALF-1,30,150));
      uint32_t color = Wheel(180);
      strip2.setPixelColor(NUM_LEDS_HALF-i-1,color);
      strip2.setPixelColor(NUM_LEDS_HALF+i,color);
    }
    
  } 
 
  // Draw peak dot  
  if(peak > 0 && peak <= NUM_LEDS_HALF-1) {
    //uint32_t color = Wheel(map(peak,0,NUM_LEDS_HALF-1,30,150));
    uint32_t color = Wheel(70);
    strip2.setPixelColor(NUM_LEDS_HALF-peak-1,color);
    strip2.setPixelColor(NUM_LEDS_HALF+peak,color);
  }
  
   strip2.show(); // Update strip
 
// Every few frames, make the peak pixel drop by 1:
 
    if(++dotCount >= PEAK_FALL4) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    }
 
 
 
  vol[volCount] = n;                      // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
 
  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl)      minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
 
}

    void VU11() {
 
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;
  
  n   = analogRead(RIGHT_IN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
 
  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peak)     peak   = height; // Keep 'peak' dot at top
 
 
  // Color pixels based on rainbow gradient
  for(i=0; i<NUM_LEDS; i++) {
    if(i >= height)               strip1.setPixelColor(i,   0,   0, 0);
    //else strip1.setPixelColor(i,Wheel(map(i,0,strip1.numPixels()-1,20,150)));
    else strip1.setPixelColor(i,Wheel(map(i,0,strip1.numPixels()-1,20,150)+150));
    
  }
 strip1.show(); // Update strip
 
  
   
 
// Every few frames, make the peak pixel drop by 1:
 
    if(++dotCount >= PEAK_FALL4) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    } 
     n   = analogRead(LEFT_IN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
 
  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peak)     peak   = height; // Keep 'peak' dot at top
 
 
  // Color pixels based on rainbow gradient
  for(i=0; i<NUM_LEDS; i++) {
    if(i >= height)               strip2.setPixelColor(i,   0,   0, 0);
    else strip2.setPixelColor(i,Wheel(map(i,0,strip2.numPixels()-1,20,200)+150));
    
  }
 strip2.show(); // Update strip
 
  // Draw peak dot  
  if(peak > 0 && peak <= NUM_LEDS-1) strip1.setPixelColor(peak,Wheel(map(peak,0,strip1.numPixels()-1,30,150)));
  if(peak > 0 && peak <= NUM_LEDS-1) strip2.setPixelColor(peak,Wheel(map(peak,0,strip2.numPixels()-1,30,150)));
   strip1.show();
   strip2.show();
 
// Every few frames, make the peak pixel drop by 1:
 
    if(++dotCount >= PEAK_FALL4) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    } 
 
 
  vol[volCount] = n;                      // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
 
  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl)      minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
 
}
 
// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

     void VU2() { // EDIT ME


   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1023;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

   int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip1.numPixels(), COLOR_FROM, COLOR_TO);
    // strip1.setPixelColor(i, 100, 100, 100); // EDIT HERE
     strip1.setPixelColor(i, 100, 100, 100); // EDIT HERE
     
   }
   
   for(int i = strip1.numPixels() ; i > led; i--)
   {
     strip1.setPixelColor(i, 0); 
     
   }
  strip1.show();
 
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
  // if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak1,(map(peak,0,strip1.numPixels()-1,0,255)));
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,(map(peak,0,strip.numPixels()-1,orange,orange)));

   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL3) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = -2;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 1023;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow1)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip2.numPixels(), COLOR_FROM, COLOR_TO);
     strip2.setPixelColor(i, 100, 100, 100); // AND HERE
   }
   
   for(int i = strip2.numPixels() ; i > led; i--)
   {
     strip2.setPixelColor(i, 0); 
     
   }
  strip2.show();

  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,(map(peak1,0,strip2.numPixels()-1,orange,orange)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL3) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = -2;
    }
}
    }

     void VU3() {

   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1023;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

   int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip1.numPixels(), COLOR_FROM, COLOR_TO);
     strip1.setPixelColor(i, 0, 255, 0);
     
   }
   
   for(int i = strip1.numPixels() ; i > led; i--)
   {
     strip1.setPixelColor(i, 0); 
     
   }
  strip1.show();
 
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
   //if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,Wheel(map(peak,0,strip1.numPixels()-1,255,0)));
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,(map(peak,0,strip1.numPixels()-1,red,red)));
   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL3) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = -2;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 1023;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow1)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip2.numPixels(), COLOR_FROM, COLOR_TO);
     strip2.setPixelColor(i, 0, 255, 0);
   }
   
   for(int i = strip2.numPixels() ; i > led; i--)
   {
     strip2.setPixelColor(i, 0); 
     
   }
  strip2.show();

  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,(map(peak1,0,strip2.numPixels()-1,red,red)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL3) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = -2;
    }
}
    }

    void VU4() {

   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1023;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

   int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip1.numPixels(), COLOR_FROM, COLOR_TO);
     strip1.setPixelColor(i, 255, 0, 0);
     
   }
   
   for(int i = strip1.numPixels() ; i > led; i--)
   {
     strip1.setPixelColor(i, 0); 
     
   }
  strip1.show();
 
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,(map(peak,0,strip1.numPixels()-1,blue,blue)));

   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL3) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = -2;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 1023;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow1)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip2.numPixels(), COLOR_FROM, COLOR_TO);
     strip2.setPixelColor(i, 255, 0, 0);
   }
   
   for(int i = strip2.numPixels() ; i > led; i--)
   {
     strip2.setPixelColor(i, 0); 
     
   }
  strip2.show();

  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,(map(peak1,0,strip2.numPixels()-1,blue,blue)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL3) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = -2;
    }
}
    }

     void VU5() {

   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1023;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

   int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip1.numPixels(), COLOR_FROM, COLOR_TO);
     strip1.setPixelColor(i, 0, 0, 255);
     
   }
   
   for(int i = strip1.numPixels() ; i > led; i--)
   {
     strip1.setPixelColor(i, 0); 
     
   }
  strip1.show();
 
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,(map(peak,0,strip1.numPixels()-1,green,green)));

   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL3) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = -2;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 1023;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow1)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;
   
   for(int i=0; i < led; i++)
   {
     int color = map(i, COLOR_START, strip2.numPixels(), COLOR_FROM, COLOR_TO);
     strip2.setPixelColor(i, 0, 0, 255);
   }
   
   for(int i = strip2.numPixels() ; i > led; i--)
   {
     strip2.setPixelColor(i, 0); 
     
   }
  strip2.show();

  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,(map(peak1,0,strip2.numPixels()-1,green,green)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL3) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = -2;
    }
  }
}

//uint32_t Wheel(byte WheelPos) {
//  WheelPos = 255 - WheelPos;
//  if(WheelPos < 45) {
//   return strip1.Color(255 - WheelPos * 3, 0, WheelPos * 3);
//   return strip2.Color(255 - WheelPos * 3, 0, WheelPos * 3);  
//} else if(WheelPos < 170) {
//    WheelPos -= 85;
//   return strip1.Color(0, WheelPos * 3, 255 - WheelPos * 3);
//   return strip2.Color(0, WheelPos * 3, 255 - WheelPos * 3);
//  } else {
//   WheelPos -= 170;
//   return strip1.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
//   return strip2.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
//  }
//}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip1.numPixels(); i++)
  for(uint16_t i=0; i<strip2.numPixels(); i++) {
    strip1.setPixelColor(i, c);
    strip2.setPixelColor(i, c);
    strip1.show();
    strip2.show();
    delay(wait);
  }
}

void colorWipe2(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip1.setPixelColor(i, c);
      strip2.setPixelColor(i, c);
      strip1.show();
      strip2.show();
      delay(wait);
  }
}


    // Pattern 2 - V U METER 2
    void VU7() {
  
  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;
 
 
 
  n   = analogRead(LEFT_IN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
 
  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peak)     peak   = height; // Keep 'peak' dot at top
 
 
  // Color pixels based on rainbow gradient
  for(i=0; i<NUM_LEDS_HALF; i++) {
    if(i >= height) {              
      strip1.setPixelColor(NUM_LEDS_HALF-i-1,   0,   0, 0);  //Hintergrund LEDs
      strip1.setPixelColor(NUM_LEDS_HALF+i,   0,   0, 0);    //Hintergrund LEDs
    }
    else {
      //uint32_t color = Wheel(map(i,0,NUM_LEDS_HALF-1,30,150));
      //uint32_t color = Wheel(map(i,0,NUM_LEDS_HALF-1,180,50));
      uint32_t color = Wheel(255);
      strip1.setPixelColor(NUM_LEDS_HALF-i-1,color);
      strip1.setPixelColor(NUM_LEDS_HALF+i,color);
    }
    
  } 
 
  // Draw peak dot  
  if(peak > 0 && peak <= NUM_LEDS_HALF-1) {
    //uint32_t color = Wheel(map(peak,0,NUM_LEDS_HALF-1,30,150));
    uint32_t color = Wheel(110);
    strip1.setPixelColor(NUM_LEDS_HALF-peak-1,color);
    strip1.setPixelColor(NUM_LEDS_HALF+peak,color);
  }

      strip1.show(); // Update strip




     n   = analogRead(RIGHT_IN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);
 
  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peak)     peak   = height; // Keep 'peak' dot at top
 
 
  // Color pixels based on rainbow gradient
  for(i=0; i<NUM_LEDS_HALF; i++) {
    if(i >= height) {              
      strip2.setPixelColor(NUM_LEDS_HALF-i-1,   0,   0, 0);
      strip2.setPixelColor(NUM_LEDS_HALF+i,   0,   0, 0);
    }
    else {
      //uint32_t color = Wheel(map(i,0,NUM_LEDS_HALF-1,30,150));
      //uint32_t color = Wheel(map(i,0,NUM_LEDS_HALF-1,180,50));
      uint32_t color = Wheel(255);
      strip2.setPixelColor(NUM_LEDS_HALF-i-1,color);
      strip2.setPixelColor(NUM_LEDS_HALF+i,color);
    }
    
  } 
 
  // Draw peak dot  
  if(peak > 0 && peak <= NUM_LEDS_HALF-1) {
    //uint32_t color = Wheel(map(peak,0,NUM_LEDS_HALF-1,30,150));
    uint32_t color = Wheel(110);
    strip2.setPixelColor(NUM_LEDS_HALF-peak-1,color);
    strip2.setPixelColor(NUM_LEDS_HALF+peak,color);
  }
  
   strip2.show(); // Update strip
 
// Every few frames, make the peak pixel drop by 1:
 
    if(++dotCount >= PEAK_FALL4) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    }
 
 
 
  vol[volCount] = n;                      // Save sample for dynamic leveling
  if(++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter
 
  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(vol[i] < minLvl)      minLvl = vol[i];
    else if(vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
 
}

    // Pattern 1 - V U METER
    void VU8() {

   uint8_t i;
   int n, height;
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1023;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;

  // Color pixels based on rainbow gradient
  for (i = 0; i < NUM_LEDS; i++) {
    if (i >= height) {
      strip2.setPixelColor(i, 0, 0, 0);
    } else {
      strip2.setPixelColor(i, Wheel(
        map(i, 0, strip2.numPixels() - 1, (int)greenOffset, (int)blueOffset)
      ));
    }
  }
  strip1.show();
 
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,Wheel(map(peak,0,strip1.numPixels()-1,0,255)));

   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL2) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 2;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 1023;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow1)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;

  // Color pixels based on rainbow gradient
  for (i = 0; i < NUM_LEDS; i++) {
    if (i >= height) {
      strip1.setPixelColor(i, 0, 0, 0);
    } else {
      strip1.setPixelColor(i, Wheel(
        map(i, 0, strip1.numPixels() - 1, (int)greenOffset, (int)blueOffset)
      ));
    }
  }
    strip2.show();

  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,Wheel(map(peak1,0,strip2.numPixels()-1,60,255)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL2) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = 2;
    }
}
    }


    void VU9(){

   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 100;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

   int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;
   
   for(int i; i <= led; i++)
   {
     int color = map(i, COLOR_START, strip1.numPixels(), COLOR_FROM, COLOR_TO);
     
     if(i<N_STEPS*0.5){
         strip1.setPixelColor(i, 0, 255, 0);
         strip1.setPixelColor(NUM_LEDS-i, 0, 255, 0);
     }
     else if(i>=N_STEPS*0.5 && i<N_STEPS*0.7){
         strip1.setPixelColor(i, 255, 255, 0);
         strip1.setPixelColor(NUM_LEDS-i, 255, 255, 0);
     }
     else{
         strip1.setPixelColor(i, 255, 0, 0);
         strip1.setPixelColor(NUM_LEDS-i, 255, 0, 0);
     }
   }
   
   for(int i = strip1.numPixels() ; i > led; i--)
   {
     strip1.setPixelColor(i, 0); 
     
   }
  strip1.show();
 
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,(map(peak,0,strip1.numPixels()-1,red,red)));

   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL3) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 100;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow1)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;
   
   for(int i; i <= led; i++)
   {
     int color = map(i, COLOR_START, strip2.numPixels(), COLOR_FROM, COLOR_TO);
     if(i<N_STEPS*0.5){
         strip2.setPixelColor(i, 0, 255, 0);
         strip2.setPixelColor(NUM_LEDS-i, 0, 255, 0);
     }
     else if(i>=N_STEPS*0.5 && i<N_STEPS*0.7){
         strip2.setPixelColor(i, 255, 255, 0);
         strip2.setPixelColor(NUM_LEDS-i, 255, 255, 0);
     }
     else{
         strip2.setPixelColor(i, 255, 0, 0);
         strip2.setPixelColor(NUM_LEDS-i, 255, 0, 0);
     }
   }
   
   
   for(int i = strip2.numPixels() ; i > led; i--)
   {
     strip2.setPixelColor(i, 0); 
     
   }
  strip2.show();

  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,(map(peak1,0,strip2.numPixels()-1,red,red)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL3) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = 0;
    }
}
    }
void VU10(){
  
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 100;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(RIGHT_IN);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

   int led = map(peakToPeak, 0, maximum, 0, strip1.numPixels()) -1;
   
   for(int i; i <= led; i++)
   {
     int color = map(i, COLOR_START2, strip1.numPixels(), COLOR_FROM, COLOR_TO);
     //drawCycleColor(cycleSpeed);
     uint32_t colorValLeft = WheelLeft((millis() >> 8) & 255);
    strip1.setPixelColor(i, colorValLeft);
    strip1.setPixelColor(NUM_LEDS-i, colorValLeft);
     
   }
   
   for(int i = strip1.numPixels() ; i > led; i--)
   {
     strip1.setPixelColor(i, 0); 
     
   }
  strip1.show();
//EDITED//  
  if(led > peak)  peak = led; // Keep 'peak' dot at top
   
   if(peak > 1 && peak <= strip1.numPixels()-1) strip1.setPixelColor(peak,(map(peak,0,strip1.numPixels()-1,white,white)));

   strip1.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount >= PEAK_FALL) { //fall rate 
      
      if(peak > 0) peak--;
      dotCount = 0;
    }
    // EDITED//
    
{
   unsigned long startMillis1= millis();  // Start of sample window
   unsigned int peakToPeak1 = 0;   // peak-to-peak level

   unsigned int signalMax1 = 0;
   unsigned int signalMin1 = 100;

   // collect data for 50 mS
   while (millis() - startMillis1 < sampleWindow)
   {
      sample1 = analogRead(LEFT_IN);
      if (sample1 < 1024)  // toss out spurious readings
      {
         if (sample1 > signalMax1)
         {
            signalMax1 = sample1;  // save just the max levels
         }
         else if (sample1 < signalMin1)
         {
            signalMin1 = sample1;  // save just the min levels
         }
      }
   }
   peakToPeak1 = signalMax1 - signalMin1;  // max - min = peak-peak amplitude
   
   int led = map(peakToPeak1, 0, maximum1, 0, strip2.numPixels()) -1;
   
   for(int i; i <= led; i++)
   {
     int color = map(i, COLOR_START2, strip2.numPixels(), COLOR_FROM, COLOR_TO);
     //drawCycleColor(cycleSpeed);
     uint32_t colorValRight = WheelLeft((millis() >> 8) & 255);
      strip2.setPixelColor(i, colorValRight);
      strip2.setPixelColor(NUM_LEDS-i, colorValRight);
   }
   
   for(int i = strip2.numPixels() ; i > led; i--)
   {
     strip2.setPixelColor(i, 0); 
     
   }
  strip2.show();

  //EDITED//
  
  if(led > peak1)  peak1 = led; // Keep 'peak' dot at top
   
   if(peak1 > 1 && peak1 <= strip2.numPixels()-1) strip2.setPixelColor(peak1,(map(peak1,0,strip2.numPixels()-1,white,white)));

   strip2.show();
   
// Every few frames, make the peak pixel drop by 1:

    if(++dotCount1 >= PEAK_FALL1) { //fall rate 
      
     if(peak1 > 0) peak1--;
      dotCount1 = 0;
    }

    //EDITED//
    
}
    }






 
void Balls() {
  for (int i = 0 ; i < NUM_BALLS ; i++) {
    tCycle[i] =  millis() - tLast[i] ;     // Calculate the time since the last time the ball was on the ground

    // A little kinematics equation calculates positon as a function of time, acceleration (gravity) and intial velocity
    h[i] = 0.5 * GRAVITY * pow( tCycle[i]/1000 , 2.0 ) + vImpact[i] * tCycle[i]/1000;

    if ( h[i] < 0 ) {                      
      h[i] = 0;                            // If the ball crossed the threshold of the "ground," put it back on the ground
      vImpact[i] = COR[i] * vImpact[i] ;   // and recalculate its new upward velocity as it's old velocity * COR
      tLast[i] = millis();

      if ( vImpact[i] < 0.01 ) vImpact[i] = vImpact0;  // If the ball is barely moving, "pop" it back up at vImpact0
    }
    pos[i] = round( h[i] * (NUM_LEDS - 1) / h0);       // Map "h" to a "pos" integer index position on the LED strip
  }

  //Choose color of LEDs, then the "pos" LED on
  for (int i = 0 ; i < NUM_BALLS ; i++) leds[pos[i]] = CHSV( uint8_t (i * 40) , 255, 255);
  FastLED.show();
  //Then off for the next loop around
  for (int i = 0 ; i < NUM_BALLS ; i++) {
    leds[pos[i]] = CRGB::Black;
  }
}


 void ripple() {
 
    if (currentBg == nextBg) {
      nextBg = random(256);
    }
    else if (nextBg > currentBg) {
      currentBg++;
    } else {
      currentBg--;
    }
    for(uint16_t l = 0; l < NUM_LEDS; l++) {
      leds[l] = CHSV(currentBg, 255, 50);         // strip.setPixelColor(l, Wheel(currentBg, 0.1));
    }
 
  if (step == -1) {
    center = random(NUM_LEDS);
    color = random(256);
    step = 0;
  }
 
  if (step == 0) {
    leds[center] = CHSV(color, 255, 255);         // strip.setPixelColor(center, Wheel(color, 1));
    step ++;
  }
  else {
    if (step < maxSteps) {
      Serial.println(pow(fadeRate,step));
 
      leds[wrap(center + step)] = CHSV(color, 255, pow(fadeRate, step)*255);       //   strip.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
      leds[wrap(center - step)] = CHSV(color, 255, pow(fadeRate, step)*255);       //   strip.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
      if (step > 3) {
        leds[wrap(center + step - 3)] = CHSV(color, 255, pow(fadeRate, step - 2)*255);     //   strip.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
        leds[wrap(center - step + 3)] = CHSV(color, 255, pow(fadeRate, step - 2)*255);     //   strip.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
      }
      step ++;
    }
    else {
      step = -1;
    }
  }
 
  LEDS.show();
  delay(50);
}
 
 
int wrap(int step) {
  if(step < 0) return NUM_LEDS + step;
  if(step > NUM_LEDS - 1) return step - NUM_LEDS;
  return step;
}
 
 
void one_color_allHSV(int ahue, int abright) {                // SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0 ; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(ahue, 255, abright);
  }
}

 
void ripple2() {
   if (BG){
    if (currentBg == nextBg) {
      nextBg = random(256);
    } 
    else if (nextBg > currentBg) {
      currentBg++;
    } else {
      currentBg--;
    }
    for(uint16_t l = 0; l < NUM_LEDS; l++) {
      strip1.setPixelColor(l, Wheel(currentBg, 0.1));
      strip2.setPixelColor(l, Wheel(currentBg, 0.1));
    }
  } else {
    for(uint16_t l = 0; l < NUM_LEDS; l++) {
      strip1.setPixelColor(l, 0, 0, 0);
      strip2.setPixelColor(l, 0, 0, 0);
    }
  }
 
 
  if (step == -1) {
    center = random(NUM_LEDS);
    color = random(256);
    step = 0;
  }
 
 
 
  if (step == 0) {
    strip1.setPixelColor(center, Wheel(color, 1));
    strip2.setPixelColor(center, Wheel(color, 1));
    step ++;
  } 
  else {
    if (step < maxSteps) {
      strip1.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
      strip1.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
      strip2.setPixelColor(wrap(center + step), Wheel(color, pow(fadeRate, step)));
      strip2.setPixelColor(wrap(center - step), Wheel(color, pow(fadeRate, step)));
      if (step > 3) {
        strip1.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
        strip1.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
        strip2.setPixelColor(wrap(center + step - 3), Wheel(color, pow(fadeRate, step - 2)));
        strip2.setPixelColor(wrap(center - step + 3), Wheel(color, pow(fadeRate, step - 2)));
      }
      step ++;
    } 
    else {
      step = -1;
    }
  }
  
  strip1.show();
  strip2.show();
  delay(50);
}


 
 
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos, float opacity) {
  
  if(WheelPos < 85) {
    return strip.Color((WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color((255 - WheelPos * 3) * opacity, 0, (WheelPos * 3) * opacity);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, (WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity);
  }
}


void sinelon() {
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, thisfade);
  int pos1 = beatsin16(thisbeat,0,NUM_LEDS);
  int pos2 = beatsin16(thatbeat,0,NUM_LEDS);
    leds[(pos1+pos2)/2] += CHSV( myhue++/64, thissat, thisbri);
}
FastLED.show();
}



void juggle() {                                               // Several colored dots, weaving in and out of sync with each other

  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
FastLED.show();

  
}



void fireblu(){
#define FRAMES_PER_SECOND 40
random16_add_entropy( random());

  
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White), colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;

    }
    FastLED.show();
  }

  void fire(){
#define FRAMES_PER_SECOND 40
random16_add_entropy( random());

  
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow,  CRGB::White), colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;

    }
    FastLED.show();
  }
  
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
  }
}


void Drip()
{
 MODE_WATER_TORTURE: 
 if (cycle())
        {
        strip1.setBrightness(255); // off limits
        strip2.setBrightness(255); // off limits
        water_torture1.animate(reverse);
        water_torture2.animate(reverse);
        strip1.show();
        strip2.show();
       // strip1.setBrightness(brightness); // back to limited
        //strip2.setBrightness(brightness); // back to limited
        }
  }

      
bool cycle()
{
  if (paused)
  {
    return false;
  }
  
  if (millis() - lastTime >= cycleMillis)
  {
    lastTime = millis();
    return true;
  }
  return false;
}


 
 
float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve){
 
  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;
 
 
  // condition curve parameter
  // limit range
 
  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;
 
  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function
 
  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }
 
  // Zero Refference the values
  OriginalRange = originalMax - originalMin;
 
  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }
 
  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float
 
  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }
 
  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;
 
  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }
 
  return rangedValue;
}

void drawRainbow(){
  for (int i=0;i<=N_STEPS;i++){
    strip1.setPixelColor(i,WheelLeft(map(i, 0, N_STEPS, 30, 150)));
    strip1.setPixelColor(NUM_LEDS-i,WheelLeft(map(i, 0, N_STEPS, 30, 150)));
    strip2.setPixelColor(i,WheelRight(map(i, 0, N_STEPS, 30, 150)));
    strip2.setPixelColor(NUM_LEDS-i,WheelRight(map(i, 0, N_STEPS, 30, 150)));
  }
}
void drawCycleColor(uint8_t _speed){
  for (int i=0;i<=N_STEPS;i++){
    uint32_t colorValLeft = WheelLeft((millis() >> _speed) & 255);
    uint32_t colorValRight = WheelLeft((millis() >> _speed) & 255);
    strip1.setPixelColor(i, colorValLeft);
    strip1.setPixelColor(NUM_LEDS-i, colorValLeft);
    strip2.setPixelColor(i, colorValRight);
    strip2.setPixelColor(NUM_LEDS-i, colorValRight);
  }
}

 
  




void Twinkle () {
   if (random(25) == 1) {
      uint16_t i = random(NUM_LEDS);
      if (redStates[i] < 1 && greenStates[i] < 1 && blueStates[i] < 1) {
        redStates[i] = random(256);
        greenStates[i] = random(256);
        blueStates[i] = random(256);
      }
    }
    
    for(uint16_t l = 0; l < NUM_LEDS; l++) {
      if (redStates[l] > 1 || greenStates[l] > 1 || blueStates[l] > 1) {
        strip1.setPixelColor(l, redStates[l], greenStates[l], blueStates[l]);
        strip2.setPixelColor(l, blueStates[l],greenStates[l], redStates[l]);
        if (redStates[l] > 1) {
          redStates[l] = redStates[l] * Fade;
        } else {
          redStates[l] = 0;
        }
        
        if (greenStates[l] > 1) {
          greenStates[l] = greenStates[l] * Fade;
        } else {
          greenStates[l] = 0;
        }
        
        if (blueStates[l] > 1) {
          blueStates[l] = blueStates[l] * Fade;
        } else {
          blueStates[l] = 0;
        }
        
      } else {
        strip1.setPixelColor(l, 0, 0, 0);
        strip2.setPixelColor(l, 0, 0, 0);
      }
    }
    strip1.show();
    strip2.show();
     delay(10);
  
}   

// Pattern 3 - JUGGLE
    void pattern3() {
       ChangeMe();
  juggle2();
  show_at_max_brightness_for_power();                         // Power managed display of LED's.
} // loop()


void juggle2() {                                               // Several colored dots, weaving in and out of sync with each other
  curhue = thishue;                                          // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for( int i = 0; i < numdots; i++) {
    leds[beatsin16(basebeat+i+numdots,0,NUM_LEDS)] += CHSV(curhue, thissat, thisbright);   //beat16 is a FastLED 3.1 function
    curhue += hueinc;
  }
} // juggle()


void ChangeMe() {                                             // A time (rather than loop) based demo sequencer. This gives us full control over the length of each sequence.
  uint8_t secondHand = (millis() / 1000) % 30;                // IMPORTANT!!! Change '30' to a different value to change duration of the loop.
  static uint8_t lastSecond = 99;                             // Static variable, means it's only defined once. This is our 'debounce' variable.
  if (lastSecond != secondHand) {                             // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    if (secondHand ==  0)  {numdots=1; faderate=2;}  // You can change values here, one at a time , or altogether.
    if (secondHand == 10)  {numdots=4; thishue=128; faderate=8;}
    if (secondHand == 20)  {hueinc=48; thishue=random8();}                               // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
  }
} // ChangeMe()

void blur() {

  uint8_t blurAmount = dim8_raw( beatsin8(3,64, 192) );       // A sinewave at 3 Hz with values ranging from 64 to 192.
  blur1d( leds, NUM_LEDS, blurAmount);                        // Apply some blurring to whatever's already on the strip, which will eventually go black.
  
  uint8_t  i = beatsin8(  9, 0, NUM_LEDS);
  uint8_t  j = beatsin8( 7, 0, NUM_LEDS);
  uint8_t  k = beatsin8(  5, 0, NUM_LEDS);
  
  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis();  
  leds[(i+j)/2] = CHSV( ms / 29, 200, 255);
  leds[(j+k)/2] = CHSV( ms / 41, 200, 255);
  leds[(k+i)/2] = CHSV( ms / 73, 200, 255);
  leds[(k+i+j)/3] = CHSV( ms / 53, 200, 255);
  
  FastLED.show();
  }

  void matrixone () {
  ChangeMeone();
  matrix();
  show_at_max_brightness_for_power();
  delay_at_max_brightness_for_power(thisdelay*2.5);
  Serial.println(LEDS.getFPS());
} // loop()



void matrix() {                                               // One line matrix

  if (huerots) thishues=thishues+5;

  if (random16(90) > 80) {
    if (thisdirs == 0) leds[0] = CHSV(thishues, thissats, 255); else leds[NUM_LEDS-1] = CHSV(thishues, thissats, 255);
  }
  else {leds[0] = CHSV(thishues, thissats, 0);}

  if (thisdirs == 0) {
    for (int i = NUM_LEDS-1; i >0 ; i-- ) leds[i] = leds[i-1];
  } else {
    for (int i = 0; i < NUM_LEDS ; i++ ) leds[i] = leds[i+1];
    FastLED.show();
  }
} // matrix()



void ChangeMeone() {                                             // A time (rather than loop) based demo sequencer. This gives us full control over the length of each sequence.
  uint8_t secondHand = (millis() / 1000) % 60;                // Change '25' to a different value to change length of the loop.
  static uint8_t lastSecond = 99;                             // Static variable, means it's only defined once. This is our 'debounce' variable.
  if (lastSecond != secondHand) {                             // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    if (secondHand ==  0)  {thisdelays=30; thishues=95; huerots=0;}
    if (secondHand ==  5)  {thisdirs=1; huerots=1;}
    if (secondHand == 10)  { }
    if (secondHand == 15)  { }
    if (secondHand == 20)  { }
    if (secondHand == 25)  { }
    if (secondHand == 30)  { }
    if (secondHand == 35)  { }
    if (secondHand == 40)  { }
    if (secondHand == 45)  { }
    if (secondHand == 50)  { }
    if (secondHand == 55)  { }
  }
} // ChangeMe()




// the loop routine runs over and over again forever;
void loop() {

  //read the DMX values
  readDMX();

/*   
  unsigned long lastPacket = DMXSerial.noDataSince();
  if (lastPacket < 2000) {
    strip.setBrightness(bright);
    FastLED.setBrightness(bright);
    }else {
      //strip.setBrightness(255);
      //dmxTopColor        = 160;
      //dmxBottomColor     = 255;
      //dmxPeakTopColor    = 73;
      //dmxPeakBottomColor = 73;
      //dmxProgram         = 1;
   }
 */
   //Serial.print("DMX Programm: ");
    //Serial.println(dmxProgram);
    //delay(1000);  

 unsigned long lastPacket = DMXSerial.noDataSince();
  if (lastPacket < 3000) {
    dmxProgram = DMXSerial.read(DMXSTART);
    //dmxBright  = DMXSerial.read(DMXSTART+1);
    }else {
    //dmxProgram = 5;
    buttonMode();
   }
  
    switch(dmxProgram) {
    //switch(map(dmxProgram, 0, 1023, 0, 100)){

     case  1: All2();     break;  // VU-Meters automatc
     case  2: All();      break;  // Patterns automatic
     case  3: VU11();     break;  // normal
     case  4: VU12();     break;  // normal rainbow 
     case  5: VU13();     break;  // rainbow centre
     case  6: VU14();     break;  // low rainbow 2
     case  7: VU7();      break;  // low rainbow 2
     case  8: VU4();      break;  // red
     case  9: VU5();      break;  // blue
     case 10: VU2();      break;  // low rainbow
     case 11: VU3();      break;  // white
     case 12: VU8();      break;  // green 
     case 13: VU9();      break;  // classic 
     case 14: VU10();     break;  // slow rainbow change 
     case 15: ripple();   break;  // 
     case 16: ripple2();  break;
     case 17: sinelon();  break;
     case 18: juggle();   break;
     case 19: pattern3(); break;
     case 20: Twinkle();  break;
     case 21: Balls();    break; 
     case 22: matrixone();break; 
     case 23: blur();     break;
     case 24: fire();     break;
     case 25: fireblu();  break;
     case 26: Drip();     break;
     case 27: allOff();   break;         
     }
 }    

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t WheelLeft(byte WheelPos) {
  if(WheelPos < 85) {
    return strip1.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip1.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip1.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t WheelRight(byte WheelPos) {
  if(WheelPos < 85) {
    return strip2.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip2.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip2.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {ripple, ripple2, sinelon, juggle, pattern3, Twinkle, blur, matrixone, fire, fireblu, Drip};
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}
void All()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();
  EVERY_N_SECONDS( 20 ) { nextPattern(); } // change patterns periodically
}
// second list

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList qPatterns = {VU11, VU12, VU13, VU14, VU7, VU4, VU5, VU2, VU3, VU8, VU9, VU10};
uint8_t qCurrentPatternNumber = 0; // Index number of which pattern is current

void nextPattern2()
{
  // add one to the current pattern number, and wrap around at the end
  qCurrentPatternNumber = (qCurrentPatternNumber + 1) % ARRAY_SIZE( qPatterns);
}
void All2()
{
  // Call the current pattern function once, updating the 'leds' array
  qPatterns[qCurrentPatternNumber]();
  EVERY_N_SECONDS( 20 ) { nextPattern2(); } // change patterns periodically
}

void readDMX(){
  dmxProgram         = DMXSerial.read(DMXSTART);
  //dmxBright          = DMXSerial.read(DMXSTART+1);
  /*dmxTopColor        = DMXSerial.read(DMXSTART+1);
  dmxBottomColor     = DMXSerial.read(DMXSTART+2);
  dmxPeakTopColor    = DMXSerial.read(DMXSTART+3);
  dmxPeakBottomColor = DMXSerial.read(DMXSTART+4);
  dmxProgram         = DMXSerial.read(DMXSTART+5);*/
}

void allOff(){
  for (int i = 0; i < NUM_LEDS; i++) {
      strip1.setPixelColor(i, 75, 0, 130);
      strip2.setPixelColor(i, 0, 255, 0);
      strip1.show(); // Update strip
      strip2.show(); // Update strip
/*
uint32_t red = strip1.Color(255, 0, 0);
uint32_t orange = strip1.Color(255, 127, 0);
uint32_t yellow = strip1.Color(255, 255, 0);
uint32_t green = strip1.Color(0, 255, 0);
uint32_t blue = strip1.Color(0, 0, 255);
uint32_t purple = strip1.Color(75, 0, 130);
uint32_t white = strip1.Color(125, 125, 125);*/
  }
}

void buttonMode(){
  
  // read that state of the pushbutton value;
  int buttonVal = digitalRead(buttonPin);
  if (buttonVal == LOW && oldButtonVal == HIGH) {// button has just been pressed
    lightPattern = lightPattern + 1;
  }
  if (lightPattern > nPatterns) lightPattern = 1;
  oldButtonVal = buttonVal;
  
  switch(lightPattern) {
     case  1: All2();     break;  // VU-Meters automatc
     case  2: All();      break;  // Patterns automatic
     case  3: VU11();     break;  // normal
     case  4: VU12();     break;  // normal rainbow 
     case  5: VU13();     break;  // rainbow centre
     case  6: VU14();     break;  // low rainbow 2
     case  7: VU7();      break;  // low rainbow 2
     case  8: VU4();      break;  // red
     case  9: VU5();      break;  // blue
     case 10: VU2();      break;  // low rainbow
     case 11: VU3();      break;  // white
     case 12: VU8();      break;  // green 
     case 13: VU9();      break;  // classic 
     case 14: VU10();     break;  // slow rainbow change 
     case 15: ripple();   break;  // 
     case 16: ripple2();  break;
     case 17: sinelon();  break;
     case 18: juggle();   break;
     case 19: pattern3(); break;
     case 20: Twinkle();  break;
     case 21: Balls();    break; 
     case 22: matrixone();break; 
     case 23: blur();     break;
     case 24: fire();     break;
     case 25: fireblu();  break;
     case 26: Drip();     break;
     case 27: allOff();   break;
    }            
  }
