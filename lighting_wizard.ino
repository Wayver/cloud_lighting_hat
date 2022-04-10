#include <Adafruit_NeoPixel.h>
#include <IRremote.h>

// define button codes for IR remote signal
#define Button1 0xFFA25D
#define Button2 0xFF629D
#define Button3 0xFFE21D
#define Button4 FF22DD
#define Button6 0xFFC23D
#define Button_hash 0xFFB04F
#define Button_mash 0xFFFFFFFF

// declare array for colors
byte colormap[6][3];

bool OnOff;
float power;

// variables to influence color spectrum of the rbgw pixels, understand bias to mean "learns toward"
// in terms of color output
bool stop_waiting = false;
bool white_bias = false;
bool rainbow_bias = true;
bool red_bias = false;
bool blue_bias = false;
bool green_bias = false;

//setup neopixel strip
int NUM_LEDS = 90;
int LED_PIN = 3;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_RGBW);

//setup ir detector
const int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

//variables that influence the frequency of lighting strike function calls
const byte HIGH_STRIKE_LIKELIHOOD = 5;
const byte LOW_STRIKE_LIKELIHOOD = 10;
int currentDataPoint = 0;
int chance = LOW_STRIKE_LIKELIHOOD;

//lightweight class to map a pixel's spatial position relative to its neighbors
// need this for realistic lightning bolts
class PixelInfo {
  public :
  byte * top;
  byte * bot;
  PixelInfo () {}
};

//initialize position of pixels, this requires physical inspection of the constructed hat
byte Starmap[180] = {0, 30, 0, 31, 0, 31, 0, 32, 0, 33, 0, 33, 0, 34, 0, 35, 0, 36, 0, 36, 0, 37, 0, 38, 0, 38, 0, 39, 0, 40, 0, 40, 0, 41, 0, 42, 0, 42, 0, 43, 0, 44, 0, 44, 0, 45, 0, 46, 0, 47, 0, 47, 0, 48, 0, 49, 0, 49, 0, 50, 0, 50,2, 51, 3, 52, 5, 52, 6, 53, 8, 54, 9, 55, 11, 56, 13, 56, 14, 57, 16, 58, 17, 59, 19, 59,21, 60, 22, 61, 24, 62, 25, 63, 27, 63, 28, 64, 30, 65, 30, 65, 31, 66, 33, 67, 34, 68, 36, 69, 37, 70, 39, 71, 40, 72, 41, 74, 43, 75, 44, 76, 46, 77, 47, 78, 49, 79, 50, 80, 50, 80, 51, 81, 52, 81, 53, 82, 54, 83, 55, 84, 56, 84, 58, 85, 59, 86, 60, 86, 61, 87, 62, 88, 63, 89, 64, 89, 65, 90, 65, 91, 67, 92, 68, 93, 70, 94, 72, 95, 73, 96, 75, 97, 77, 98, 78,99, 80, 100};

PixelInfo Stars[90] = {};

// Simple moving average plot
int NUM_Y_VALUES = 17;

float yValues[] = {0, 7, 10, 9, 7.1, 7.5, 7.4,12,15,10,0,3,3.5,4,1,7,1};
float simple_moving_average_previous = 0;
float random_moving_average_previous = 0;
float (*functionPtrs[2])(); //the array of function pointers
void (*electric_ptrs[5])();
int NUM_FUNCTIONS = 2;

byte luck_range;

void setup() {
  OnOff = true;
  //populate Starmap
  // map stars to neighbor pixels
    byte * prt = Starmap;
    int c = -1;
    for(int i=0; i <90; i++) {
      c += 2;
      Stars[i].top = &(Starmap[c]);
      Stars[i].bot = &(Starmap[c-1]);
    }
  Serial.begin(9600);
  //deal with remote
  irrecv.enableIRIn();
  irrecv.blink13(true);
  // Neopixel setup
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  // initializes the array of function pointers.
  functionPtrs[0] = simple_moving_average;
  functionPtrs[1] = random_moving_average;

  
  //functions that call different lightning effects
  electric_ptrs[0] = bolt;  //random path traveling lightning bolt  
  electric_ptrs[1] = thunderburst;  //bright flash over large sections of cloud
  electric_ptrs[2] = rolling;  // quick flash effect over whole hat
  electric_ptrs[3] = storm;  //multiple bolts
  electric_ptrs[4] = vanilla; //testing function that illuminates a target led constantly

 
  colormap[0][0]=1;
  colormap[0][1]=0;
  colormap[0][2]=0;

  colormap[1][0]=1;
  colormap[1][1]=1;
  colormap[1][2]=0;

  colormap[2][0]=0;
  colormap[2][1]=1;
  colormap[2][2]=0;

  colormap[3][0]=0;
  colormap[3][1]=1;
  colormap[3][2]=1;

  colormap[4][0]=0;
  colormap[4][1]=0;
  colormap[4][2]=1;

  colormap[5][0]=1;
  colormap[5][1]=0;
  colormap[5][2]=1;

  power = 10;
  luck_range = 2;
}

