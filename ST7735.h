#include <PDQ_GFX.h>            // PDQ: Core graphics library
#include "PDQ_ST7735_config.h"  // PDQ: ST7735 pins and other setup for this sketch
#include <PDQ_ST7735.h>         // PDQ: Hardware-specific driver library

PDQ_ST7735 tft;     //create LCD object (using pins in "PDQ_ST7735_config.h")

/* TFT display and SD card will share the hardware SPI interface.
 * Hardware SPI pins are specific to the Arduino board type and
 * cannot be remapped to alternate pins.  For Arduino Uno,
 * Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
 */

#define SD_CS      47
#define DAC_CS     46
#define TFT_CS     53
#define TFT_RST    49
#define TFT_DC     48  //use these for mega2560
// Screen Resolution
#define MAX_TFT_X           160
#define MAX_TFT_Y           128
// Resolution multiplier
#define MULT_X              2
#define MULT_Y              2
// Effective Resolution
#define COMP_X              (MAX_TFT_X / MULT_X)
#define COMP_Y              (MAX_TFT_Y / MULT_Y)
#define MAX_SPRITE_FRAMES     10
#define MAX_CHARACTER_FRAMES  4

/* Button pins. REPLACE WITH PORT POLLING
 * (NOTE: These should be on one 
 * port so that their state can be transferred
 * into a register rapidly)
 */
 
#define b_up    45
#define b_dn    41
#define b_lf    43
#define b_rt    39
#define b_a    35
#define b_b    37

// Colors for bitmaps
#define Blu   0x03
#define Red   0xE0
#define Tan   0xD8
#define Grn   0x1C
#define Ylw   0xFC
#define Vio   0xC3
#define Cyn   0x1B
#define Org   0xF4
#define Brn   0x6C
#define Blk   0x00
#define Wht   0xFF
#define LGy   0x92  // Light Gray
#define DGy   0x49  // Dark Gray
#define Inv   0x24  // defined invisible color for testing

// Draws a Pixel on the Compressed 8 bit screen
// x - Compressed 8 bit screen location
// y - Compressed 8 bit screen location
// color8 - Compressed 8 bit color
void drawPixel(signed int x, signed int y, unsigned char color8)
{
  // Convert to 24 bit color for TFT connectivity
  unsigned int color16 = ((unsigned int)(color8 & 0xE0) << 8) | ((unsigned int)(color8 & 0x1C) << 6) | ((unsigned int)(color8 & 0x03) << 3); //RRRG GGBB  to RRR0 0GGG 000B B000
  signed int newx = (x * MULT_X);
  signed int newy = (y * MULT_Y);  
  // Determine if sprite is on screen
  if ((newx >= 0) && (newx < MAX_TFT_X) && (newy >= 0) && (newy < MAX_TFT_Y))
  {
    // draw sprite
    tft.fillRect((MAX_TFT_Y - 2) - newy, newx, (signed int) MULT_X, (signed int) MULT_Y, color16); //Swapping x and y to rotate the screen
  }
}


