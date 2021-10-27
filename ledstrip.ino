#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>
#include <IRremote.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct pulse {
  int position;
  int step = 0;
  int center = 2000;
  int out1 = 0;
  int out2 = 0;
} PulseStruct;

// Pattern types supported:
enum pattern { NONE, RAINBOW_CYCLE, RAINBOW_FADE, FADE, MUSIC_LED_HUE, PULSE, MUSIC_FILL };
// Patern directions supported:
enum direction { FORWARD, REVERSE };

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    private:
    const int num_pulses = 10;
    PulseStruct *pulses[10] = {NULL};
    public:

    // Member Variables:
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern

    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position

    uint8_t brightness;   // strip brightness
    uint8_t numLeds;    // num leds to turn on with audio

    long lColor1;   // signed color 1
    long lColor2;   // signed color 2
    long colorPad;    // enough room for a color +- some padding

    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern

    void (*OnComplete)();  // Callback on completion of pattern

    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }

    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                case RAINBOW_FADE:
                    RainbowFadeUpdate();
                    break;
                case MUSIC_LED_HUE:
                    MusicLedHueUpdate();
                    break;
                case PULSE:
                    PulseUpdate();
                    break;
                case MUSIC_FILL:
                    MusicFillUpdate();
                    break;
                default:
                    break;
            }
        }
    }

    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the completion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }

    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }

    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, uint8_t br, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
        brightness = br;
    }

    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i = 0; i <  numPixels();  i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        show();
        Increment();
    }

    // Init rainbow fade
    void RainbowFade(uint8_t br) {
      ActivePattern = RAINBOW_FADE;
      Index = 0;
      TotalSteps = 16383;
      Direction = FORWARD;
      brightness = br;
    }

    // Update rainbow fade
    void RainbowFadeUpdate() {
      ColorSet(gamma32(ColorHSV(map(Index, 0, TotalSteps, 0, 65535), 255, brightness)));
      show();
      Increment();
    }

    // Init music reactivity changing hue and amount of illuminated LEDs
    void MusicLedHue() {
      ActivePattern = MUSIC_LED_HUE;
      Index = 0;
      TotalSteps = 65535;
      Interval = 0;
    }

    // Update strip leds, hue to music
    void MusicLedHueUpdate() {
      // Turn on number of LEDs respective to how loud it is
      for(int i = 0; i < numPixels(); i++) {
        if (numLeds > 1 && i <= numLeds) {
          // Set HSV value of current pixel, with hue constantly updating and based on position in strip
          setPixelColor(i, gamma32(ColorHSV(map(i + Index, 0, numPixels(), 0, 65535), 255, brightness)));
        }
        else
          setPixelColor(i, Color(0, 0, 0));
     }
     // Display strip
     show();

     // Rate at which hue updates depends on volume level
     int rateOfChange = 10;
     // The louder the volume, the faster the hue updates
     if(numLeds <= 20)
       rateOfChange = 50;
     else if(numLeds > 20 && numLeds <= 40)
       rateOfChange = 35;
     else if(numLeds > 40 && numLeds <= 60)
       rateOfChange = 25;
     else if(numLeds > 60 && numLeds <= 80)
       rateOfChange = 10;
     else if(numLeds > 80 && numLeds <= 90)
       rateOfChange = 5;
     else if(numLeds > 90)
       rateOfChange = 1;

      if(millis() % rateOfChange == 0) {
        Increment();
      }
    }

    void Pulse() {
      ActivePattern = PULSE;
      Index = 0;
      TotalSteps = 7;
      Color1 = 65535;
      // lColor1 = 21844;
      // colorPad = lColor1;
      // lColor2 = lColor1;
    }

    void PulseUpdate() {
      // 00000 - red
      // 10922 - magenta
      // 21844 - blue
      // 32768 - cyan
      // 43688 - green
      // 54610 - yellow
      // 65535 - red
      for(int i = 0; i < numPixels(); i++) {
        setPixelColor(i, ColorHSV(Color1, 255, brightness));
      }

      // If struct at index n is empty
      // malloc some space for it
      // n is determined by volume level

      if(pulses[0] == NULL) {
        pulses[0] = malloc(sizeof(PulseStruct));
        pulses[0]->position = 5;
        //setPixelColor(5, ColorHSV(Color1 - 10000, 255, brightness));
      }
      else{
        //setPixelColor(5, ColorHSV(Color1 + 2000, 255, brightness));
        pulseLed(pulses[0], 0);
      }
      show();
      if(millis() % 100 == 0)
        Increment();
    }

    void pulseLed(PulseStruct *pulse, int idx) {
      // 0 - center 2000
      // 1 - 1 out, center 1500
      // 2 - 2 out, center 1000
      // 3 - 2 at 3/4 bright, center 500
      // 4 - 2 at 1/2 bright, center off
      // 5 - all off
      int s = pulse->step;
      if(s > 0 && s < 5) {
        pulse->center -= 500;
        if(s == 1)
          pulse->out1 = 2000;
        if(s == 2)
          pulse->out2 = 2000;
        if(s > 1)
          pulse->out1 -= 500;
      }
      if(s > 2)
        pulse->out2 -= 500;

     setPixelColor(pulse->position - 2, ColorHSV(Color1 + pulse->out2, 255, brightness));
     setPixelColor(pulse->position - 1, ColorHSV(Color1 + pulse->out1, 255, brightness));
     setPixelColor(pulse->position, ColorHSV(Color1 + pulse->center, 255, brightness));
     setPixelColor(pulse->position + 1, ColorHSV(Color1 + pulse->out1, 255, brightness));
     setPixelColor(pulse->position + 2, ColorHSV(Color1 + pulse->out2, 255, brightness));

      pulse->step++;

      if(pulse->step == 5) {
        free(pulse);
        pulses[idx] = NULL;
      }
    }

    /**
     * Given a hue, updates it circularly
     * @param color a reference to the color to update
     * @param cPad the initial color. has room for addition of a cap
     * @param cap how far to update the color before switching directions
     * @param stepLen the factor to update the hue by
     * @param dir a reference to the current direction we're moving
     **/
    void updateHue(long &color, long cPad, int32_t cap, int stepLen, bool startedFwd) {
      Serial.println(color);
      // Moving in the forward direction
      if(Direction == FORWARD) {
        // Increment color by stepLen until it overflows the cap
        color += stepLen;
        // At which point reverse direction
        if(color >= (startedFwd ? cPad + cap : cPad))
          Direction = REVERSE;
      }
      // Moving in the backwards direction
      else {
        // Decrement color by stepLen until it reaches the original colr
        color -= stepLen;
        // At which point reverse direction
        if(color <= (startedFwd ? cPad : cPad - cap))
          Direction = FORWARD;
      }
    }

    void MusicFill(uint32_t h) {
      ActivePattern = MUSIC_FILL;
      Index = 0;
      TotalSteps = 65535;
      Interval = 0;
      Color2 = h;
    }

    void MusicFillUpdate() {
      // Turn on number of LEDs respective to how loud it is
      for(int i = 0; i < numPixels(); i++) {
        if (numLeds > 1 && i <= numLeds) {
          // Set HSV value of current pixel, with hue determined by user input
          setPixelColor(i, gamma32(ColorHSV(map(Color2, 0, 255, 0, 65535), 255, brightness)));
        }
        else
          setPixelColor(i, Color(0, 0, 0));
     }
     // Display strip
     show();
    }

    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }

    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;

        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }

    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        double factor = (brightness / 255.0);
        if(WheelPos < 85)
        {
            return Color((255 - WheelPos * 3) * factor, 0, (WheelPos * 3) * factor);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, (WheelPos * 3) * factor, (255 - WheelPos * 3) * factor);
        }
        else
        {
            WheelPos -= 170;
            return Color((WheelPos * 3) * factor, (255 - WheelPos * 3) * factor, 0);
        }
    }
};

