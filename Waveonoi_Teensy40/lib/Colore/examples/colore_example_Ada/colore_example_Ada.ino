#include <Colore.h>
#include <Adafruit_NeoPixel.h>

#define PIN 6
#define LED_AM 60
#define BEAM_AM 10

Beam beams[BEAM_AM];

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_AM, PIN, NEO_GRB + NEO_KHZ800);

Segment seg[] = {
   Segment(0,10),  // 0
   Segment(11,60)  // 1
};

byte segAm = sizeof(seg)/sizeof(Segment);
Colore colore( LED_AM, seg, segAm, beams, BEAM_AM, &set_ledLib, &get_ledLib, &show_ledLib, &reset_ledLib );


void setup(){
  Serial.begin(9600);
  leds.begin();
  
  // let the LEDs do something...
  seg[0].setRainbow(20,10,100);
  seg[1].setRainbow(1,1,20);
}

void loop(){
  colore.update();
  printFramerate();
}


void printFramerate(){
  Serial.print("FrameRate: ");
  Serial.println(colore.getFPS()); // print framerate
}


/* ---- Library Interface Functions ---- */

void set_ledLib(int pixel, byte r, byte g, byte b){
  leds.setPixelColor(pixel, r, g, b);
}

void show_ledLib(){
  leds.show();
}

void reset_ledLib(){
  for(int i=0; i<LED_AM; i++){
    leds.setPixelColor(i,0,0,0);
  }
}

Color get_ledLib(int pixel){
  uint32_t conn = leds.getPixelColor(pixel);  // retrieve the color that has already been saved
  byte b = conn & 255;       // unpack the color
  byte g = conn >> 8 & 255;
  byte r = conn >> 16 & 255;
  Color pixelCol(r,g,b,RGB_MODE);
  return pixelCol;
}
