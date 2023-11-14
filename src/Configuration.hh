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

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include "SDL/SDL.h"
#include <string.h>

#define REDSHIP 1
#define BLUESHIP 2
#define GREENSHIP 3
#define YELLOWSHIP 4

class Configuration
{
 private:
  void setDefaults ();
  Sint32 screenHeight;
  Sint32 screenWidth;
  Sint32 worldHeight;
  Sint32 worldWidth;
  Sint32 statusHeight;
  bool fullScreen;
  char* nick;
  Uint16 port;
  Uint8 colour;
  Sint32 respawnTime;
  Sint32 maxShield;
  Uint8 startLives;
  Uint8 priWpnAmmo;
  Uint8 secWpnAmmo;
  Sint32 priReloadTime;
  Sint32 secReloadTime;
  Sint32 repairTime;
  Sint32 refuelTime;
  double maxFuel;
  double airResistance;
  double gravity;
  double thrust;
  double turn;
  //  char* level;
  SDLKey thrustKey;
  SDLKey leftKey;
  SDLKey rightKey;
  SDLKey primaryKey;
  SDLKey secondaryKey;
  Sint32 textTime;
  char* home;
 public:
  Configuration ();
  ~Configuration ();
  const char* getHome () { return home; }
  Sint32 getScreenHeight () { return screenHeight; }
  Sint32 getScreenWidth () { return screenWidth; }
  Sint32 getWorldHeight () { return worldHeight; }
  Sint32 getWorldWidth () { return worldWidth; }
  Sint32 getViewHeight () { return screenHeight - statusHeight; }
  Sint32 getViewWidth () { return screenWidth; }
  Sint32 getStatusHeight () { return statusHeight; }
  void setWorldHeight (Sint32 h) { worldHeight = h; }
  void setWorldWidth (Sint32 w) { worldWidth = w; }
  void setScreenHeight (Sint32 h) { screenHeight = h; }
  void setScreenWidth (Sint32 w) { screenWidth = w; }
  void setStatusHeight (Sint32 h) { statusHeight = h; }
  bool getFullScreen () { return fullScreen; }
  const char* getNick () { return nick; }
  void setNick (char* n) { delete[] nick; nick = new char[strlen (n) + 1]; strcpy (nick, n); }
  Uint16 getPort () { return port; }
  void setPort (Uint16 p) { port = p; }
  Uint8 getColour () { return colour; }
  void setColour (Uint8 c) { colour = c; }
  bool setColour (char* c);
  Sint32 getRespawnTime () { return respawnTime; }
  void setRespawnTime (Sint32 r) { if (r > 0) respawnTime = r; }
  Sint32 getMaxShield () { return maxShield; }
  void setMaxShield (Sint32 s) { if (s > 0) maxShield = s; }
  double getMaxFuel () { return maxFuel; }
  void setMaxFuel (double f) { if (f > 0) maxFuel = f; }
  double getAirResistance () { return airResistance; }
  void setAirResistance (double a) { if (a >= 0) airResistance = a; }
  double getGravity () { return gravity; }
  double getThrust () { return thrust; }
  void setThrust (double t) { if (t > 0) thrust = t; }
  double getTurn () { return turn; }
  void setTurn (double t) { if (t > 0) turn = t; }
  SDLKey getThrustKey () { return thrustKey; }
  void setThrustKey (SDLKey k) { thrustKey = k; }
  SDLKey getLeftKey () { return leftKey; }
  void setLeftKey (SDLKey k) { leftKey = k; }
  SDLKey getRightKey () { return rightKey; }
  void setRightKey (SDLKey k) { rightKey = k; }
  SDLKey getPrimaryKey () { return primaryKey; }
  void setPrimaryKey (SDLKey k) { primaryKey = k; }
  SDLKey getSecondaryKey () { return secondaryKey; }
  void setSecondaryKey (SDLKey k) { secondaryKey = k; }
  //  const char* getLevel () { return level; }
  //  void setLevel (char* l) { delete[] level; level = new char[strlen (l) + 1]; strcpy (level, l); }
  Uint8 getStartLives () { return startLives; }
  Uint8 getPriWpnAmmo () { return priWpnAmmo; }
  Uint8 getSecWpnAmmo () { return secWpnAmmo; }
  void setPriWpnAmmo (Uint8 a) { priWpnAmmo = a; }
  void setSecWpnAmmo (Uint8 a) { secWpnAmmo = a; }
  Sint32 getPriReloadTime () { return priReloadTime; }
  void setPriReloadTime (Sint32 t) { priReloadTime = t; }
  Sint32 getSecReloadTime () { return secReloadTime; }
  void setSecReloadTime (Sint32 t) { secReloadTime = t; }
  Sint32 getRepairTime () { return repairTime; }
  void setRepairTime (Sint32 t) { repairTime = t; }
  Sint32 getRefuelTime () { return refuelTime; }
  void setRefuelTime (Sint32 t) { refuelTime = t; }
  Sint32 getTextTime () { return textTime; }
  void setTextTime (Sint32 t) { textTime = t; }
};

#endif // _CONFIGURATION_H 