#define SENS 25
#define BACKLIGHT 13
#define NUM_LEDS 100
#define ENVELOPE A1
#define LED_PIN 6
#define IR_PIN 8

// Define LCD pins
// Parameters: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 7, 5, 4, 3, 2);

// Define some NeoPatterns for the two rings and the stick
// as well as some completion routines
NeoPatterns strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800, NULL);//, &Ring1Complete);

//Initialize IR Receiver
IRrecv irrecv(IR_PIN);
// Get results
decode_results results;

//If the backlight is turned on
bool powerOn = true;

//List of all possible button inputs
String codes[22] = {"PWR", "VL+", "FNC", "LFT", "PP ", "RGT", "DWN", "VL-", "UP ",
"0  ", "EQ ", "ST ", "1  ", "2  ", "3  ", "4  ", "5  ", "6  ", "7  ", "8  ", "9  "};

/**
 * Possible strip modes
 * 0: Red
 * 1: Grn
 * 2: Blu
 * 3: Wht
 * 4: Rainbow fade
 * 5: Rainbow chase
 * 6: Music bar fill, rainbow
 * 7: Music bar fill, red
 * 8: Adjust hue, solid
 * 9: Adjust hue, music
**/
unsigned short modes[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

// Initialize button id to error value
int id = -1;
// Initialize LED mode to default value
unsigned short mode = 0;

// Whether we need to update the mode - dynamic modes need this
bool modeChange = false;

// How bright the lights are
int stripBrightness = 200;
// Current hue (valid for modes 8, 9)
int hue = 235;
// Brightness string
char br[4];
// Hue string
char h[4];

const int pause = 1000;
unsigned long previousMillis = 0;

void setup()
{
  Serial.begin(9600);

  // Enable backlight
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, powerOn);

  // Start IR Receiver
  irrecv.enableIRIn();

  // Start LCD screen
  lcd.begin(16,2);
  setupLCD();

  // Initialize all the pixelStrips
  strip.begin();
  // Start on default mode 0
  updateMode();
}