void loop() {

  // keep luck range from infinitely increasing
  if(luck_range > 5) {
    luck_range = 1;
  }

  // prevent power multiplier from going below zero, this way we cycle down from 100 -> 0 % repeatedly
  if(power <= 0) { power = 10; }
  controller();

  //bool flag = controller();
  static bool strike = false;
  if(OnOff) {
    if(random(chance) <= luck_range) {   //activate 
  //  if(random(chance) == 3) {   //activate 
      byte funky = random(0,5);
      //Serial.print("funky function for this loop is: ");
      //Serial.println(funky);
      call_lightning(funky);
      chance = HIGH_STRIKE_LIKELIHOOD;
    } // end flag loop 
    else { // end activate
      chance = LOW_STRIKE_LIKELIHOOD;
    }
  } else {
    delay(2000);
  }
  turnAllPixelsOff();
  delay(1000);
}

void turnAllPixelsOff() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

float colorValue() {
  float brightness = callFunction(random(2));
  //Serial.println(brightness);
  float scaled_brightness = abs(brightness*150);
  if(scaled_brightness > 255) {
    scaled_brightness = 255;  
  }  
  return scaled_brightness*(power/10);
}

void lightningStrike(int pixel) {
  //byte brightness = colorValue();
  static byte ccount = 0;
  if(ccount > 6) {
    ccount = 0;
  }
  static byte color_switch = 0;

  if(white_bias) {
    //Serial.println("WHITE FLASH");
    strip.setPixelColor(pixel, strip.Color(colorValue(), colorValue(), colorValue(), 0));
  } else {
    //Serial.println("old blue white");
    byte brightness = colorValue();
    //strip.setPixelColor(pixel, strip.Color(brightness, brightness, brightness, 0));
    strip.setPixelColor(pixel, strip.Color(colormap[ccount][0]*brightness, colormap[ccount][1]*brightness, colormap[ccount][2]*brightness, 0));
    color_switch += 1;
  }

  if(color_switch > 15) {
    ccount += 1;
    color_switch = 0;
  }
  currentDataPoint++;
  currentDataPoint = currentDataPoint%NUM_Y_VALUES;
}

float callFunction(int index) {
  return (*functionPtrs[index])(); //calls the function at the index of `index` in the array
}

void call_lightning(int index) {
  return (*electric_ptrs[index])(); //calls the function at the index of `index` in the array
}

float simple_moving_average() {
  uint32_t startingValue = currentDataPoint;
  uint32_t endingValue = (currentDataPoint+1)%NUM_Y_VALUES;
  float simple_moving_average_current = simple_moving_average_previous + 
                                  (yValues[startingValue])/NUM_Y_VALUES - 
                                  (yValues[endingValue])/NUM_Y_VALUES;

  simple_moving_average_previous = simple_moving_average_current;
  return simple_moving_average_current;
}


// Same as simple moving average, but with randomly-generated data points.
float random_moving_average() {
  float firstValue = random(1, 10);
  float secondValue = random(1, 10);
  float random_moving_average_current = random_moving_average_previous +
                                  firstValue/NUM_Y_VALUES -
                                  secondValue/NUM_Y_VALUES;
  random_moving_average_previous = random_moving_average_current;
  
  //Serial.print(random_moving_average_current);
  return random_moving_average_current;
}

void thunderburst(){

  // this thunder works by lighting two random lengths
  // of the strand from 10-19 pixels.
       for(byte k = 0;k<random(2,4);k++){

  byte rs1 = random(0,NUM_LEDS/2);
  byte rl1 = random(10,19);
  byte rs2 = random(rs1+rl1,NUM_LEDS);
  byte rl2 = random(10,19);
  
  //repeat this chosen strands a few times, adds a bit of realism
  for(byte r = 0;r<random(0,3);r++){
    
    for(byte i=0;i< rl1; i++){
      lightningStrike(i+rs1);
    }
    
    if(rs2+rl2 < NUM_LEDS){
      for(byte i=0;i< rl2; i++){
        lightningStrike(i+rs1);
      }
    }
    //Serial.println("bursting!");
    strip.show();
    //stay illuminated for a set time
    delay(random(250,500));
    turnAllPixelsOff();
  } //end repeat
       }
}  // end thunderburst


