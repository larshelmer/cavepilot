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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "Level.hh"

#define RADIUS 11

class Player
{
 private:
  void respawn ();
  void regain ();
  void fire (Uint8, Uint8);
  Uint8 priWpn;
  Uint8 secWpn;
  Sint32 priWpnRate;
  Sint32 secWpnRate;
  Sint32 priWpnTimer;
  Sint32 secWpnTimer;
  Uint8 priWpnAmmo;
  Uint8 secWpnAmmo;
  bool firePrimary;
  bool fireSecondary;
  bool onBase;
  bool change;
  bool onHomeBase;
  Sint32 priReloadTimer;
  Sint32 secReloadTimer;
  Sint32 repairTimer;
  Sint32 refuelTimer;
 protected:
  void killPlayer ();
  bool inGame;
  Uint8 id;
  Uint8 colour;
  double worldX, worldY;
  double startX, startY;
  double direction, facing;
  double velocity;
  double fuel;
  double lwAng, rwAng;
  Sint32 respawnTimer;
  Sint32 shield;
  Uint8 killer;
  Uint8 killerWpn;
  Base* homeBase;
  Uint8 lives;
 public:
  Player (Uint8, Uint8);
  virtual ~Player ();
  void update (double, double, Uint8, Uint8);
  virtual void draw (double, double);
  bool isInGame () { return inGame; }
  void joinGame ();
  void partGame () { inGame = false; }
  double getWorldX () { return worldX; }
  double getWorldY () { return worldY; }
  double getVelocity () { return velocity; }
  double getFacing () { return facing; }
  double getDirection () { return direction; }
  bool hasMoved () { return change; }
  bool isDead () { if (respawnTimer >= 0) { change = false; return true; } return false; }
  Uint8 getKiller () { return killer; }
  Uint8 getKillerWeapon () { return killerWpn; }
  bool firedPrimary () { if (firePrimary) { firePrimary = false; return true; } return false; }
  bool firedSecondary () { if (fireSecondary) { fireSecondary = false; return true; } return false; }
  bool isHit (double, double, Uint8, Uint8);
  Uint8 getPrimaryWpn () { return priWpn; }
  Uint8 getSecondaryWpn () { return secWpn; }
  void collision ();
  bool isOut () { return lives <= 0 ? true : false; }
  Uint8 getLives () { return lives; }
  Sint32 getShield () { return shield; }
  Sint32 getFuel () { return Sint32 (fuel); }
  Uint8 getPriAmmo () { return priWpnAmmo; }
  Uint8 getSecAmmo () { return secWpnAmmo; }
  void setColour (Uint8);
};

class NetPlayer : public Player
{
 private:
  char* nick;
  SDL_mutex* updateMutex;
 public:
  NetPlayer (Uint8, Uint8, char*);
  ~NetPlayer ();
  char* getNick () { return nick; };
  void update (double, double, double, double, double, Uint8);
  void killPlayer (double, double, Uint8, Uint8, bool);
  void draw (double, double);
  bool isHit (double x, double y);
  bool collides (double, double, double);
};

#endif // _PLAYER_H 
