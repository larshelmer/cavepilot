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

#include "Particles.hh"
#include "Configuration.hh"
#include <cmath>
#include <ctime>

extern double timeScale;
extern GfxCore* gfx;
extern Configuration* conf;

Particles::Particles ()
{
  initRandom ();
  explosionMutex = SDL_CreateMutex ();
  first = NULL;
}

Particles::~Particles ()
{
  SDL_LockMutex (explosionMutex);
  while (first != NULL)
    deleteParticle (NULL, first);
  SDL_UnlockMutex (explosionMutex);
  SDL_DestroyMutex (explosionMutex);
}

void Particles::initRandom ()
{
  seed = time (NULL);
}

Uint32 Particles::getRandom ()
{
  Sint32 p1 = 1103515245;
  Sint32 p2 = 12345;
  seed = (seed * p1 + p2) % 2147483647;
  return (Uint32)seed / 3;
}

void Particles::createExplosion (Sint32 x, Sint32 y, Sint32 r, Sint32 g, Sint32 b, Sint32 energy, Sint32 density)
{
  SDL_LockMutex (explosionMutex);
  /* Create a number of particles proportional to the size of the explosion. */
  for (Sint32 i = 0; i < density; i++)
  {
    Particle* part = new Particle;
    part->x = x;
    part->y = y;
    part->angle = getRandom () % 360;
    part->energy = (double)(getRandom () % (energy * 20)) / 100.0;
    
    /* Set the particle's color. */
    part->r = r;
    part->g = g;
    part->b = b;
    part->next = NULL;
    /* Add the particle to the particle system. */
    if (first == NULL)
      first = part;
    else
    {
      part->next = first;
      first = part;
    }
  }
  SDL_UnlockMutex (explosionMutex);
}

void Particles::update ()
{
  SDL_LockMutex (explosionMutex);
  Particle* part = first;
  Particle* prev = NULL;
  while (part != NULL)
  {
    part->x += part->energy * cos (part->angle * M_PI / 180) * timeScale;
    part->y += part->energy * -sin (part->angle * M_PI / 180) * timeScale;
    part->r -= timeScale;
    part->g -= timeScale;
    part->b -= timeScale;
    if (part->r < 0)
      part->r = 0;
    if (part->g < 0)
      part->g = 0;
    if (part->b < 0)
      part->b = 0;
    if ((part->r + part->g + part->b) == 0)
    {
      Particle* tmp2 = part->next;
      deleteParticle (prev, part);
      part = tmp2;
      continue;
    }
    if (gfx->hitsLevel (part->x, part->y))
    {
      Particle* tmp2 = part->next;
      deleteParticle (prev, part);
      part = tmp2;
      continue;
    }
    if (part->energy < 0.1)
    {
      Particle* tmp2 = part->next;
      deleteParticle (prev, part);
      part = tmp2;
      continue;
    }
    if (part->x > conf->getWorldWidth () || part->x < 0 || part->y > conf->getWorldHeight () || part->y < 0)
    {
      Particle* tmp2 = part->next;
      deleteParticle (prev, part);
      part = tmp2;
      continue;
    }
    prev = part;
    part = part->next;
  }
  SDL_UnlockMutex (explosionMutex);
}

void Particles::draw (double cameraX, double cameraY)
{
  if (first != NULL)
  {
    SDL_LockMutex (explosionMutex);
    gfx->drawParticles (cameraX, cameraY, first);
    SDL_UnlockMutex (explosionMutex);
  }
}

void Particles::deleteParticle (Particle* prev, Particle* part)
{
  if (prev != NULL)
    prev->next = part->next;
  else
    first = part->next;
  delete part;
}

