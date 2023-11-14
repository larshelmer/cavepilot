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
#include "Player.hh"
#include "Weapons.hh"
#include "Particles.hh"
#include <math.h>
#include <string.h>

#include <iostream>

#define MAX_PLAYERS 16
#define NO_WEAPON 42

extern GfxCore* gfx;
extern Configuration* conf;
extern Level* lvl;
extern double timeScale;
extern Weapons* wpns;
extern NetPlayer** players;
extern Particles* parts;

Player::Player (Uint8 i, Uint8 c)
{
  id = i;
  colour = c;
  inGame = false;
  gfx->loadShip (c);
  startX = 0;
  startY = 0;
  lwAng = 3.8282052;
  rwAng = 2.4549801;
  priWpn = PRI_WPN_GATLING;
  secWpn = SEC_WPN_ROCKET;
  priWpnRate = wpns->getFireRate (priWpn);
  secWpnRate = wpns->getFireRate (secWpn);
  lives = conf->getStartLives ();
  respawn ();
}

Player::~Player ()
{

}

bool Player::isHit (double x, double y, Uint8 t, Uint8 p)
{
  double dist;
  if (respawnTimer >= 0 || !inGame || isOut ())
    return false;
  if ((dist = sqrt ((x - worldX) * (x - worldX) + (y - worldY) * (y - worldY))) < RADIUS) // quick rejection
  {
    if (gfx->isHit (worldX, worldY, facing, x, y))
    {
      std::cout << "you're hit!" << std::endl;
      switch (t)
      {
       case PRI_WPN_GATLING:
       case PRI_WPN_RANDOM:
       case PRI_WPN_TWIN:
       case PRI_WPN_TRIAD:
	 shield -= 3;
	 break;
       case SEC_WPN_ROCKET:
       case SEC_WPN_MINE:
       case SEC_WPN_HOMING:
	  if (dist <= 5)
	    shield = 0;
	  else
	    shield -= (Sint32)(conf->getMaxShield () * (1 - (0.05 * (dist - 5))));
	 break;
       case SEC_WPN_BALLOON:
	 shield -= 5;
	 break;
      }
      if (shield <= 0)
      {
	killer = p;
	killerWpn = t;
	killPlayer ();
      }
      return true;
    }
  }
  return false;
}

void Player::joinGame ()
{
  homeBase = lvl->getBase (id);
  if (homeBase == NULL)
    return;
  startX = homeBase->startX + ((homeBase->stopX - homeBase->startX) / 2);
  startY = homeBase->y - RADIUS;
  inGame = true;
  worldX = startX;
  worldY = startY;
}

void Player::respawn ()
{
  respawnTimer = -1;
  onBase = true;
  onHomeBase = true;
  direction = facing = (3 * M_PI) / 2;
  // INSERT CODE -- use id to get start base from level and set coords accordingly
  //  worldY = conf->getScreenHeight () / 2;
  //  worldX = 50 + 50 * id;
  worldX = startX;
  worldY = startY;
  shield = conf->getMaxShield ();
  fuel = conf->getMaxFuel ();
  velocity = 0;
  change = true;
  killer = 0;
  killerWpn = 0;
  priWpnTimer = -1;
  secWpnTimer = -1;
  priWpnAmmo = conf->getPriWpnAmmo (); // temp
  secWpnAmmo = conf->getSecWpnAmmo (); // temp
  firePrimary = false;
  fireSecondary = false;
  priReloadTimer = -1;
  secReloadTimer = -1;
  repairTimer = -1;
  refuelTimer = -1;
}

