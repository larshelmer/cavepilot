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

#include "GfxCore.hh"
#include "Configuration.hh"
#include "Cavepilot.hh"
#include <iostream>
#include <ctype.h>
#include <string.h>
#include <math.h>

#ifdef _WINDOWS
#undef DATADIR
#define DATADIR "share/"
#else
#ifdef _DEVEL
#undef DATADIR
#define DATADIR "share/"
#endif
#endif

#define CONSOLEFONT "ConsoleFont.bmp"
#define SMALLFONT "SmallFont.bmp"
#define RED_SHIP "RedShip.bmp"
#define BLUE_SHIP "BlueShip.bmp"
#define GREEN_SHIP "GreenShip.bmp"
#define YELLOW_SHIP "YellowShip.bmp"
#define SMALL_SHIPS "SmallShips.bmp"

extern Configuration* conf;

GfxCore::GfxCore ()
{
  up = true;
  if (SDL_Init (SDL_INIT_VIDEO) < 0)
  {
    std::cout << "Unable to initialize SDL: " << SDL_GetError () << std::endl;
    up = false;
  }
  else
  {
    atexit (SDL_Quit);

    Uint32 flags = SDL_DOUBLEBUF;
    if (conf->getFullScreen ())
      flags = flags | SDL_FULLSCREEN;
    //    if (conf->getHwSurface ())
    //      flags = flags | SDL_HWSURFACE;
    
    if ((screen = SDL_SetVideoMode (conf->getScreenWidth (), conf->getScreenHeight (), 16, flags)) == NULL)
    {
      std::cout << "Unable to set video mode: " << SDL_GetError () << std::endl;
      up = false;
    }
    else
    {
      SDL_WM_SetCaption ("Cavepilot", "Cavepilot");
      SDL_ShowCursor (0);
    }
  }
  shipMask = NULL;
  caveMask = NULL;
  cave = NULL;
  collision = NULL;
  redShip = NULL;
  blueShip = NULL;
  greenShip = NULL;
  yellowShip = NULL;
}

GfxCore::~GfxCore ()
{

  SDL_ShowCursor(1);
  SDL_FreeSurface (screen);
  if (shipMask != NULL)
  {
    delete[] shipMask->bits;
    delete shipMask;
  }
  if (caveMask != NULL)
  {
    delete[] caveMask->bits;
    delete caveMask;
  }
  if (cave != NULL)
    SDL_FreeSurface (cave);
  if (redShip != NULL)
    SDL_FreeSurface (redShip);
  if (blueShip != NULL)
    SDL_FreeSurface (blueShip);
  if (greenShip != NULL)
    SDL_FreeSurface (greenShip);
  if (yellowShip != NULL)
    SDL_FreeSurface (yellowShip);
  if (smallShips != NULL)
    SDL_FreeSurface (smallShips);
  if (font != NULL)
    SDL_FreeSurface (font);
  if (smallFont != NULL)
    SDL_FreeSurface (smallFont);
}

// debugfunction
void printBitMask (Uint32 bm)
{
  Uint32 bc = bm;
  for (Sint32 i = 0; i < 32; i++)
  {
    if ((bc & 1) > 0)
      std::cout << "1";
    else
      std::cout << "0";
    bc >>= 1;
  }
  //  std::cout << std::endl;
}

bool GfxCore::collides (double x1, double y1, double f1, double x2, double y2, double f2)
{
  Sint32 pl1OffsX = (Sint32)((f1 * (180 / M_PI)) / 3);
  if (pl1OffsX < 0)
    pl1OffsX += 120;
  Sint32 pl2OffsX = (Sint32)((f2 * (180 / M_PI)) / 3);
  if (pl2OffsX < 0)
    pl2OffsX += 120;
  Uint32* plBits = shipMask->bits;
  Sint32 diffX = (Sint32)(x1 - x2);
  Sint32 diffY = (Sint32)(y1 - y2);
  Sint32 startY = (diffY < 0 ? -diffY : 0);
  Uint32 pl1Bits, pl2Bits;
  for (; startY < PLAYER_HEIGHT; startY++)
  {
    pl1Bits = *(plBits + startY * plWordWidth + pl1OffsX);
    pl2Bits = *(plBits + (startY + diffY) * plWordWidth + pl2OffsX);
    pl2Bits = (diffX < 0 ? pl2Bits >> -diffX : pl2Bits << diffX);
    if ((pl1Bits & pl2Bits) > 0)
      return true;
  }
  return false;
}

