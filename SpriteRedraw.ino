/* * * * * * * * * * * * * * * * * * * * * * * * * *
 * The Pretendo R.A.G.E. entertainment system:
 * An open-source gaming design program based on the
 * Arduino Mega (ATMEGA2560) microcontroller.
 *
 * This is the 2016 junior design project by the
 * following students: Thaddeus Gulden, Christian
 * Dickinson and Talen Phillips.
 * 
 * The project uses the following components:
 *
 * The Arduino Mega 2560 development kit
 *
 * Adafruit ST7735 TFT and the associated libraries:
 *  ----> http://www.adafruit.com/products/358
 * 
 * The Microchip MCP4901 DAC, and LM386 Audio Op-Amp
 * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <SPI.h>
#include <avr/pgmspace.h>
//#include <PDQ_GFX.h>    // PDQ: Core graphics library
//#include "PDQ_ST7735_config.h"  // PDQ: ST7735 pins and other setup for this sketch
//#include <PDQ_ST7735.h>   // PDQ: Hardware-specific driver library
#include "bitmaps.h"




// Defunct code for the circle sprite
int sp_x = 80;
int sp_y = 64;

int b_reg = 0; //button poll register

int b_x, b_y, b_act;

int b_reg_p = 0;  //previous poll compare register
int dir = 0;    //determines ball direction

// DEFUNCT
int dir_lf, dir_rt, dir_up, dir_dn; // which button is pressed

#define TIMER_ISR_FIRED   1
unsigned int Status = 0;  // Keeps track of bit flags



struct BITMAP
{
    unsigned char width;
    unsigned char height;
    unsigned char invisible;
    unsigned char attributes;
    const unsigned char *BMP_ptr;
};


struct TILE
{
    BITMAP *BCKGND_ptr;
    TILE *UP_ptr;
    TILE *DN_ptr;
    TILE *LF_ptr;
    TILE *RT_ptr;    
};

#define BEHAVE_NO_MOVE      0
#define BEHAVE_DRIFT_RT     1 
#define BEHAVE_DRIFT_LT     2
#define BEHAVE_DRIFT_UP     3
#define BEHAVE_DRIFT_DN     4
#define BEHAVE_CIRCLE_CW    5
#define BEHAVE_CIRCLE_CCW   6
#define BEHAVE_BOUNCE_HORIZ 7 
#define BEHAVE_BOUNCE_VERT  8
#define BEHAVE_RANDOM       9

#define BEHAVE_STATE_INIT   0
#define BEHAVE_STATE_RT     1
#define BEHAVE_STATE_LT     2
#define BEHAVE_STATE_UP     3
#define BEHAVE_STATE_DN     4

struct SPRITE
{
    BITMAP *Array[MAX_SPRITE_FRAMES];  
    unsigned char behavior;
    unsigned char behaveState;  //Default to BEHAVE_STATE_INIT
    unsigned char curBitMap;
    unsigned char updateDelay;
    unsigned char curUpdateDelayCount;
    unsigned char movementDelay;
    unsigned char curMovementDelayCount;
    unsigned char equipIndex;
    signed char top;
    signed char left;  
    signed char minX;
    signed char maxX;  
    signed char minY;
    signed char maxY;  
};

#define CHAR_STATE_STAND        0
#define CHAR_STATE_MOVE         1
#define CHAR_STATE_ACTIVE_A     2
#define CHAR_STATE_ACTIVE_B     3

#define CHAR_FACING_RT          0
#define CHAR_FACING_LT          1
#define CHAR_FACING_UP          2
#define CHAR_FACING_DN          3

struct CHARACTER
{
    BITMAP *Standing[MAX_CHARACTER_FRAMES]; 
    BITMAP *Moving[MAX_CHARACTER_FRAMES]; 
    BITMAP *ActiveA[MAX_CHARACTER_FRAMES]; 
    BITMAP *ActiveB[MAX_CHARACTER_FRAMES]; 
    unsigned char charState;
    unsigned char curBitMap;
    unsigned char updateDelay;
    unsigned char curUpdateDelayCount;
    unsigned char facing;
    signed char top;
    signed char left;  
    signed char minX;
    signed char maxX;  
    signed char minY;
    signed char maxY;  
};

Bitmap Mountain(&mountain_bulk[0][0], 204, 64, BmpDir::Up, 0x01, false);

BITMAP mountain = {204, 64, 0x01, BMP_ATTR_NO_INVISIBLE | BMP_ATTR_DRAW_DIR_UP, &mountain_bulk[0][0]};

TILE start_tile = {&mountain, &start_tile, &start_tile, &start_tile, &start_tile};

//Spinning sword 5x5 diagonal and straight bitmaps (translated for each cardinal direction)

const static unsigned char Sword_N_bulk[5][5] PROGMEM =     {{Inv, Inv, Inv, Inv, Inv},
                                                             {Inv, Blu, Inv, Inv, Inv},
                                                             {Blu, Blu, LGy, LGy, LGy},
                                                             {Inv, Blu, Inv, Inv, Inv},
                                                             {Inv, Inv, Inv, Inv, Inv}};
                                                         
const static unsigned char Sword_NE_bulk[5][5] PROGMEM =    {{Blu, Inv, Blu, Inv, Inv},
                                                             {Inv, Blu, Inv, Inv, Inv},
                                                             {Blu, Inv, LGy, Inv, Inv},
                                                             {Inv, Inv, Inv, LGy, Inv},
                                                             {Inv, Inv, Inv, Inv, LGy}};

/*
const static unsigned char Character[9][9] PROGMEM =  {{Inv, Inv, Inv, Inv, Inv, Inv, Inv, Inv, Inv},
                                                       {Inv, Inv, Inv, Inv, Wht, Wht, Inv, Inv, Inv},
                                                       {Inv, Inv, Inv, Wht, LGy, LGy, Wht, Inv, Brn},
                                                       {Inv, Grn, Ylw, Wht, LGy, LGy, Wht, Grn, Brn},
                                                       {Grn, Grn, Tan, Tan, Wht, Wht, Wht, Grn, Inv},
                                                       {Inv, Grn, Ylw, Wht, Grn, Grn, Brn, Grn, Brn},
                                                       {Inv, Inv, Inv, Blu, Grn, Inv, Inv, Inv, Brn},
                                                       {LGy, LGy, LGy, Blu, Tan, Inv, Inv, Inv, Inv},
                                                       {Inv, Inv, Inv, Blu, Inv, Inv, Inv, Inv, Inv}};
*/
const unsigned char Character [8][10] PROGMEM = {
  {0xFF, 0xBA, 0x50, 0x28, 0x24, 0x92, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xDF, 0x2C, 0x50, 0x4D, 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0x34, 0x38, 0x50, 0x50, 0x8D, 0x92, 0x4D, 0x28, 0xB6, 0xFF},
  {0x38, 0x18, 0x50, 0x6C, 0x91, 0x04, 0x48, 0x4C, 0x48, 0xB6},
  {0x54, 0x94, 0xD4, 0xD0, 0x8C, 0x68, 0x48, 0x71, 0x6D, 0x20},
  {0xD5, 0xD0, 0xD1, 0xB2, 0xFB, 0x8D, 0x4C, 0x50, 0x08, 0x60},
  {0xFA, 0xD0, 0x88, 0x49, 0x6D, 0xDB, 0xB1, 0x48, 0xDA, 0xB6},
  {0xFF, 0xD1, 0xB1, 0xFF, 0xDB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

BITMAP sprite8 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_UP, &Sword_N_bulk[0][0]};
BITMAP sprite7 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_UP, &Sword_NE_bulk[0][0]};
BITMAP sprite6 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_CCW, &Sword_N_bulk[0][0]};
BITMAP sprite5 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_CCW, &Sword_NE_bulk[0][0]};
BITMAP sprite4 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_DN, &Sword_N_bulk[0][0]};
BITMAP sprite3 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_DN, &Sword_NE_bulk[0][0]};
BITMAP sprite2 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_CW, &Sword_N_bulk[0][0]};
BITMAP sprite1 = {5, 5, Inv, BMP_ATTR_DRAW_DIR_CW, &Sword_NE_bulk[0][0]};