void Player::update (double turn, double accel, Uint8 primary, Uint8 secondary)
{
  if (!inGame || isOut ())
  {
    change = false;
    return;
  }
  if (respawnTimer >= 0)
  {
    respawnTimer++;
    if (respawnTimer >= ((double)conf->getRespawnTime () / timeScale))
      respawn ();
    return;
  }
  if (shield <= 0)
  {
    killPlayer ();
    return;
  }
  if (fuel <= 0)
    accel = 0;
  else
    accel *= timeScale;
  if (onBase && accel == 0)
  {
    change = false;
    if (onHomeBase)
      regain ();
  }
  else
  {
    onBase = false;
    change = true;
    double x = worldX;
    double y = worldY;
    double dx, dy;
    // movement!
    facing += turn * timeScale;
    if (facing < 0)
      facing += 2 * M_PI;
    else if (facing >= 2 * M_PI)
      facing -= 2 * M_PI;
    if (velocity > 0)
    {
      velocity -= velocity * conf->getAirResistance () * timeScale;
      x += velocity * cos (direction);
      y -= velocity * -sin (direction);
    }
    if (accel > 0)
    {
      x += accel * cos (facing);
      y -= accel * -sin (facing);
    }
    y += conf->getGravity () * 100 * timeScale; // temp value for weight!
    dx = x - worldX;
    dy = y - worldY;
    direction = atan2 (dy, dx);
    if (direction < 0)
      direction += 2 * M_PI;
    else if (direction >= 2 * M_PI)
     direction -= 2 * M_PI;
    velocity = sqrt (dx * dx + dy * dy);
    
    //    if (velocity > conf->MAX_SPEED ())
    //      velocity = conf->MAX_SPEED ();
    //    worldX += velocity * cos (direction);
    //    worldY -= velocity * -sin (direction);
    double oldY = worldY;
    worldX = x;
    worldY = y;

    if (worldX < 0)
      worldX = 0;
    else if (worldX > conf->getWorldWidth ())
      worldX = conf->getWorldWidth () - 1;
    if (worldY < 0)
      worldY = 0;
    else if (worldY > conf->getWorldHeight ())
      worldY = conf->getWorldHeight () - 1;

    // decrease fuel
    if (accel > 0)
      fuel -= timeScale / 30; // hardcoded :/

    if ((facing > (((3 * M_PI) / 2) - 0.2)) && (facing < (((3 * M_PI) / 2) + 0.2)))
    {
      Base* b;
      double lwX = worldX + RADIUS * cos (-(facing + lwAng));
      double rwX = worldX + RADIUS * cos (-(facing + rwAng));
      if ((b = lvl->isOnBase (lwX, rwX, oldY + RADIUS - 1, worldY + RADIUS - 1)) != NULL)
      {
	worldY = b->y - RADIUS;
	velocity = 0;
	facing = direction = (3 * M_PI) / 2;
	onBase = true;
	if (b->startX == homeBase->startX && b->y == homeBase->y)
	{
	  onHomeBase = true;
	  priReloadTimer = 0;
	  secReloadTimer = 0;
	  repairTimer = 0;
	  refuelTimer = 0;
	}
	else
	  onHomeBase = false;
      }
    }
    if (!onBase)
    {
      if (lvl->crash (worldX, worldY, facing))
      {
	killer = 0;
	killerWpn = LEVEL_CRASH;
	killPlayer ();
      }
    }
  }
  fire (primary, secondary);
  for (Sint32 i = 0; i < MAX_PLAYERS; i++)
  {
    if (players[i] != NULL)
    {
      if (players[i]->collides (worldX, worldY, facing))
      {
	killer = i;
	killerWpn = PLAYER_CRASH;
	killPlayer ();
	break;
      }
    }
  }
}

void Player::draw (double cameraX, double cameraY)
{
  if (respawnTimer >= 0 || !inGame || isOut ())
    return;

  double screenX, screenY;
  screenX = (Sint32)worldX - cameraX;
  screenY = (Sint32)worldY - cameraY;
  gfx->drawShip (screenX, screenY, facing, colour);
}

void Player::fire (Uint8 primary, Uint8 secondary)
{
  if (priWpnTimer >= 0)
  {
    priWpnTimer++;
    if (priWpnTimer >= ((double)priWpnRate / timeScale))
      priWpnTimer = -1;
  }
  if (secWpnTimer >= 0)
  {
    secWpnTimer++;
    if (secWpnTimer >= ((double)secWpnRate / timeScale))
      secWpnTimer = -1;
  }
  if (primary > 0 && priWpnAmmo > 0 && priWpnTimer == -1)
  {
    change = true;
    firePrimary = true;
    priWpnTimer = 0;
    priWpnAmmo--;
    //    turretX = worldX + (RADIUS + 2) * cos (-facing);
    //    turretY = worldY + (RADIUS + 2) * -sin (-facing);
    wpns->addShot (worldX, worldY, facing, direction, velocity, priWpn, id);
  }
  if (secondary > 0 && secWpnAmmo > 0 && secWpnTimer == -1)
  {
    change = true;
    fireSecondary = true;
    secWpnTimer = 0;
    secWpnAmmo--;
    //    turretX = worldX + (RADIUS + 2) * cos (-facing);
    //    turretY = worldY + (RADIUS + 2) * -sin (-facing);
    wpns->addShot (worldX, worldY, facing, direction, velocity, secWpn, id);
  }
}