// Main loop
void loop()
{
  digitalWrite(BACKLIGHT, powerOn);

  // Get input from sound card, manipulate it to fit our number of leds
  if(mode == 6 || mode == 7 || mode == 9) {
    int input = analogRead(ENVELOPE);
    unsigned int reading = (input * input) / SENS;
    strip.numLeds = (reading);
  }

  if(mode == 9)
    strip.Color2 = hue;

  // Store index before receiving a signal
  int tempI = strip.Index;

  // If we get something from the receiver
  if(irrecv.decode()) {
    // Any signal received during a dynamically updating pattern will be malformed, so we need a second input
    if(isModeDynamic()) {
      modeChange = true;
    }

    // Print input
    if(powerOn)
      codeToStr(irrecv.decodedIRData.decodedRawData);

    // Do the appropriate action depending on the pressed button
    performAction(id);

    // Restore signal after updating mode
    if(id != mode)
      strip.Index = tempI;

    // Resume receiving input
    irrecv.resume();
  }

  // Give users one second to input another IR signal
  unsigned long currentMillis = millis();
  if(modeChange) {
    if(currentMillis - previousMillis >= pause) {
      previousMillis = currentMillis;
      modeChange = false;
    }
  }

  // Update strip if dynamic and user has not requested an input
  if(powerOn && !modeChange && isModeDynamic()) {
    strip.brightness = stripBrightness;
    strip.Update();
  }
}

/**
* Convert an IR value to string to be printed
* @param hexVal the IR input
* @returns a human-readable string representing the button pushed,
* or 'bad' if input could not be understood
**/
void codeToStr(long hexVal) {
  switch(hexVal) {
    case 0:
      Serial.println("Repeat");
      break;
    case 0xBA45FF00: id = 0; break;
    case 0xB946FF00: id = 1; break;
    case 0xB847FF00: id = 2; break;
    case 0xBB44FF00: id = 3; break;
    case 0xBF40FF00: id = 4; break;
    case 0xBC43FF00: id = 5; break;
    case 0xF807FF00: id = 6; break;
    case 0xEA15FF00: id = 7; break;
    case 0xF609FF00: id = 8; break;
    case 0xE916FF00: id = 9; break;
    case 0xE619FF00: id = 10; break;
    case 0xF20DFF00: id = 11; break;
    case 0xF30CFF00: id = 12; break;
    case 0xE718FF00: id = 13; break;
    case 0xA15EFF00: id = 14; break;
    case 0xF708FF00: id = 15;break;
    case 0xE31CFF00: id = 16; break;
    case 0xA55AFF00:id = 17; break;
    case 0xBD42FF00:id = 18; break;
    case 0xAD52FF00: id = 19; break;
    case 0xB54AFF00:id = 20; break;
    default: id = -1; break;
    }

    lcd.setCursor(7, 1);

    //If ID is unchanged, we got a bad input
    if(id == -1)
      lcd.print("Bad");
    else {
      lcd.print(codes[id]);
    }
}