BITMAP char8 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char7 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char6 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char5 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char4 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char3 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char2 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char1 = {8, 10, 0xFF, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
/*
BITMAP char8 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char7 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char6 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char5 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char4 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char3 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char2 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
BITMAP char1 = {9, 9, Inv, BMP_ATTR_DRAW_DIR_UP, &Character[0][0]};
*/

CHARACTER MyCharacter {{&char2, NULL},                      //*Standing[MAX_CHARACTER_FRAMES]
                       {&char1, &char3, NULL},            //*Moving[MAX_CHARACTER_FRAMES]
                       {&char5, &char6, &char7, NULL},  //*ActiveA[MAX_CHARACTER_FRAMES]
                       {&char7, &char6, &char5, NULL},  //*ActiveB[MAX_CHARACTER_FRAMES]
                       CHAR_STATE_STAND,    //charState;
                       0,                   //curBitMap;
                       20,                   //updateDelay;
                       0,                   //curUpdateDelayCount;
                       CHAR_FACING_RT,      //facing
                       50,                  //top
                       0,                   //left
                       -2,                  //minX
                       78,                  //maxX
                       -128,                //minY
                       127};                //maxY

SPRITE mySprite = {{&sprite1 , &sprite2, &sprite3 , &sprite4, &sprite5 , &sprite6, &sprite7 , &sprite8, NULL,}, //*Array[MAX_SPRITE_FRAMES]
                    BEHAVE_DRIFT_LT,  //behavior
                    BEHAVE_STATE_INIT,  //behaveState
                    0,          //curBitMap
                    5,          //updateDelay
                    0,          //curUpdateDelayCount
                    1,          //movementDelay
                    0,          //curMovementDelayCount
                    0xFF,       //equipIndex
                    50,         //top
                    0,          //left
                    -2,       //minX
                    78,       //maxX
                    -128,       //minY
                    127};       //maxY


SPRITE mySprite2 = {{&sprite1 , &sprite2, &sprite3 , &sprite4, &sprite5 , &sprite6, &sprite7 , &sprite8, NULL,}, //*Array[MAX_SPRITE_FRAMES]
                    BEHAVE_DRIFT_DN,  //behavior
                    BEHAVE_STATE_INIT,  //behaveState
                    3,      //curBitMap
                    5,          //updateDelay
                    0,          //curUpdateDelayCount
                    1,          //movementDelay
                    0,          //curMovementDelayCount
                    0xFF,       //equipIndex
                    50,         //top
                    26,         //left
                    -2,       //minX
                    78,     //maxX
                    -2,       //minY
                    64};      //maxY

SPRITE mySprite3 = {{&sprite1 , &sprite2, &sprite3 , &sprite4, &sprite5 , &sprite6, &sprite7 , &sprite8, NULL,}, //*Array[MAX_SPRITE_FRAMES]
                    BEHAVE_DRIFT_UP,  //behavior
                    BEHAVE_STATE_INIT,  //behaveState
                    5,          //curBitMap
                    5,          //updateDelay
                    0,          //curUpdateDelayCount
                    1,          //movementDelay
                    0,          //curMovementDelayCount
                    0xFF,       //equipIndex
                    50,         //top
                    52,         //left
                    -2,       //minX
                    78,     //maxX
                    -2,       //minY
                    64};      //maxY


SPRITE mySprite4 = {{&sprite1 , &sprite2, &sprite3 , &sprite4, &sprite5 , &sprite6, &sprite7 , &sprite8, NULL,}, //*Array[MAX_SPRITE_FRAMES]
                    BEHAVE_CIRCLE_CW,   //behavior
                    BEHAVE_STATE_INIT,  //behaveState
                    5,          //curBitMap
                    5,          //updateDelay
                    0,          //curUpdateDelayCount
                    1,          //movementDelay
                    0,          //curMovementDelayCount
                    0xFF,       //equipIndex
                    20,         //top
                    10,         //left
                    20,       //minX
                    50,       //maxX
                    10,       //minY
                    40};      //maxY
                    
SPRITE mySprite5 = {{&sprite8 , &sprite7, &sprite6 , &sprite5, &sprite4 , &sprite3, &sprite2 , &sprite1, NULL,}, //*Array[MAX_SPRITE_FRAMES]
                    BEHAVE_NO_MOVE,   //behavior
                    BEHAVE_STATE_INIT,  //behaveState
                    5,          //curBitMap
                    5,          //updateDelay
                    0,          //curUpdateDelayCount
                    1,          //movementDelay
                    0,          //curMovementDelayCount
                    0xFF,       //equipIndex
                    25,         //top
                    35,         //left
                    20,       //minX
                    70,       //maxX
                    10,       //minY
                    50};      //maxY

SPRITE mySprite6 = {{&sprite1 , &sprite2, &sprite3 , &sprite4, &sprite5 , &sprite6, &sprite7 , &sprite8, NULL,}, //*Array[MAX_SPRITE_FRAMES]
                    BEHAVE_BOUNCE_HORIZ, //behavior
                    BEHAVE_STATE_INIT, //behaveState
                    5,      //curBitMap
                    5,      //updateDelay
                    0,      //curUpdateDelayCount
                    1,      //movementDelay
                    0,      //curMovementDelayCount
                    0xFF,   //equipIndex
                    0,     //top
                    0,     //left
                    0,   //minX
                    80,    //maxX
                    0,   //minY
                    64};   //maxY

SPRITE mySprite7 = {{&sprite1 , &sprite2, &sprite3 , &sprite4, &sprite5 , &sprite6, &sprite7 , &sprite8, NULL,}, //*Array[MAX_SPRITE_FRAMES]
                    BEHAVE_BOUNCE_VERT, //behavior
                    BEHAVE_STATE_INIT,  //behaveState
                    5,          //curBitMap
                    5,          //updateDelay
                    0,          //curUpdateDelayCount
                    1,          //movementDelay
                    0,          //curMovementDelayCount
                    0xFF,       //equipIndex
                    0,        //top
                    0,        //left
                    0,      //minX
                    80,       //maxX
                    0,      //minY
                    64};      //maxY

TILE      *CurTile        = NULL;
CHARACTER *CurCharacter   = NULL;

/***************************SETUP*************************************/

void setup(void) {
  // CHANGE THIS: Put pins on one port, and use PORT[X] = [binary number]
  pinMode(b_up, INPUT);
  pinMode(b_dn, INPUT);
  pinMode(b_lf, INPUT);
  pinMode(b_rt, INPUT);
  pinMode(b_a, INPUT);
  pinMode(b_b, INPUT);  //BUTTON INPUT CONFIG
  pinMode(SD_CS, OUTPUT);
  pinMode(DAC_CS, OUTPUT);
  digitalWrite(DAC_CS, HIGH);

  Serial.begin(9600);

#define ST7735_RST_PIN    49   //use these for mega2560

#if defined(ST7735_RST_PIN)        // reset like Adafruit does 
  FastPin<ST7735_RST_PIN>::setOutput();
  FastPin<ST7735_RST_PIN>::hi();
  FastPin<ST7735_RST_PIN>::lo();
  delay(1);
  FastPin<ST7735_RST_PIN>::hi();
#endif

  tft.begin();            // use instead of tft.initR(INITR_BLACKTAB) to initialize display for PDQ library

  newTile(&start_tile); // draw the background

  cli();    //? stop interrupts

  //set timer0 interrupt at 60Hz
  TCCR0A = 0;// set entire TCCR0A register to 0
  TCCR0B = 0;// same for TCCR0B
  TCNT0  = 0;//initialize counter value to 0
  // set compare match  for 60Hz increments
  OCR0A = 255;// = (16*10^6) / (60*1024) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set timer config bits for 1024 prescaler
  TCCR0B |= (1 << CS02) | (0 << CS01) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);

  sei();  //allow interrupts
}

