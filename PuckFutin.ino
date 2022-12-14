/**
 * PuckFutin light animation application.
 * 
 * Runs a set of patterns on a string of WS2811 lights.
 * 
 * Signal pin: Mega = 2, NodeMCU = D4
 */

#include <Adafruit_NeoPixel.h>
#ifdef __AVR_ATtiny85__ // Trinket, Gemma, etc.
#include <avr/power.h>
#endif


// Num pixels, signal pin, per-pixel byte offsets the colors in the data stream.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, 2, NEO_RGB + NEO_KHZ400);

#include "morse.h"
#include "palettes.h" // Must happen after strip is instantiated.

MorseLights* morse_lights = NULL;

/**
 * Pre-loop operations and setup.
 */
void setup() {
  Serial.begin(115200);
  strip.begin();
  
  morse_lights = new MorseLights();
  morse_lights->setPalette(ukraine_palette, 2);
  morse_lights->setup(&strip);
}

/**
 * Working code loop.
 */
void loop() {
  morse_lights->loop(&strip);
}
