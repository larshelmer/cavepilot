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

#ifndef _PARTICLES_H
#define _PARTICLES_H

struct Particle
{
  double x, y;
  double energy;
  double angle;
  double r, g, b;
  Particle* next;
};

#include "GfxCore.hh"

class Particles
{
  Sint32 seed;
  void initRandom ();
  Uint32 getRandom ();
  void deleteParticle (Particle*, Particle*);
  Particle* first;
  SDL_mutex* explosionMutex;
 public:
  Particles ();
  ~Particles ();
  void createExplosion (Sint32, Sint32, Sint32, Sint32, Sint32, Sint32, Sint32);
  void update ();
  void draw (double, double);
};

#endif // _PARTICLES_H 