#define butt_lf   0x01
#define butt_rt   0x02
#define butt_up   0x04
#define butt_dn   0x08
#define butt_a    0x10
#define butt_b    0x20

void readbuttons(signed char *x_update, signed char *y_update, bool *ActiveA, bool *ActiveB)
{
  unsigned int b_reg_x = (digitalRead(b_lf)) | (digitalRead(b_rt) << 1);
  unsigned int b_reg_y = (digitalRead(b_up) << 2) | (digitalRead(b_dn) << 3);
  unsigned int b_act = (digitalRead(b_a) << 4) | (digitalRead(b_b) << 5);

  if (b_reg_x == butt_lf) *x_update = -1;
  else if (b_reg_x == butt_rt) *x_update = 1;
  else *x_update = 0;

  if (b_reg_y == butt_dn) *y_update = 1;
  else if (b_reg_y == butt_up) *y_update = -1;
  else *y_update = 0;

  *ActiveA = false;
  *ActiveB = false;
  if (b_act == butt_a) *ActiveA = true;
  else if (b_act == butt_b) *ActiveB = true;
}


/* * * * * * * * * * * * * Main game loop * * * * * * * * * * * * * */
void loop()
{
  signed char x_update = 0; 
  signed char y_update = 0;
  bool ActiveA = false;
  bool ActiveB = false;  
  
  if (Status & TIMER_ISR_FIRED)
  {
    Status &= ~TIMER_ISR_FIRED;  
    readbuttons(&x_update, &y_update, &ActiveA, &ActiveB);
    updateCharacter(x_update, y_update, ActiveA, ActiveB);
    updateSprite(&mySprite);
    updateSprite(&mySprite2);
    updateSprite(&mySprite3);
    updateSprite(&mySprite4);
    updateSprite(&mySprite5);
    updateSprite(&mySprite6);
    updateSprite(&mySprite7);
  }
}

