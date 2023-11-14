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

#ifndef _WEAPONS_H
#define _WEAPONS_H

#include "SDL/SDL.h"

#define PRI_WPN_GATLING 0
#define PRI_WPN_RANDOM 1
#define PRI_WPN_TWIN 2
#define PRI_WPN_TRIAD 3

#define GATLING_RATE 5
#define RANDOM_RATE 5
#define TWIN_RATE 10
#define TRIAD_RATE 15

#define GATLING_SPEED 4.5F
#define RANDOM_SPEED 4.5F
#define TWIN_SPEED 4.5F
#define TRIAD_SPEED 4.5F

#define GATLING_WEIGHT 0.4F
#define RANDOM_WEIGHT 0.4F
#define TWIN_WEIGHT 0.4F
#define TRIAD_WEIGHT 0.4F

#define SEC_WPN_ROCKET 4
#define SEC_WPN_MINE 5
#define SEC_WPN_HOMING 6
#define SEC_WPN_BALLOON 7

#define ROCKET_RATE 30
#define MINE_RATE 30
#define HOMING_RATE 30
#define BALLOON_RATE 30

#define ROCKET_SPEED 4.0F
#define MINE_SPEED 4.0F
#define HOMING_SPEED 4.0F
#define BALLOON_SPEED 4.0F

#define ROCKET_WEIGHT 1.0F
#define MINE_WEIGHT 1.0F
#define HOMING_WEIGHT 1.0F
#define BALLOON_WEIGHT 1.0F

#define PLAYER_CRASH 254 // "weapon"
#define LEVEL_CRASH 255 // "weapon"

struct Shot
{
  double x, y, v, d;
  Uint8 p;
  Shot* next;
};

class Weapons
{
  class Weapon
  {
   protected:
    Uint8 type;
    char* name;
    double speed;
    double weight;
    Shot* first;
    void deleteShot (Shot*, Shot*);
   public:
    Weapon (Uint8, char*, double, double);
    virtual ~Weapon ();
    virtual void update ();
    void draw (double, double);
    virtual void addShot (double, double, double, double, double, Uint8);
    char* getName () { return name; };
    void clearAllShots ();
  };
  Weapon** weapons;
  SDL_mutex* weaponsMutex;
 public:
  Weapons ();
  ~Weapons ();
  void update ();
  void draw (double, double);
  void addShot (double, double, double, double, double, Uint8, Uint8);
  char* getWeaponName (Uint8 w) { return weapons[w]->getName (); };
  Sint32 getFireRate (Uint8);
  Sint32 getReloadRate (Uint8);
  void clearAllShots ();
};

#endif // _WEAPONS_H