/**
* After determining the index of the string, perform appropriate action
* @param idx the pressed button
**/
void performAction(int idx) {
    // Power button pressed
    if (idx == 0) {
        powerOn = !powerOn;
        if(!powerOn) {
          lcd.clear();
          wipe();
        }
        else {
          setupLCD();
          updateMode();
          strip.show();
        }
    }
    if(powerOn) {
      //Hue shifting
      if(mode == 8 || mode == 9) {
        //Hue shifting left
        if(idx == 3) {
          if(hue > 0)
            hue--;
          else
            hue = 255;
        }
        //Hue shifting right
        else if(idx == 5) {
          if (hue < 255)
            hue++;
          else
            hue = 0;
        }
      }
      // Volume up button pressed
      if (idx == 1) {
        if(stripBrightness + 10 < 255)
          stripBrightness += 10;
        else if(stripBrightness + 10 >= 255)
          stripBrightness = 0;
      }
      // Down button pressed
      else if (idx == 6) {
        if(mode == 0)
          mode = 9;
        else
          mode--;
      }
      // Volume down button pressed
      else if(idx == 7) {
        if(stripBrightness - 10 > 0)
          stripBrightness -= 10;
        else if(stripBrightness - 10 <= 0)
          stripBrightness = 255;
      }
      // Up button pressed
      else if (idx == 8) {
        if (mode == 9)
          mode = 0;
        else
          mode++;
      }
      // Map button index to mode index
      else if (idx == 9 || (idx >= 12 && idx <= 20)) {
        if (idx == 9)
          mode = 0;
        else
          mode = idx - 11;
      }

      //Print mode
      lcd.setCursor(6, 0);
      lcd.print(mode);

      //Print brightness
      snprintf(br, sizeof(br), "%03d", stripBrightness);
      lcd.setCursor(13, 0);
      lcd.print(br);

      snprintf(h, sizeof(h), "%03d", hue);
      lcd.setCursor(13, 1);
      lcd.print(hue);

      updateMode();
    }
}

/**
 * Update the mode
 *
 */
void updateMode() {
  switch(mode) {
    // Solid Red
    case 0:
      strip.ColorSet(strip.Color(stripBrightness, 0, 0));
      strip.ActivePattern = NONE;
      break;
    // Solid Green
    case 1:
      strip.ColorSet(strip.Color(0, stripBrightness, 0));
      strip.ActivePattern = NONE;
      break;
    // Solid Blue
    case 2:
      strip.ColorSet(strip.Color(0, 0, stripBrightness));
      strip.ActivePattern = NONE;
      break;
    // Solid White
    case 3:
     strip.ColorSet(strip.gamma32(strip.ColorHSV(map(hue, 0, 255, 0, 65535), 0, stripBrightness)));
      strip.ActivePattern = NONE;
      break;
    // Rainbow Fade
    case 4:
      strip.RainbowFade(stripBrightness);
      strip.ActivePattern = RAINBOW_FADE;
      break;
    // Rainbow Cycle
    case 5:
      strip.RainbowCycle(100, stripBrightness);
      strip.ActivePattern = RAINBOW_CYCLE;
      break;
    // Music num leds
    case 6:
      strip.MusicLedHue();
      strip.ActivePattern = MUSIC_LED_HUE;
      strip.Interval = 0;
      break;
    // music pulse
    case 7:
      strip.Pulse();
      strip.ActivePattern = PULSE;
      break;
    // Solid Hue
    case 8:
      strip.ColorSet(strip.gamma32(strip.ColorHSV(map(hue, 0, 255, 0, 65535), 255, stripBrightness)));
      strip.ActivePattern = NONE;
      break;
    case 9:
      strip.MusicFill(hue);
      strip.ActivePattern = MUSIC_FILL;
      break;
    default:
      break;
  }
  strip.show();
}

/**
* Some reusable code for setting up LCD screen
**/
void setupLCD() {
  lcd.clear();

  snprintf(br, sizeof(br), "%03d", stripBrightness);
  snprintf(h, sizeof(h), "%03d", hue);

  lcd.setCursor(13, 0);
  lcd.print(br);

  lcd.setCursor(0, 0);
  lcd.print("Mode:    Br: ");

  lcd.setCursor(6, 0);
  lcd.print(mode);

  lcd.setCursor(0, 1);
  lcd.print("Input:");

  lcd.setCursor(11, 1);
  lcd.print("H: ");
  lcd.setCursor(13, 1);
  lcd.print(hue);
}

/**
 * Simple function to determine whether a mode needs constant updating
 * Returns true when:
 * Theme is default, modes are 4 - 9
 * Theme is Halloween, modes are
 * Theme is Xmas, modes are
 * @returns whether a mode needs constant updating
 */
bool isModeDynamic() {
  return (mode >= 4 && mode <= 7 || mode == 9);
}

/**
* Clear all LEDs
**/
void wipe() {
  for(int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}