bool GfxCore::isHit (double plX, double plY, double plF, double sX, double sY)
{
  Sint32 plOffsX = (Sint32)((plF * (180 / M_PI)) / 3);
  if (plOffsX < 0)
    plOffsX += 120;
  Sint32 plOffsY = (Sint32)(sY - (plY - PLAYER_HEIGHT / 2));
  Sint32 shotOffsX = (Sint32)(sX - (plX - PLAYER_WIDTH / 2));
  //  std::cout << "plx: " << (Sint32)(plX - PLAYER_WIDTH / 2) << " shotx: " << (Sint32)sX << " xoffs: " << shotOffsX << std::endl;
  Uint32 shot = 0;
  shot |= 1;
  shot <<= (31 - shotOffsX);
  //  printBitMask (shot);
  //  printBitMask (*(shipMask->bits + plOffsY * plWordWidth + plOffsX));
  if ((shot & *(shipMask->bits + plOffsY * plWordWidth + plOffsX)) > 0)
    return true;
  return false;
}

bool GfxCore::hitsLevel (double x, double y)
{
  Uint32 shot = 0;
  shot |= 1;
  Sint32 bitOffs = (Sint32)x % 32;
  Sint32 shotOffsX = (Sint32)floor (x / 32);
  shot <<= (31 - bitOffs);
  if ((shot & *(caveMask->bits + (Sint32)y * lvlWordWidth + shotOffsX)) > 0)
    return true;
  return false;
}

void GfxCore::drawShots (double cameraX, double cameraY, Shot* shots, Uint8 r, Uint8 g, Uint8 b)
{
  Sint32 x, y;
  Shot* tmp = shots;
  Uint16* pixels;
  Uint16 colour = createHiColourPixel (screen->format, r, g, b);
  if (SDL_LockSurface (screen) != 0)
    return;
  pixels = (Uint16*)screen->pixels;
  while (tmp != NULL)
  {
    x = (Sint32)(tmp->x - cameraX);
    y = (Sint32)(tmp->y - cameraY);
    if ((x < 0) || (x >= conf->getViewWidth ()))
    {
      tmp = tmp->next;
      continue;
    }
    if ((y < 0) || (y > conf->getViewHeight ()))
    {
      tmp = tmp->next;
      continue;
    }
    pixels[(screen->pitch / 2) * y + x] = colour;
    tmp = tmp->next;
  }
  SDL_UnlockSurface (screen);
}

void GfxCore::drawParticles (double cameraX, double cameraY, Particle* part)
{
  Sint32 x, y;
  Particle* tmp = part;
  Uint16* pixels;
  if (SDL_LockSurface (screen) != 0)
    return;
  pixels = (Uint16*)screen->pixels;
  while (tmp != NULL)
  {
    x = (Sint32)(tmp->x - cameraX);
    y = (Sint32)(tmp->y - cameraY);
    if ((x < 0) || (x >= conf->getViewWidth ()))
    {
      tmp = tmp->next;
      continue;
    }
    if ((y < 0) || (y > conf->getViewHeight ()))
    {
      tmp = tmp->next;
      continue;
    }
    Uint16 colour = createHiColourPixel (screen->format, (Sint32)tmp->r, (Sint32)tmp->g, (Sint32)tmp->b);
    pixels[(screen->pitch / 2) * y + x] = colour;
    tmp = tmp->next;
  }
  SDL_UnlockSurface (screen);
}

void GfxCore::drawLevel (double x, double y)
{
  SDL_Rect src, dest;
  src.x = (Sint16)x;
  src.y = (Sint16)y;
  src.w = conf->getViewWidth ();
  src.h = conf->getViewHeight ();
  dest.x = 0;
  dest.y = 0;
  dest.w = conf->getViewWidth ();
  dest.h = conf->getViewHeight ();
  SDL_BlitSurface (cave, &src, screen, &dest);
}