void Player::regain ()
{
  if (priReloadTimer >= 0)
  {
    priReloadTimer++;
    if (priReloadTimer >= ((double)wpns->getReloadRate (priWpn) / timeScale))
      priReloadTimer = -1;
  }
  if (priReloadTimer == -1 && priWpnAmmo < conf->getPriWpnAmmo ())
  {
    priReloadTimer = 0;
    priWpnAmmo++;
  }
  if (secReloadTimer >= 0)
  {
    secReloadTimer++;
    if (secReloadTimer >= ((double)wpns->getReloadRate (secWpn) / timeScale))
      secReloadTimer = -1;
  }
  if (secReloadTimer == -1 && secWpnAmmo < conf->getSecWpnAmmo ())
  {
    secReloadTimer = 0;
    secWpnAmmo++;
  }
  if (repairTimer >= 0)
  {
    repairTimer++;
    if (repairTimer >= ((double)conf->getRepairTime () / timeScale))
      repairTimer = -1;
  }
  if (repairTimer == -1 && shield < conf->getMaxShield ())
  {
    repairTimer = 0;
    shield++;
  }
  if (refuelTimer >= 0)
  {
    refuelTimer++;
    if (refuelTimer >= ((double)conf->getRefuelTime () / timeScale))
      refuelTimer = -1;
  }
  if (refuelTimer == -1 && fuel < conf->getMaxFuel ())
  {
    refuelTimer = 0;
    fuel++;
  }  
}

void Player::killPlayer ()
{
  std::cout << "crash!" << std::endl;
  respawnTimer = 0;
  change = true;
  lives--;
  parts->createExplosion((Sint32)worldX, (Sint32)worldY, 200, 200, 200, 15, 150);
  parts->createExplosion((Sint32)worldX, (Sint32)worldY, 255, 0, 0, 15, 150);
  parts->createExplosion((Sint32)worldX, (Sint32)worldY, 255, 255, 0, 15, 150);
}

// if we're hit by another player and haven't noticed yet...
// set change to false because everyone knows already
void Player::collision ()
{
  if (respawnTimer == -1 && inGame && !isOut ())
    killPlayer ();
}

void Player::setColour (Uint8 c)
{
  colour = c;
  gfx->loadShip (colour);
}

NetPlayer::NetPlayer (Uint8 i, Uint8 c, char* n) : Player (i, c)
{
  nick = new char[strlen (n) + 1];
  strcpy (nick, n);
  updateMutex = SDL_CreateMutex ();
}

NetPlayer::~NetPlayer ()
{
  delete[] nick;
  SDL_DestroyMutex (updateMutex);
}

bool NetPlayer::isHit (double x, double y)
{
  bool result = false;
  SDL_LockMutex (updateMutex);
  if (respawnTimer >= 0 || !inGame)
    result = false;
  else if (sqrt ((x - worldX) * (x - worldX) + (y - worldY) * (y - worldY)) < RADIUS) // quick rejection
    result = gfx->isHit (worldX, worldY, facing, x, y);
  SDL_UnlockMutex (updateMutex);
  return result;
}

bool NetPlayer::collides (double x, double y, double f)
{
  bool result = false;
  SDL_LockMutex (updateMutex);
  if (respawnTimer >= 0 || !inGame)
    result = false;
  else if (sqrt ((x - worldX) * (x - worldX) + (y - worldY) * (y - worldY)) < 2 * RADIUS) // quick rejection
    result = gfx->collides (worldX, worldY, facing, x, y, f);
  SDL_UnlockMutex (updateMutex);
  return result;
}

void NetPlayer::update (double x, double y, double vel, double face, double dir, Uint8 fire)
{
  SDL_LockMutex (updateMutex);
  respawnTimer = -1;
  worldX = x;
  worldY = y;
  velocity = vel;
  facing = face;
  direction = dir;
  // buggy -- cannot fire more than 1 wpn / frame :/
  if (fire != NO_WEAPON)
    wpns->addShot (worldX, worldY, facing, direction, velocity, fire, id);
  SDL_UnlockMutex (updateMutex);
}

void NetPlayer::killPlayer (double x, double y, Uint8 w, Uint8 p, bool isOut)
{
  SDL_LockMutex (updateMutex);
  if (respawnTimer == -1 && inGame)
  {
    parts->createExplosion((Sint32)x, (Sint32)y, 200, 200, 200, 15, 150);
    parts->createExplosion((Sint32)x, (Sint32)y, 255, 0, 0, 15, 150);
    parts->createExplosion((Sint32)x, (Sint32)y, 255, 255, 0, 15, 150);

    w += 0;
    p += 0;
  
    respawnTimer = 0;
    if (isOut)
      lives = 0;
  }
  SDL_UnlockMutex (updateMutex);
}

void NetPlayer::draw (double cameraX, double cameraY)
{
  SDL_LockMutex (updateMutex);
  if (respawnTimer >= 0 || !inGame)
  {
   SDL_UnlockMutex (updateMutex);
   return;
  }
  Player::draw (cameraX, cameraY);
  SDL_UnlockMutex (updateMutex);
}