/* * * * * * * * * * * * * FRAME REDRAW MEDIATION * * * * * * * * * * * * */
// AND MAYBE GAME LOGIC AND BUTTON POLL (both at ~60Hz, so same interrupt is feasible)
// REPLACE WITH PORT POLL
ISR(TIMER0_COMPA_vect)
{
  Status |= TIMER_ISR_FIRED;
}



BITMAP **GetCharBitMapArray(unsigned char charState)
{
    switch (charState)
    {
      case CHAR_STATE_ACTIVE_A:
        return &CurCharacter->ActiveA[0]; 
      break;
      
      case CHAR_STATE_ACTIVE_B:
        return &CurCharacter->ActiveB[0]; 
      break;      
      
      case CHAR_STATE_MOVE:
        return &CurCharacter->Moving[0]; 
      break;

      default: //CHAR_STATE_STAND
        return &CurCharacter->Standing[0];      
      break;
    }
}

void updateCharacter(signed char x_update, signed char y_update, bool ActiveA, bool ActiveB)
{
  bool overheadView = true;
  unsigned char lastFacing = CurCharacter->facing;
  unsigned char lastState = CurCharacter->charState;
  unsigned char lastBitMap = CurCharacter->curBitMap;
  signed char   lastLeft = CurCharacter->left;
  signed char   lastTop = CurCharacter->top;
  BITMAP        **lastArray = GetCharBitMapArray(CurCharacter->charState);
  BITMAP        **curArray;

  if (CurCharacter != NULL)
  {
    switch (CurCharacter->charState)
    {
      case CHAR_STATE_ACTIVE_A:
      break;
      
      case CHAR_STATE_ACTIVE_B:
      break;      
      
      case CHAR_STATE_MOVE:
        if (ActiveA) CurCharacter->charState = CHAR_STATE_ACTIVE_A;
        else if (ActiveB) CurCharacter->charState = CHAR_STATE_ACTIVE_B;
        else if ((x_update == 0) || (overheadView && (y_update == 0))) CurCharacter->charState = CHAR_STATE_STAND;   
      break;

      default: //CHAR_STATE_STAND
        if (ActiveA) CurCharacter->charState = CHAR_STATE_ACTIVE_A;
        else if (ActiveB) CurCharacter->charState = CHAR_STATE_ACTIVE_B;
        else if ((x_update != 0) || (overheadView && (y_update != 0))) CurCharacter->charState = CHAR_STATE_MOVE;             
      break;
    }

    if (x_update > 0) CurCharacter->facing = CHAR_FACING_RT;
    else if (x_update < 0) CurCharacter->facing = CHAR_FACING_LT;
    else if (overheadView && (y_update > 0)) CurCharacter->facing = CHAR_FACING_UP;
    else if (overheadView && (y_update < 0)) CurCharacter->facing = CHAR_FACING_DN;

    if (CurCharacter->charState != lastState)
    {
      CurCharacter->curBitMap = 0;
      CurCharacter->curUpdateDelayCount = 0;
    }

    //Update new BitMap and Count
    CurCharacter->curUpdateDelayCount++;
    if (CurCharacter->curUpdateDelayCount >= CurCharacter->updateDelay)
    {
      CurCharacter->curUpdateDelayCount = 0;
      CurCharacter->curBitMap++;

      if ((CurCharacter->curBitMap >= MAX_CHARACTER_FRAMES) || (GetCharBitMapArray(CurCharacter->charState)[CurCharacter->curBitMap] == NULL))
      {
        CurCharacter->curBitMap = 0;
        if ((CurCharacter->charState == CHAR_STATE_ACTIVE_A) || (CurCharacter->charState == CHAR_STATE_ACTIVE_B))
        {
          if ((x_update != 0) || (overheadView && (y_update != 0))) CurCharacter->charState = CHAR_STATE_MOVE;
          else CurCharacter->charState = CHAR_STATE_STAND; 
        }
      }
    }

    CurCharacter->left += x_update;
    if (overheadView) CurCharacter->top += y_update;

    curArray = GetCharBitMapArray(CurCharacter->charState);
  
    // redraw the newly exposed background
    if ((lastLeft != CurCharacter->left) || (lastTop != CurCharacter->top) || (lastBitMap != CurCharacter->curBitMap) || (lastState != CurCharacter->charState))
    {
      replaceBackGround(lastLeft, lastTop, lastArray[lastBitMap]->width, lastArray[lastBitMap]->height, CurTile->BCKGND_ptr);
  
      if (CurCharacter->facing == CHAR_FACING_RT) curArray[CurCharacter->curBitMap]->attributes = BMP_ATTR_DRAW_DIR_CCW;
      else if (CurCharacter->facing == CHAR_FACING_LT) curArray[CurCharacter->curBitMap]->attributes = BMP_ATTR_DRAW_DIR_CW;
      else if (CurCharacter->facing == CHAR_FACING_UP) curArray[CurCharacter->curBitMap]->attributes = BMP_ATTR_DRAW_DIR_UP;
      else if (CurCharacter->facing == CHAR_FACING_DN) curArray[CurCharacter->curBitMap]->attributes = BMP_ATTR_DRAW_DIR_DN;
  
      drawBitMap(CurCharacter->left, CurCharacter->top, curArray[CurCharacter->curBitMap], false);
    }
    
  }

  
}