void rolling(){
  //Serial.println("rolling");
  // a simple method where we go through every LED with 1/10 chance
  // of being turned on, up to 10 times, with a random delay wbetween each time
  for(int r=0;r<random(2,10);r++){
    //iterate through every LED
    for(int i=NUM_LEDS;i>0;i--){
      if(random(0,100)>90){
        lightningStrike(i);
        delay_show(100);
      }
      else{
        //dont need reset as we're blacking out other LEDs her 
        strip.setPixelColor(i, 0, 0, 0, 0);
      }
    }
  }
}

void bolt () {
  //Serial.println("lightning bolt! lightning bolt!");
  int lrod;
  for (int chain_len=0; chain_len < 10; chain_len++) {
 
    if (chain_len==0) { 
      lrod = random(NUM_LEDS);
      lightningStrike(lrod);
      delay_show(50);
    } else {
     
       int nselect = random(3);    

       if(nselect==0) { lrod = *(Stars[lrod].top); }
       if(nselect==1) { lrod = *(Stars[lrod].bot); }
       if(nselect==2) { lrod += 1; }
       if(nselect==3) { lrod -= 1; }      
       if(lrod >= 90 || lrod <= 0 ) {
         break;
       } else {
         lightningStrike(lrod);
         delay_show(50);
       }
    }
  }
}


void controller(){
  //Serial.println("we are in the controller");

  if (irrecv.decode(&results)) {  
     if( results.value == Button_mash) {
     // Serial.println(results.value, HEX);
      bool waiting = true;
      while(waiting) {
        irrecv.resume();
       //  Serial.println("waiting for new command");
        delay(5000);
        if( (irrecv.decode(&results)) && (results.value != Button_mash)) {
          waiting = false;
        }
       //Serial.println("we made it through another cmd wait loop");
       //Serial.println(results.value, HEX);
      }
    } // button mash end

   // decrease brightness from colorValue by 10%
   if( results.value == Button_hash) {
    // Serial.println("decreased power");
     power -= 1;
     //Serial.println(results.value, HEX);
     irrecv.resume();
     //return false;
     return;
   } // white bias button mash

    // flip white bias to true
    if( results.value == Button3) {
     // Serial.println("it's white");
      //Serial.println(results.value, HEX);
      white_bias = true;
     // Serial.println(white_bias);
      irrecv.resume();
      //return false;
      return;
     } // white bias button mash

     // flip white bias to zero (turn on colors)
     if( results.value == Button6) {
     //  Serial.println("it's rainbow");
       //Serial.println(results.value, HEX);
        white_bias = false;
        //Serial.println(white_bias);
        irrecv.resume();
        //return true;
        return;
     } // white bias button mash

     // toggle lights on off
     if( results.value == Button1){
       bool flip = OnOff; 
     //  Serial.println(results.value, HEX);
       OnOff = !flip;
    //   Serial.print("toggled OnOff to: ");
     //
     //Serial.println(OnOff);
       irrecv.resume();
       return;
     } // end button one

     // increase catch range to initiate strike
     if( results.value == Button2){
      luck_range += 1; 
     // Serial.println(results.value, HEX);   
     // Serial.print("increased luck to: ");
     // Serial.println(luck_range);
      irrecv.resume();
      return;
    }  // end button 2

    
  } else {
   // Serial.println("no results were available");
  } // end irrecv.decode if

  //Serial.println("value at exit is");
  //Serial.println(results.value, HEX);
  irrecv.resume();
  //delay(1000);
}

void delay_show(byte maxtime){
   strip.show();
  delay(random(5, maxtime));
}

void storm() {
  //Serial.println("storming");
  bolt();
  bolt();
  bolt();
  bolt();
}

void vanilla() {
  //Serial.println("called vanilla");
  for (int i = 0; i < 10; i++) {
      // Use this line to keep the lightning focused in one LED.
      // lightningStrike(led):
      // Use this line if you want the lightning to spread out among multiple LEDs.
      lightningStrike(random(NUM_LEDS));
      delay_show(100);
    }
}
