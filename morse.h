#include <Adafruit_NeoPixel.h>

// Number of cycles to fade up and down.
#define TRANSISTION_STEPS 5
// Number of cycles per beat.  Dot = 1 beat, Dash = 3.
#define BEAT_STEPS 3

// Some example morse code arrays.
// Use 1=dot, 3 = dash, NULL to end the character.
char morseP[] = {1,3,3,1, NULL};
char morseU[] = {1,1,3, NULL};
char morseC[] = {3,1,3,1, NULL};
char morseK[] = {1,3,1, NULL};
char morseF[] = {1,1,3,1, NULL};
char morseT[] = {3, NULL};
char morseI[] = {1,1, NULL};
char morseN[] = {3,1, NULL};
char morseA[] = {1,3, NULL};
char morseM[] = {3,3, NULL};
char morseR[] = {3,1,3, NULL};
char morseS[] = {1,1,1, NULL};

// This is the string we'll be transmitting.
char* text[] = {morseF, morseU, morseC, morseK, morseP, morseU, morseT, morseI, morseN};
int text_length = sizeof(text)/sizeof(text[0]);

class MorseLight {
  protected:
    uint32_t color;    
    int letter_index;
    int letter_step;
    int starting_countdown;
    int step_countdown;
    int ending_countdown;
    int break_countdown;
   
  public:
    int address;
    MorseLight(int light_address, int letter_offset, uint32_t my_color) {
      address = light_address;
      color = my_color;
      setLetterIndex(letter_offset);
    }

    void setAddress(unsigned new_address) {
      address = new_address;
    }

    // Advance to this index in the text string.
    void setLetterIndex(int index) {
      setLetterIndexAndStep(index, 0);
    }

    // Advance to this index in the text string.
    void setLetterIndexAndStep(int new_index, int new_step) {
      letter_index = new_index;
      letter_step = new_step;
      starting_countdown = TRANSISTION_STEPS;
      step_countdown = text[letter_index][letter_step] * BEAT_STEPS;
      ending_countdown = TRANSISTION_STEPS;
      break_countdown = BEAT_STEPS * 1; // 1 beats between marks in a letter.
      if (NULL == text[letter_index][letter_step +1]) {
        break_countdown += BEAT_STEPS * 3; // 3 beats at the end of a letter.
      }
    }

    /**
     * Returns color scaled to a brightness of max 255, for fading transitions.
     * 
     * @param uint32_t color
     *   32bit color to scale.
     * @param uint8_t brightness
     *   How much to scale it : 0 = black, 255 = full color.
     *   
     * @return uint32_t
     *   The scaled color.
     */
    uint32_t scaleColor(uint32_t color, uint8_t brightness) {
      uint8_t red = (uint8_t)(color >> 16);
      uint8_t green = (uint8_t)(color >>  8);
      uint8_t blue = (uint8_t)color;

      red = (red * brightness) >> 8;
      green = (green * brightness) >> 8;
      blue = (blue * brightness) >> 8;
            
      return ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
    }

    /**
     * The color of this light at this time.
     * 
     * What this light should look light based on its state and counters.
     * 
     * @return uint32_t
     *   The color this light should be in this cycle.
     */
    uint32_t lightColor() {
      if (starting_countdown > 0) {
        starting_countdown--;
        return scaleColor(color, (255 * (TRANSISTION_STEPS - starting_countdown)) / TRANSISTION_STEPS);
      }
      if (step_countdown > 0) {
        step_countdown--;
        return color;
      }
      if (ending_countdown > 0) {
        ending_countdown--;
        return scaleColor(color, (255 * ending_countdown) / TRANSISTION_STEPS);
      }
      if (break_countdown > 0) {
        break_countdown--;
        return (uint32_t) 0;
      }
      return (uint32_t) 0;
    }

    // Conditions where we are still sending the same letter.
    bool stillSendingLetter() {
      return (starting_countdown > 0) || (step_countdown > 0) || (ending_countdown > 0) || (break_countdown > 0);
    }

   /**
    * Cycle this light one step forward in transmitting the message.
    */
    void cycle(Adafruit_NeoPixel *strip) {
      strip->setPixelColor(address, lightColor());      

      // If this step is still going, nothing to do.
      if (stillSendingLetter()) {
        return;
      }

      // Move on to the beep in this letter.
      letter_step++;
      // If we're not at the end of the letter steps, set the countdowns and continue.
      if (NULL != text[letter_index][letter_step]) {
        setLetterIndexAndStep(letter_index, letter_step);
        return;
      }
      
      // End of letter processing, switch to black.
      strip->setPixelColor(address, 0,0,0);        
      // Move to next letter.
      letter_index++;
      // If we're not out of letters, keep going.
      if (letter_index < text_length) {
        setLetterIndex(letter_index);
        return;
      }
      // If we're not out of letters, that's a sentence!
    }

    /**
     * We have reached the end of the sentence.
     * 
     * @return bool
     *   If we have transmitted the entire sentence yet.
     */
    bool finishedSentence() { return (break_countdown <= 0) && letter_index >= text_length; }
};


/**
 * Displays a repeating series of colors that send morse code sentences starting from a different point.
 */
class MorseLights {
  protected:
    int num_colors = 1;
    uint32_t *colors = NULL;
    int string_length = 0;
    MorseLight **lights_list;
  
  public:

    MorseLights() {
      num_colors = 0;
    }    

    /**
     * Store the list of colors to use for the light string.
     */
    void setPalette(uint32_t *color_list, int color_count) {
      colors = color_list;
      num_colors = color_count;
    }

    /** 
     * Total number of dots and dashes in our text.
     * Needed at one point, left here in case.
     * 
     * @return int
     *   Number of beeps (dots + dashes) in our sentence.
     */
    int totalBeeps() {
      int total_beeps = 0;
      char* letter;
      for (int i = 0; i < text_length; i++) {
        letter = text[i];
        int current_step = 0;        
        do {
          current_step++;
          total_beeps++;
        } while (NULL != letter[current_step]);
      }
      return total_beeps;
    }

    /**
     * Set up the array of MorseLight objects.
     */
    void setup(Adafruit_NeoPixel* strip) {
      string_length = strip->numPixels();
      lights_list = new MorseLight*[string_length];

      // Each offset is LETTER_INDEX * 10 + step.
      // I don't care enough to write this to alloc text_length * 4 of 2 int arrays.
      int beep_offsets[40];
      char* letter;
      int offset_count = 0;
      for (int letter_index = 0; letter_index < text_length; letter_index++) {
        letter = text[letter_index];
        int current_step = 0;        
        do {
          beep_offsets[offset_count++] = (letter_index * 10) + current_step;
          current_step++;
        } while (NULL != letter[current_step]);
      }

      for (int index = 0; index < string_length; index++) {
        lights_list[index] = new MorseLight(index, 0, colors[index % num_colors]);
        lights_list[index]->setLetterIndexAndStep(
          beep_offsets[index % offset_count] / 10,
          beep_offsets[index % offset_count] % 10       
        );
      }
      strip->show();
    }

    /**
     * Update lights along their progression.
     * 
     * Cycle every light, restart ones that have finished the sentence.
     */
    void loop(Adafruit_NeoPixel* strip) {
      // Pause to set the pace.
      delay(100);
      // Make changes to lights.
      for (int index = 0; index < string_length; index++) {
        lights_list[index]->cycle(strip);
        // Reset any that have finished the sentence to start over.
        if (lights_list[index]->finishedSentence()) {
          lights_list[index]->setLetterIndex(0);
        }
      }
      // Display the new lights state.
      strip->show();  
    }
};