void updateSprite(SPRITE *sprite_ptr)
{
  signed char incX = 0;
  signed char incY = 0;
  bool        stopReached = false;
  bool        wrapAround = false;
  unsigned char oldBitMap = sprite_ptr->curBitMap;
  unsigned char oldLeft = sprite_ptr->left;
  unsigned char oldTop = sprite_ptr->top;
  
  //Update new BitMap and Count
  sprite_ptr->curUpdateDelayCount++;
  if (sprite_ptr->curUpdateDelayCount >= sprite_ptr->updateDelay)
  {
    sprite_ptr->curUpdateDelayCount = 0;
    sprite_ptr->curBitMap++;
    if ((sprite_ptr->curBitMap >= MAX_SPRITE_FRAMES) || (sprite_ptr->Array[sprite_ptr->curBitMap] == NULL))
    {
      sprite_ptr->curBitMap = 0;
    }
  }

  //Update movementy stuff
  sprite_ptr->curMovementDelayCount++;
  if (sprite_ptr->curMovementDelayCount >= sprite_ptr->movementDelay)
  {
    sprite_ptr->curMovementDelayCount = 0;
    
    //===================================================== CHOOSE BEHAVIOR =====================================================
    // we need to move, so look at behavior
    switch(sprite_ptr->behavior)
    {
      case BEHAVE_DRIFT_RT:
        incX = 1;
        wrapAround = true;
        break;
      
      case BEHAVE_DRIFT_LT:
        incX = -1;
        wrapAround = true;
      break;
      
      case BEHAVE_DRIFT_UP:
        incY = 1;
        wrapAround = true;
      break;
      
      case BEHAVE_DRIFT_DN:
        incY = -1;   
        wrapAround = true;
      break;
      
      case BEHAVE_CIRCLE_CW:
      case BEHAVE_CIRCLE_CCW:      
        switch(sprite_ptr->behaveState)
        {
          case BEHAVE_STATE_LT:
            incX = -1;
          break;
          case BEHAVE_STATE_UP:
            incY = 1;
          break;
          case BEHAVE_STATE_DN:
            incY = -1;
          break;
          default: //BEHAVE_STATE_INIT, BEHAVE_STATE_RT
            sprite_ptr->behaveState = BEHAVE_STATE_RT;
            incX = 1;
          break;
        }
      break;
      
       case BEHAVE_BOUNCE_HORIZ:
        switch(sprite_ptr->behaveState)
        {
          case BEHAVE_STATE_LT:
            incX = -1;
          break;
          default: //BEHAVE_STATE_INIT, BEHAVE_STATE_RT
            sprite_ptr->behaveState = BEHAVE_STATE_RT;
            incX = 1;
          break;
        }
        break;

      case BEHAVE_BOUNCE_VERT:
        switch(sprite_ptr->behaveState)
        {
          case BEHAVE_STATE_UP:
            incY = 1;
          break;          
          default: //BEHAVE_STATE_INIT, BEHAVE_STATE_DN
            sprite_ptr->behaveState = BEHAVE_STATE_DN;
            incY = -1;
          break;
        }
      break;
      
      default:  //BEHAVE_NO_MOVE:
        //do nothing
      break;      
    }

    //===================================================== DO ACTUAL MOVEMENT =====================================================
    sprite_ptr->left += incX;
    if (sprite_ptr->left < sprite_ptr->minX) 
    {
      if (wrapAround) 
      {
        sprite_ptr->left = sprite_ptr->maxX;  
      }
      else 
      {
        sprite_ptr->left = sprite_ptr->minX;  
      }
      stopReached = true; 
    }
    
    if (sprite_ptr->left > sprite_ptr->maxX)
    {
      if (wrapAround) 
      {
        sprite_ptr->left = sprite_ptr->minX;  
      }   
      else          
      {
        sprite_ptr->left = sprite_ptr->maxX;  
      }
      stopReached = true; 
    }

    sprite_ptr->top -= incY;  //Bigger Y values are on the bottom, subtracting makes it seem like they are on the top
    if (sprite_ptr->top < sprite_ptr->minY) 
    {
      if (wrapAround) 
      {
        sprite_ptr->top = sprite_ptr->maxY;   
      }
      else 
      {
        sprite_ptr->top = sprite_ptr->minY;
      }
      stopReached = true; 
    }
    
    if (sprite_ptr->top > sprite_ptr->maxY) 
    {
      if (wrapAround) 
      {
        sprite_ptr->top = sprite_ptr->minY; 
      }
      else 
      {
        sprite_ptr->top = sprite_ptr->maxY;  
      }
      stopReached = true; 
    }

    //===================================================== UPDATE BEHAVIOR =====================================================
    if (stopReached)
    {
      // we need to move, so look at behavior
      switch(sprite_ptr->behavior)
      {
        case BEHAVE_CIRCLE_CW:
          switch(sprite_ptr->behaveState)
          {
            case BEHAVE_STATE_RT:
              sprite_ptr->behaveState = BEHAVE_STATE_DN;
            break;
            case BEHAVE_STATE_DN:
              sprite_ptr->behaveState = BEHAVE_STATE_LT;
            break;            
            case BEHAVE_STATE_LT:
              sprite_ptr->behaveState = BEHAVE_STATE_UP;
            break;
            case BEHAVE_STATE_UP:
              sprite_ptr->behaveState = BEHAVE_STATE_RT;
            break;
          }
        break;

        case BEHAVE_CIRCLE_CCW:      
          switch(sprite_ptr->behaveState)
          {
            case BEHAVE_STATE_LT:
              sprite_ptr->behaveState = BEHAVE_STATE_DN;
            break;
            case BEHAVE_STATE_DN:
              sprite_ptr->behaveState = BEHAVE_STATE_RT;
            break;
            case BEHAVE_STATE_RT:
              sprite_ptr->behaveState = BEHAVE_STATE_UP;
            break;            
            case BEHAVE_STATE_UP:
              sprite_ptr->behaveState = BEHAVE_STATE_LT;
            break;
          }
        break;
        
        case BEHAVE_BOUNCE_HORIZ:      
          switch(sprite_ptr->behaveState)
          {
            case BEHAVE_STATE_LT:
              sprite_ptr->behaveState = BEHAVE_STATE_RT;
            break;
            case BEHAVE_STATE_RT:
              sprite_ptr->behaveState = BEHAVE_STATE_LT;
            break;            
          }
        break;

        case BEHAVE_BOUNCE_VERT:      
          switch(sprite_ptr->behaveState)
          {
            case BEHAVE_STATE_DN:
              sprite_ptr->behaveState = BEHAVE_STATE_UP;
            break;       
            case BEHAVE_STATE_UP:
              sprite_ptr->behaveState = BEHAVE_STATE_DN;
            break;
          }
        break;
      }    
    }
  }
  // redraw the newly exposed background
  if ((oldLeft != sprite_ptr->left) || (oldTop != sprite_ptr->top) || (oldBitMap != sprite_ptr->curBitMap))
  {
    replaceBackGround(oldLeft, oldTop, sprite_ptr->Array[oldBitMap]->width, sprite_ptr->Array[oldBitMap]->height, CurTile->BCKGND_ptr);

    drawBitMap(sprite_ptr->left, sprite_ptr->top, sprite_ptr->Array[sprite_ptr->curBitMap], false);
  }
}