// Unfortunately this "pixel-perfect" collision detection
// is a bit buggy and hence not anything like perfect...
bool GfxCore::crash (double x, double y, double f)
{
  if (x < -PLAYER_WIDTH || x > conf->getWorldWidth () + PLAYER_WIDTH)
    return false;
  if (y < -PLAYER_HEIGHT || y > conf->getWorldHeight () + PLAYER_HEIGHT)
    return false;

  Uint32* plBits = shipMask->bits;
  Uint32* lvlBits = caveMask->bits;
  Sint32 plOffsX = (Sint32)((f * (180 / M_PI)) / 3);
  if (plOffsX < 0)
    plOffsX += 120;
  Sint32 lvlOffsX = (Sint32)floor ((x - PLAYER_WIDTH / 2) / 32);
  Sint32 lvlOffsY = (Sint32)y - PLAYER_HEIGHT / 2;
  Sint32 bitOffs = ((Sint32)(x - PLAYER_WIDTH / 2)) % 32;
  Sint32 startY = 0;
  if (lvlOffsY < 0)
    startY = -lvlOffsY;
  Uint32 plWord = 0;
  Uint32 lvlWord;
  for (; (startY < PLAYER_HEIGHT) && (startY < startY + lvlOffsY); startY++)
  {
    if (lvlOffsX >= 0)
    {
      plWord = *(plBits + startY * plWordWidth + plOffsX);
      lvlWord = *(lvlBits + (startY + lvlOffsY) * lvlWordWidth + lvlOffsX);
      lvlWord <<= bitOffs;
      if ((plWord & lvlWord) > 0)
	return true;
    }
    if ((bitOffs > 0) && (lvlOffsX < lvlWordWidth))
    {
      lvlWord = *(lvlBits + (startY + lvlOffsY) * lvlWordWidth + lvlOffsX + 1);
      lvlWord >>= (32 - bitOffs);
      if ((plWord & lvlWord) > 0)
	return true;
    }
  }
  return false;
}

bool GfxCore::loadCollisionImage (char* lvl)
{
  if (strlen (lvl) < 1)
    return false;
  SDL_Surface* tmp;
  tmp = SDL_LoadBMP (lvl);
  if (tmp == NULL)
    return false;
  else
  {
    //    SDL_SetColorKey (tmp, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
    if (collision != NULL)
    {
      SDL_FreeSurface (collision);
      collision = NULL;
    }
    collision = SDL_DisplayFormat (tmp);
    if (collision == NULL)
      collision = tmp;
    else
      SDL_FreeSurface (tmp);
  }
  if (caveMask != NULL)
  {
    delete[] caveMask->bits;
    delete caveMask;
  }
  caveMask = new Bitmask;
  caveMask->w = collision->w;
  caveMask->h = collision->h;
  Sint32 mWidth = (Sint32)ceil (collision->w / 32); // rounded up
  caveMask->bits = new Uint32[mWidth * collision->h];
  memset (caveMask->bits, 0, mWidth * collision->h * 4); // clear the memory
  Uint32* bits = caveMask->bits;
  SDL_LockSurface (collision);
  Sint32 bpp = collision->format->BytesPerPixel;
  Uint8* p;
  for (Sint32 i = 0; i < collision->h; i++) // rows
  {
    for (Sint32 j = 0; j < mWidth; j++) // words
    {
      for (Sint32 k = 0; k < 32; k++) // bits
      {
	*(bits + i * mWidth + j) <<= 1;
	if ((k + j * 32) < collision->w)
	{
	  p = (Uint8*)collision->pixels + i * collision->pitch + (k + j * 32) * bpp;
	  if (*(Uint16*)p != 0x00)
	    *(bits + i * mWidth + j) |= 1;
	}
      }
    }
  }
  SDL_UnlockSurface(collision);
  SDL_FreeSurface (collision); // collision map not needed anymore
  lvlWordWidth = mWidth;
  return true;
}

