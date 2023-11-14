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

#include "Weapons.hh"
#include "Player.hh"
#include "GfxCore.hh"
#include "Configuration.hh"
#include <string.h>
#include <math.h>

#include <iostream>

#define GT_WEAPONS 8 // The grand total number of different weapons (not yet, but sometime)
#define MAX_PLAYERS 16 // same as in network. lousy solution... :/

extern Player* localPlayer;
extern NetPlayer** players;
extern GfxCore* gfx;
extern Configuration* conf;
extern double timeScale;

Weapons::Weapon::Weapon (Uint8 t, char* n, double s, double w)
{
  type = t;
  name = new char[strlen (n) + 1];
  strcpy (name, n);
  speed = s;
  weight = w;
  first = NULL;
}

Weapons::Weapon::~Weapon ()
{
  std::cout << "weapon destr" << std::endl;
  delete[] name;
  while (first != NULL)
    deleteShot (NULL, first);
  std::cout << "end weapon destr" << std::endl;
}

void Weapons::Weapon::update ()
{
  Shot* tmp = first;
  Shot* prev = NULL;
  while (tmp != NULL)
  {
    tmp->x += tmp->v * cos(tmp->d) * timeScale;
    tmp->y -= tmp->v * -sin(tmp->d) * timeScale;
    tmp->y += (conf->getGravity () / 5) * weight;
    tmp->v -= tmp->v * (conf->getAirResistance () / 10) * timeScale;
    if (localPlayer->isInGame ())
    {
      if (localPlayer->isHit (tmp->x, tmp->y, type, tmp->p))
      {
	Shot* tmp2 = tmp->next;
	deleteShot (prev, tmp);
	tmp = tmp2;
	continue;
      }
    }
    bool hit = false;
    for (Sint32 i = 0; i < MAX_PLAYERS; i++)
    {
      if (players[i] != NULL)
      {
	if (players[i]->isInGame ())
	{
	  if (players[i]->isHit (tmp->x, tmp->y))
	  {
	    Shot* tmp2 = tmp->next;
	    deleteShot (prev, tmp);
	    tmp = tmp2;
	    hit = true;
	    continue;
	  }
	}
      }
    }
    if (hit) // boy is this ugly...
      continue;
    if (gfx->hitsLevel (tmp->x, tmp->y))
    {
      Shot* tmp2 = tmp->next;
      deleteShot (prev, tmp);
      tmp = tmp2;
      continue;
    }
    if (tmp->v < 0.1)
    {
      Shot* tmp2 = tmp->next;
      deleteShot (prev, tmp);
      tmp = tmp2;
      continue;
    }
    if (tmp->x > conf->getWorldWidth () || tmp->x < 0 || tmp->y > conf->getWorldHeight () || tmp->y < 0)
    {
      Shot* tmp2 = tmp->next;
      deleteShot (prev, tmp);
      tmp = tmp2;
      continue;
    }
    prev = tmp;
    tmp = tmp->next;
  }
}

void Weapons::Weapon::draw (double cameraX, double cameraY)
{
  if (first != NULL)
    gfx->drawShots (cameraX, cameraY, first, 255, 255, 255);
}

void Weapons::Weapon::addShot (double x, double y, double f, double d, double v, Uint8 p)
{
  Shot* tmp = new Shot;
  tmp->x = x + (RADIUS + 2) * cos (-f);
  tmp->y = y + (RADIUS + 2) * -sin (-f);
  double tX = tmp->x;
  double tY = tmp->y;
  tX += v * cos (d);
  tY -= v * -sin (d);
  tX += speed * cos (f);
  tY -= speed * -sin (f);
  double dX = tX - tmp->x;
  double dY = tY - tmp->y;
  tmp->d = atan2 (dY, dX);
  if (tmp->d < 0)
    tmp->d += 2 * M_PI;
  else if (tmp->d >= 2 * M_PI)
    tmp->d -= 2 * M_PI;
  tmp->v = sqrt (dX * dX + dY * dY);
  tmp->p = p;
  tmp->next = first;
  first = tmp;
}

void Weapons::Weapon::deleteShot (Shot* prev, Shot* shot)
{
  if (prev != NULL)
    prev->next = shot->next;
  else
    first = shot->next;
  delete shot;
}

void Weapons::Weapon::clearAllShots ()
{
  while (first != NULL)
    deleteShot (NULL, first);
  first = NULL;
}

Weapons::Weapons ()
{
  weapons = new Weapon*[GT_WEAPONS]; // hardcoded :/
  for (Sint32 i = 0; i < GT_WEAPONS; weapons[i++] = NULL);
  weapons[PRI_WPN_GATLING] = new Weapon (PRI_WPN_GATLING, "gatling", GATLING_SPEED, GATLING_WEIGHT);
  weapons[SEC_WPN_ROCKET] = new Weapon (SEC_WPN_ROCKET, "rocket", ROCKET_SPEED, ROCKET_WEIGHT);
  weaponsMutex = SDL_CreateMutex ();
}

Weapons::~Weapons ()
{
  for (Sint32 i = 0; i < GT_WEAPONS; i++)
  {
    if (weapons[i] != NULL)
      delete weapons[i];
  }
  delete[] weapons;
  SDL_DestroyMutex (weaponsMutex);
}

void Weapons::clearAllShots ()
{
  SDL_LockMutex (weaponsMutex);
  for (Sint32 i = 0; i < GT_WEAPONS; i++)
  {
    if (weapons[i] != NULL)
      weapons[i]->clearAllShots ();
  }
  SDL_UnlockMutex (weaponsMutex);
}

void Weapons::update ()
{
  SDL_LockMutex (weaponsMutex);
  for (Sint32 i = 0; i < GT_WEAPONS; i++)
  {
    if (weapons[i] != NULL)
      weapons[i]->update ();
  }
  SDL_UnlockMutex (weaponsMutex);
}

void Weapons::draw (double cameraX, double cameraY)
{
  SDL_LockMutex (weaponsMutex);
  for (Sint32 i = 0; i < GT_WEAPONS; i++)
  {
    if (weapons[i] != NULL)
      weapons[i]->draw (cameraX, cameraY);
  }
  SDL_UnlockMutex (weaponsMutex);
}

void Weapons::addShot (double x, double y, double f, double d, double v, Uint8 t, Uint8 p)
{
  SDL_LockMutex (weaponsMutex);
  weapons[t]->addShot (x, y, f, d, v, p);
  SDL_UnlockMutex (weaponsMutex);
}

Sint32 Weapons::getFireRate (Uint8 w)
{
  switch (w)
  {
   case PRI_WPN_GATLING: return GATLING_RATE;
   case PRI_WPN_RANDOM: return RANDOM_RATE;
   case PRI_WPN_TWIN: return TWIN_RATE;
   case PRI_WPN_TRIAD: return TRIAD_RATE;
   case SEC_WPN_ROCKET: return ROCKET_RATE;
   case SEC_WPN_MINE: return MINE_RATE;
   case SEC_WPN_HOMING: return HOMING_RATE;
   case SEC_WPN_BALLOON: return BALLOON_RATE;
  }
  return 100; // hehe
}

Sint32 Weapons::getReloadRate (Uint8 w)
{
  switch (w)
  {
   case PRI_WPN_GATLING:
   case PRI_WPN_RANDOM:
   case PRI_WPN_TWIN:
   case PRI_WPN_TRIAD:
     return 3;
     break;
   case SEC_WPN_ROCKET:
   case SEC_WPN_MINE:
   case SEC_WPN_HOMING:
   case SEC_WPN_BALLOON:
     return 30;
     break;
  }
  return 100; // hehe
}