void newTile(TILE *tile)
{
  CurTile = tile;
  drawBitMap(0, 0, tile->BCKGND_ptr, true);
  setCharacter(&MyCharacter, 10, 37);
}

void setCharacter(CHARACTER *character, signed char top, signed char left)
{
  character->top = top;
  character->left = left;
  CurCharacter = character;
}

void drawBitMap(signed char left, signed char top, const BITMAP *bmp, bool ignoreInvisible)
{
  signed char X, Y;
  unsigned char I, J;  
  unsigned char color8;

  for (I = 0; I < bmp->width; I++)
  {
    X = left + I;
    if ((X >= 0) && (X < COMP_X))
    {
      for (J = 0; J < bmp->height; J++)  
      {
        Y = top + J;
        if ((Y >= 0) && (Y < COMP_Y))
        {
          switch(bmp->attributes & BMP_ATTR_DRAW_DIR_MASK)
          {
            case BMP_ATTR_DRAW_DIR_UP:
              color8 = pgm_read_byte_near(bmp->BMP_ptr + (I * bmp->height) + J);
            break;

            case BMP_ATTR_DRAW_DIR_CW:
              color8 = pgm_read_byte_near(bmp->BMP_ptr + (J * bmp->height) + (bmp->width - 1 - I));
            break;

            case BMP_ATTR_DRAW_DIR_DN:
              color8 = pgm_read_byte_near(bmp->BMP_ptr + ((bmp->width - 1 - I) * bmp->height) + (bmp->height - 1 - J));
            break;

            case BMP_ATTR_DRAW_DIR_CCW:
              color8 = pgm_read_byte_near(bmp->BMP_ptr + ((bmp->height - 1 - J) * bmp->height) + I);
            break;
          }
          if ((color8 != bmp->invisible) || ignoreInvisible || ((bmp->attributes & BMP_ATTR_NO_INVISIBLE) == BMP_ATTR_NO_INVISIBLE))
          {
            drawPixel(X, Y, color8); 
          }
        }
      }
    }
  }
}

void replaceBackGround(signed char left, signed char top, unsigned char width, unsigned char height, const BITMAP *BackGroundPtr)
{
  signed char X, Y;
  unsigned char I, J;  
  unsigned char color8;

  for (I = 0; I < width; I++)
  {
    X = left + I;
    if ((X >= 0) && (X < COMP_X))
    {
      for (J = 0; J < height; J++)  
      {
        Y = top + J;
        if ((Y >= 0) && (Y < COMP_Y))
        {
          color8 = pgm_read_byte_near(BackGroundPtr->BMP_ptr + (X * BackGroundPtr->height) + Y);
          drawPixel(X, Y, color8); 
        }
      }
    }
  }
}
