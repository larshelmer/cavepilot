// Copyright (C) 2004 Lars Helmer <lasso@spacecentre.se>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  

#ifndef _GFXCORE_H
#define _GFXCORE_H

#include "SDL/SDL.h"
#include "Weapons.hh"
#include "Particles.hh"

#define LETTER_WIDTH 9
#define LETTER_HEIGHT 14

#define SMALL_LETTER_WIDTH 7
#define SMALL_LETTER_HEIGHT 8

#define PLAYER_WIDTH 31
#define PLAYER_HEIGHT 31

struct Bitmask
{
  int w, h;
  Uint32* bits;
};

class GfxCore
{
 private:
  Uint16 createHiColourPixel (SDL_PixelFormat*, Uint8, Uint8, Uint8);
  SDL_Surface* screen;
  SDL_Surface* font;
  SDL_Surface* smallFont;
  SDL_Surface* redShip;
  SDL_Surface* blueShip;
  SDL_Surface* greenShip;
  SDL_Surface* yellowShip;
  SDL_Surface* cave;
  SDL_Surface* collision;
  SDL_Surface* smallShips;
  bool up;
  Bitmask* shipMask;
  Bitmask* caveMask;
  Sint32 lvlWordWidth;
  Sint32 plWordWidth;
 public:
  GfxCore ();
  ~GfxCore ();
  bool isUp () { return up; };
  void clearScreen () { SDL_FillRect (screen, NULL, createHiColourPixel (screen->format, 0, 0, 0)); };
  void flipScreen () { SDL_Flip (screen); };
  bool loadConsoleFont ();
  bool loadSmallFont ();
  void drawConsole (Sint32, Sint32, Sint32, Sint32, Sint32, Sint32);
  void drawConsoleText (Sint32, Sint32, char*);
  void drawGameText (Sint32, Sint32, char*);
  bool loadShip (Uint8);
  void drawShip (double, double, double, Uint8);
  bool loadLevelImage (char*);
  bool loadCollisionImage (char*);
  void drawLevel (double, double);
  bool crash (double, double, double);
  bool isHit (double, double, double, double, double);
  bool hitsLevel (double, double);
  void drawShots (double, double, Shot*, Uint8, Uint8, Uint8);
  bool collides (double, double, double, double, double, double);
  void drawStatus (Uint8, Sint32, Sint32, Uint8, Uint8);
  void drawParticles (double, double, Particle*);
};

#endif // _GFXCORE_H 
