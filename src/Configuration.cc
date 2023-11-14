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

#include "Configuration.hh"
#include "Cavepilot.hh"
#include <cstdlib>
#include <iostream>

Configuration::Configuration ()
{
  // INSERT CODE -- load configurationfile
  setDefaults ();
#ifdef _WINDOWS
  home = new char[1];
  *home = 0;
#else
  home = getenv ("HOME"); // e.g., /home/lasso
#endif
}

Configuration::~Configuration ()
{
  delete[] nick;
  //  delete[] level;
#ifdef _WINDOWS
  delete[] home;
#endif
}

void Configuration::setDefaults ()
{
  fullScreen = false;
  screenHeight = 480;
  screenWidth = 640;
  statusHeight = 10;
  //  fullScreen = true;
  //  screenHeight = 600;
  //  screenWidth = 800;
  nick = new char[strlen ("lasso") + 1];
  strcpy (nick, "lasso");
  port = 42013;
  colour = REDSHIP;
  respawnTime = 60;
  maxShield = 40;
  maxFuel = 60;
  airResistance = 0.05;
  gravity = 0.0008;
  thrust = 0.50;
  turn = 0.1324;
  thrustKey = SDLK_SPACE;
  leftKey = SDLK_LEFT;
  rightKey = SDLK_RIGHT;
  primaryKey = SDLK_UP;
  secondaryKey = SDLK_DOWN;
  //  level = new char[strlen ("share/levels/masterpiece") + 1];
  //  strcpy (level, "share/levels/masterpiece");
  startLives = 10;
  priWpnAmmo = 50;
  secWpnAmmo = 10;
  priReloadTime = 3;
  secReloadTime = 30;
  repairTime = 8;
  refuelTime = 1;
  textTime = 150;
}