bool GfxCore::loadLevelImage (char* lvl)
{
  if (strlen (lvl) < 1)
    return false;
  SDL_Surface* tmp;
  tmp = SDL_LoadBMP (lvl);
  if (tmp == NULL)
    return false;
  else
  {
    SDL_SetColorKey (tmp, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
    if (cave != NULL)
    {
      SDL_FreeSurface (cave);
      cave = NULL;
    }
    cave = SDL_DisplayFormat (tmp);
    if (cave == NULL)
      cave = tmp;
    else
      SDL_FreeSurface (tmp);
  }
  conf->setWorldHeight (cave->h);
  conf->setWorldWidth (cave->w);
  return true;
}

void GfxCore::drawConsole (Sint32 vMargin, Sint32 hMargin, Sint32 aHeight, Sint32 aWidth, Sint32 iHeight, Sint32 iWidth)
{
  SDL_Rect dst;
  dst.x = hMargin;
  dst.y = vMargin;
  dst.w = aWidth;
  dst.h = aHeight;
  SDL_FillRect (screen, &dst, createHiColourPixel (screen->format, 50, 0, 20));
  dst.x = hMargin;
  dst.y = conf->getScreenHeight () - vMargin - iHeight;
  dst.w = iWidth;
  dst.h = iHeight;
  SDL_FillRect (screen, &dst, createHiColourPixel (screen->format, 50, 0, 20));
}

void GfxCore::drawConsoleText (Sint32 x, Sint32 y, char* text)
{
  if (text == NULL || (strlen (text) == 0))
    return;

  SDL_Rect src, dest;
  char curr;

  for (int i = 0; i <= (Sint32)strlen (text); i++)
  {
    curr = *(text + i);
    if (curr == 'å')
      curr = '_' + 1 - ' ';
    else if (curr == 'ä')
      curr = '_' + 2 - ' ';
    else if (curr == 'ö')
      curr = '_' + 3 - ' ';
    else if (toupper (curr) > '_')
      curr = 0;
    else
      curr = toupper (curr) - ' ';
    src.x = curr * LETTER_WIDTH;
    src.y = 0;
    src.w = LETTER_WIDTH;
    src.h = LETTER_HEIGHT;
    dest.x = (int)x + i * LETTER_WIDTH;
    dest.y = y;
    dest.w = LETTER_WIDTH;
    dest.h = LETTER_HEIGHT;
    SDL_BlitSurface (font, &src, screen, &dest);
  }
}

void GfxCore::drawGameText (Sint32 x, Sint32 y, char* text)
{
  if (text == NULL || (strlen (text) == 0))
    return;

  SDL_Rect src, dest;
  char curr;

  for (int i = 0; i <= (Sint32)strlen (text); i++)
  {
    curr = *(text + i);
    if (curr == 'å')
      curr = '_' + 1 - ' ';
    else if (curr == 'ä')
      curr = '_' + 2 - ' ';
    else if (curr == 'ö')
      curr = '_' + 3 - ' ';
    else if (toupper (curr) > '_')
      curr = 0;
    else
      curr = toupper (curr) - ' ';
    src.x = curr * SMALL_LETTER_WIDTH;
    src.y = 0;
    src.w = SMALL_LETTER_WIDTH;
    src.h = SMALL_LETTER_HEIGHT;
    dest.x = (int)x + i * SMALL_LETTER_WIDTH;
    dest.y = y;
    dest.w = SMALL_LETTER_WIDTH;
    dest.h = SMALL_LETTER_HEIGHT;
    SDL_BlitSurface (smallFont, &src, screen, &dest);
  }
}

bool GfxCore::loadConsoleFont ()
{
  SDL_Surface* tmp;
  char* path = new char[strlen (DATADIR) + strlen ("gfx/") + strlen (CONSOLEFONT) + 1];
  *path = 0;
  strcat (path, DATADIR);
  strcat (path, "gfx/");
  strcat (path, CONSOLEFONT);
  tmp = SDL_LoadBMP (path);
  delete[] path;
  if (tmp == NULL)
    return false;
  SDL_SetColorKey (tmp, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);
  font = SDL_DisplayFormat (tmp);
  if (font == NULL)
    font = tmp;
  else
    SDL_FreeSurface (tmp);
  return true;
}

bool GfxCore::loadSmallFont ()
{
  SDL_Surface* tmp;
  char* path = new char[strlen (DATADIR) + strlen ("gfx/") + strlen (SMALLFONT) + 1];
  *path = 0;
  strcat (path, DATADIR);
  strcat (path, "gfx/");
  strcat (path, SMALLFONT);
  tmp = SDL_LoadBMP (path);
  delete[] path;
  if (tmp == NULL)
    return false;
  SDL_SetColorKey (tmp, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);
  smallFont = SDL_DisplayFormat (tmp);
  if (smallFont == NULL)
    smallFont = tmp;
  else
    SDL_FreeSurface (tmp);
  return true;
}

void GfxCore::drawShip (double x, double y, double facing, Uint8 colour)
{
  // INSERT CODE -- check with colour which ship to draw
  if (x < -PLAYER_WIDTH || x > conf->getViewWidth () + PLAYER_WIDTH)
    return;
  if (y < -PLAYER_HEIGHT || y > conf->getViewHeight () + PLAYER_HEIGHT)
    return;
  
  SDL_Surface* ship;
  switch (colour)
  {
   case REDSHIP:
     ship = redShip;
     break;
   case BLUESHIP:
     ship = blueShip;
     break;
   case GREENSHIP:
     ship = greenShip;
     break;
   case YELLOWSHIP:
     ship = yellowShip;
     break;
   default:
     return;
  }

  SDL_Rect src, dest;
  Sint32 angle = (int)(facing * (180 / M_PI));
  if (angle < 0)
    angle += 360;
  src.x = PLAYER_WIDTH * (angle / 3);
  src.y = 0;
  src.w = PLAYER_WIDTH;
  src.h = PLAYER_HEIGHT;
  dest.x = (Sint16)x - PLAYER_WIDTH / 2;
  dest.y = (Sint16)y - PLAYER_HEIGHT / 2;
  dest.w = PLAYER_WIDTH;
  dest.h = PLAYER_HEIGHT;
  SDL_BlitSurface (ship, &src, screen, &dest);
}

bool GfxCore::loadShip (Uint8 colour)
{
  char* file;
  SDL_Surface* ship;
  switch (colour)
  {
   case REDSHIP:
     if (redShip != NULL)
       return true;
     file = new char[strlen (RED_SHIP) + 1];
     strcpy (file, RED_SHIP);
     break;
   case BLUESHIP:
     if (blueShip != NULL)
       return true;
     file = new char[strlen (BLUE_SHIP) + 1];
     strcpy (file, BLUE_SHIP);
     break;
   case GREENSHIP:
     if (greenShip != NULL)
       return true;
     file = new char[strlen (GREEN_SHIP) + 1];
     strcpy (file, GREEN_SHIP);
     break;
   case YELLOWSHIP:
     if (yellowShip != NULL)
       return true;
     file = new char[strlen (YELLOW_SHIP) + 1];
     strcpy (file, YELLOW_SHIP);
     break;
   default:
     return false;
  }

  char* path = new char[strlen (DATADIR) + strlen ("gfx/") + strlen (file) + 1];
  *path = 0;
  strcat (path, DATADIR);
  strcat (path, "gfx/");
  strcat (path, file);
  delete[] file;

  SDL_Surface* tmp;
  tmp = SDL_LoadBMP (path);
  if (tmp == NULL)
  {
    std::cout << "Cannot load ship graphics!" << std::endl;
    return false;
  }
  else
  {
    SDL_SetColorKey (tmp, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
    ship = SDL_DisplayFormat (tmp);
    if (ship == NULL)
      ship = tmp;
    else
      SDL_FreeSurface (tmp);
  }
  delete[] path;
  if (smallShips == NULL)
  {
    path = new char[strlen (DATADIR) + strlen ("gfx/") + strlen (SMALL_SHIPS) + 1];
    *path = 0;
    strcat (path, DATADIR);
    strcat (path, "gfx/");
    strcat (path, SMALL_SHIPS);
    
    tmp = SDL_LoadBMP (path);
    if (tmp == NULL)
    {
      std::cout << "Cannot load the small ships!" << std::endl;
    }
    else
    {
      SDL_SetColorKey (tmp, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
      smallShips = SDL_DisplayFormat (tmp);
      if (smallShips == NULL)
	smallShips = tmp;
      else
	SDL_FreeSurface (tmp);
    }
    delete[] path;
  }
  if (shipMask == NULL) // generate bitmask for ships
  {
    shipMask = new Bitmask;
    shipMask->w = ship->w; // 3720
    shipMask->h = ship->h;
    Sint32 mWidth = (ship->w + (ship->w / PLAYER_WIDTH)) / 32; // 120
    shipMask->bits = new Uint32[mWidth * PLAYER_HEIGHT];
    memset (shipMask->bits, 0, mWidth * PLAYER_HEIGHT * 4); // clear the memory
    Uint32* bits = shipMask->bits;
    SDL_LockSurface (ship);
    Sint32 bpp = ship->format->BytesPerPixel;
    Uint8* p;
    
    for (Sint32 i = 0; i < PLAYER_HEIGHT; i++) // rows
    {
      for (Sint32 j = 0; j < mWidth; j++) // words
      {
	for (Sint32 k = 0; k < 31; k++) // bits
	{
	  p = (Uint8*)ship->pixels + i * ship->pitch + (k + j * 31) * bpp;
	  if (*(Uint16*)p != 0x00)
	    *(bits + i * mWidth + j) |= 1;
	  *(bits + i * mWidth + j) <<= 1;
	}
	*(bits + i * mWidth + j) <<= 1;
      }
    }
    SDL_UnlockSurface(ship);
    plWordWidth = mWidth;
    //    00000000 00000000 00000000 00000001
  }
  switch (colour)
  {
   case REDSHIP: redShip = ship; break;
   case BLUESHIP: blueShip = ship; break;
   case GREENSHIP: greenShip = ship; break;
   case YELLOWSHIP: yellowShip = ship; break;
  }
  std::cout << "leaving loadship" << std::endl;
  return true;
}

void GfxCore::drawStatus (Uint8 lives, Sint32 shield, Sint32 fuel, Uint8 priAmmo, Uint8 secAmmo)
{
  SDL_Rect src, dest;

  int y = conf->getViewHeight ();
  src.x = conf->getColour () * 8;
  src.y = 0;
  src.w = 8;
  src.h = 10;
  // lives
  for (int i = 1; i <= lives; i++)
  {
    dest.x = 10 * i;
    dest.y = y;
    dest.w = 8;
    dest.h = 10;
    SDL_BlitSurface (smallShips, &src, screen, &dest);
  }
  // shield
  y += 3;
  if (shield > 0)
  {
    dest.x = 30 + conf->getStartLives () * 9;
    dest.y = y;
    dest.w = shield;
    dest.h = 5;
    SDL_FillRect (screen, &dest, createHiColourPixel (screen->format, 0, 255, 0));
  }
  // fuel
  if (fuel > 0)
  {
    dest.x = 50 + conf->getStartLives () * 8 + conf->getMaxShield ();
    dest.y = y;
    dest.w = fuel;
    dest.h = 5;
    SDL_FillRect (screen, &dest, createHiColourPixel (screen->format, 232, 81, 81));
  }
  // gatling ammo
  if (priAmmo > 0)
  {
    dest.x = 70 + conf->getStartLives () * 8 + conf->getMaxShield () + (Sint32)conf->getMaxFuel ();
    dest.y = y;
    dest.w = priAmmo;
    dest.h = 5;
    SDL_FillRect (screen, &dest, createHiColourPixel (screen->format, 203, 229, 6));
  }
  // rocket ammo
  for (int i = 0; i < secAmmo; i++)
  {
    dest.x = 90 + conf->getStartLives () * 8 + conf->getMaxShield () + (Sint32)conf->getMaxFuel () + conf->getPriWpnAmmo () + i * 5;
    dest.y = y;
    dest.w = 4;
    dest.h = 5;
    SDL_FillRect (screen, &dest, createHiColourPixel (screen->format, 229, 121, 6));
  }
}

// This function is originally written by John R. Hall
Uint16 GfxCore::createHiColourPixel (SDL_PixelFormat* fmt, Uint8 red, Uint8 green, Uint8 blue)
{
  Uint16 value;

  /* This series of bit shifts uses the information from the SDL_Format
     structure to correctly compose a 16-bit pixel value from 8-bit red,
     green, and blue data. */
  value = (((red >> fmt->Rloss) << fmt->Rshift) + ((green >> fmt->Gloss) << fmt->Gshift) + ((blue >> fmt->Bloss) << fmt->Bshift));

  return value;
}
// End of John R. Hall's code
