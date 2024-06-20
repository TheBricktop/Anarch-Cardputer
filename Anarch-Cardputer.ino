/**
  @file main_espboy.ino

  This is m5 Cardputer implementation of the game front end.

  by Miloslav Ciz (drummyfish), 2021

  Sadly compiling can't be done with any other optimization flag than -Os.

  Released under CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
  plus a waiver of all other intellectual property. The goal of this work is to
  be and remain completely in the public domain forever, available for any use
  whatsoever.
*/

#define SFG_CAN_SAVE 1 // for version without saving, set this to 0

#define SFG_FOV_HORIZONTAL 240

#define SFG_SCREEN_RESOLUTION_X 240
#define SFG_SCREEN_RESOLUTION_Y 135
#define SFG_FPS 30
#define SFG_RAYCASTING_MAX_STEPS 50
#define SFG_RAYCASTING_SUBSAMPLE 2
#define SFG_DIMINISH_SPRITES 1
#define SFG_CAN_EXIT 0
#define SFG_DITHERED_SHADOW 1
/*
#define SFG_SCREEN_RESOLUTION_X 128
#define SFG_SCREEN_RESOLUTION_Y 128
#define SFG_FPS 30
#define SFG_RAYCASTING_MAX_STEPS 40
#define SFG_RAYCASTING_SUBSAMPLE 2
#define SFG_CAN_EXIT 0
#define SFG_DITHERED_SHADOW 1
#define SFG_RAYCASTING_MAX_HITS 10
#define SFG_ENABLE_FOG 0
#define SFG_DIMINISH_SPRITES 0
#define SFG_ELEMENT_DISTANCES_CHECKED_PER_FRAME 16
#define SFG_FOG_DIMINISH_STEP 2048
//#define SFG_DIFFERENT_FLOOR_CEILING_COLORS 1
*/

#define SFG_AVR 1

#include "M5Cardputer.h"


#if SFG_CAN_SAVE
#include <EEPROM.h>
#define SAVE_VALID_VALUE 73
#endif

#include "game.h"

#include "sounds.h"

uint8_t *gamescreen;
uint8_t keys;

void SFG_setPixel(uint16_t x, uint16_t y, uint8_t colorIndex)
{
  gamescreen[y * SFG_SCREEN_RESOLUTION_X + x] = colorIndex;
}

void SFG_sleepMs(uint16_t timeMs)
{
}

uint32_t SFG_getTimeMs()
{
  return millis();
}

int8_t SFG_keyPressed(uint8_t key)
{
  switch (key)
  {
    case SFG_KEY_UP:    return keys & 0x02; break;
    case SFG_KEY_DOWN:  return keys & 0x04; break;
    case SFG_KEY_RIGHT: return keys & 0x08; break;
    case SFG_KEY_LEFT:  return keys & 0x01; break;
    case SFG_KEY_A:     return keys & 0x10; break;
    case SFG_KEY_B:     return keys & 0x20; break;
    case SFG_KEY_C:     return keys & 0x80; break;
    case SFG_KEY_MAP:   return keys & 0x40; break;
    default: return 0; break;
  }
}

void SFG_getMouseOffset(int16_t *x, int16_t *y)
{
}

int16_t activeSoundIndex = -1; //-1 means inactive
uint16_t activeSoundOffset = 0;
uint16_t activeSoundVolume = 0;

uint8_t musicOn = 0;

#define MUSIC_VOLUME   64

// ^ this has to be init to 0 (not 1), else a few samples get played at start

void ICACHE_RAM_ATTR audioFillCallback()
{
  int16_t s = 0;

  //SFG_musicTrackAverages should be not in program memory!
  if (musicOn) s = ((SFG_getNextMusicSample() - SFG_musicTrackAverages[SFG_MusicState.track]) & 0xffff) * MUSIC_VOLUME;

  if (activeSoundIndex >= 0)
  {
    s += (128 - SFG_GET_SFX_SAMPLE(activeSoundIndex, activeSoundOffset)) * activeSoundVolume;
    ++activeSoundOffset;
    if (activeSoundOffset >= SFG_SFX_SAMPLE_COUNT) activeSoundIndex = -1;
  }

  s = 128 + (s >> 5);
  if (s < 0) s = 0;
  if (s > 255) s = 255;

  sigmaDeltaWrite(0, s);
}

void SFG_setMusic(uint8_t value)
{
  switch (value)
  {
    case SFG_MUSIC_TURN_ON: musicOn = 1; break;
    case SFG_MUSIC_TURN_OFF: musicOn = 0; break;
    case SFG_MUSIC_NEXT: SFG_nextMusicTrack(); break;
    default: break;
  }
}

void SFG_save(uint8_t data[SFG_SAVE_SIZE])
{
#if SFG_CAN_SAVE
  EEPROM.write(0, SAVE_VALID_VALUE);

  for (uint8_t i = 0; i < SFG_SAVE_SIZE; ++i)
    EEPROM.write(i + 1, data[i]);

  EEPROM.commit();
#endif
}

void SFG_processEvent(uint8_t event, uint8_t data)
{
}

uint8_t SFG_load(uint8_t data[SFG_SAVE_SIZE])
{
#if SFG_CAN_SAVE
  if (EEPROM.read(0) == SAVE_VALID_VALUE)
    for (uint8_t i = 0; i < SFG_SAVE_SIZE; ++i)
      data[i] = EEPROM.read(i + 1);

  return 1;
#else
  return 0;
#endif
}

void SFG_playSound(uint8_t soundIndex, uint8_t volume)
{
  activeSoundOffset = 0;
  activeSoundIndex = soundIndex;
  activeSoundVolume = 1 << (volume / 37);
}

/* Draw a frame to the display without scaling.
// Not normally called. Edit the code to use this function
void draw_frame(uint32_t fb[144][160]) {
  for(unsigned int i = 0; i < LCD_WIDTH; i++) {
    for(unsigned int j = 0; j < LCD_HEIGHT; j++) {
      if(fb[j][i] != swap_fb[j][i]) {
        M5Cardputer.Display.drawPixel((int32_t)i, (int32_t)j, fb[j][i]);
      }
      swap_fb[j][i] = fb[j][i];
    } 
  }
}
*/

void setup()
{
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  gamescreen = new uint8_t [SFG_SCREEN_RESOLUTION_X * SFG_SCREEN_RESOLUTION_Y];
  int textsize = M5Cardputer.Display.height() / 60;
  if (textsize == 0) {
        textsize = 1;
    }
  M5Cardputer.Display.setTextSize(textsize);

#if SFG_CAN_SAVE
  EEPROM.begin(SFG_SAVE_SIZE + 1);
#endif

  SFG_init();

}


void loop() {

    SFG_mainLoopBody();
  /*
  Display code
  trying to determine how to print framebuffer to the screen 
  */
    static uint16_t scrbf[SFG_SCREEN_RESOLUTION_X];
    uint32_t readscrbf=0;
  
    for (uint32_t j=0; j<SFG_SCREEN_RESOLUTION_Y; j++){
      for (uint32_t i=0; i<SFG_SCREEN_RESOLUTION_X; i++){
        scrbf[i]=paletteRGB565[gamescreen[readscrbf++]];
        M5Cardputer.Display.drawPixel(i, j, scrbf[i]);
      }
        
    //myESPboy.tft.pushPixels(&scrbf, SFG_SCREEN_RESOLUTION_X);
  }


}
