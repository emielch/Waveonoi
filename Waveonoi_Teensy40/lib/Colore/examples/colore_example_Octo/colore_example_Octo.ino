#include <Colore.h>
#include <OctoWS2811.h>

#define BEAM_AM 10
Beam beams[BEAM_AM];

const int ledsPerStrip = 144;
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

Segment seg[] = {
   Segment(0,144),  // 0
};

const int LED_AM = ledsPerStrip * 8;
byte segAm = sizeof(seg)/sizeof(Segment);
Colore colore( LED_AM, seg, segAm, beams, BEAM_AM, &set_ledLib, &get_ledLib, &show_ledLib, &reset_ledLib );


void setup(){
  Serial.begin(9600);
  leds.begin();
  
  // let the LEDs do something...
  seg[0].setRainbow(0.5,1,40);
}

void loop(){
  colore.update(true);
  printFramerate();
}


void printFramerate(){
  Serial.print("FrameRate: ");
  Serial.println(colore.getFPS()); // print framerate
}


/* ---- Library Interface Functions ---- */

void set_ledLib(int pixel, byte r, byte g, byte b){
  leds.setPixel(pixel, r, g, b);
}

void show_ledLib(){
  leds.show();
}

uint32_t bufsize = ledsPerStrip*24;
void reset_ledLib(){
  memset(drawingMemory, 0, bufsize);
}

Color get_ledLib(int pixel){
  uint32_t conn = leds.getPixel(pixel);  // retrieve the color that has already been saved
  byte b = conn & 255;       // unpack the color
  byte g = conn >> 8 & 255;
  byte r = conn >> 16 & 255;
  Color pixelCol(r,g,b,RGB_MODE);
  return pixelCol;
}
