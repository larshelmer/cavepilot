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

#include "Cavepilot.hh"
#include "GfxCore.hh"
#include "Messages.hh"
#include "Network.hh"
#include "NetworkServer.hh"
#include "NetworkClient.hh"
#include "Console.hh"
#include "Configuration.hh"
#include "Player.hh"
#include "Level.hh"
#include "Weapons.hh"
#include "Status.hh"
#include "GameText.hh"
#include <ctime>
#include <iostream>

// global objects
GfxCore* gfx;
Configuration* conf;
Network* net;
Player* localPlayer;
Level* lvl;
NetPlayer** players;
SDL_mutex* updateMutex;
Weapons* wpns;
Particles* parts;

// global variables
double timeScale;
double cameraX;
double cameraY;

void rockOn ()
{
  Sint32 prevTicks = 0, curTicks = 0;
  Sint32 startTime, endTime;
  Sint32 framesDrawn = 0;
  startTime = time (NULL);

  double turn = 0;
  double thrust = 0;
  Uint8 primary = 0;
  Uint8 secondary = 0;

  Uint8 state = STATE_CONSOLE;

  Messages* messages = new Messages ();
  net = new Network (messages);
  Console* console = new Console (messages);
  lvl = new Level (messages);
  players = new NetPlayer*[MAX_PLAYERS];
  for (int i = 0; i < MAX_PLAYERS; players[i++] = NULL);
  updateMutex = SDL_CreateMutex ();
  wpns = new Weapons ();
  Status* status = new Status ();
  GameText* text = new GameText (messages);
  parts = new Particles ();

  SDL_Event event;

  SDL_EnableUNICODE (1);

  while (state != STATE_QUIT)
  {
    prevTicks = curTicks;
    curTicks = SDL_GetTicks ();
    timeScale = (double)(curTicks - prevTicks) / 30.0;
    gfx->clearScreen ();
    SDL_PumpEvents ();
    SDL_PollEvent (&event);
    if (state == STATE_CONSOLE)
    {
      turn = 0;
      primary = 0;
      secondary = 0;
      thrust = 0;
      if (event.type == SDL_KEYDOWN)
	state = console->update (&event.key);
      else
	state = console->update (NULL);
      console->draw ();
    }
    else if (state == STATE_GAME)
    {
      switch (event.type)
      {
       case SDL_KEYDOWN:
	 switch (event.key.keysym.sym)
	 {
	  case SDLK_LEFT: turn = -conf->getTurn (); break;
	  case SDLK_RIGHT: turn = conf->getTurn (); break;
	  case SDLK_UP: primary = 1; break;
	  case SDLK_DOWN: secondary = 1; break;
	  case SDLK_SPACE: thrust = conf->getThrust (); break;
	  case SDLK_ESCAPE: state = STATE_CONSOLE; break;
	  default:
	    break;
	 }
	 break;
       case SDL_KEYUP:
	 switch (event.key.keysym.sym)
	 {
	  case SDLK_LEFT: if (turn < 0) turn = 0; break;
	  case SDLK_RIGHT: if (turn > 0) turn = 0; break;
	  case SDLK_UP: primary = 0; break;
	  case SDLK_DOWN: secondary = 0; break;
	  case SDLK_SPACE: thrust = 0; break;
	  default:
	    break;
	 }
	 break;
       default:
	 break;
      }
    }
    if (net->isConnected () && net->isGameStarted ())
    {
      SDL_LockMutex (updateMutex);
      localPlayer->update (turn, thrust, primary, secondary);
      SDL_UnlockMutex (updateMutex);
      wpns->update ();
      status->update ();
      text->update (state == STATE_GAME);
      parts->update ();
      if (state == STATE_GAME)
      {
	cameraX = localPlayer->getWorldX () - conf->getViewWidth () / 2;
	cameraY = localPlayer->getWorldY () - conf->getViewHeight () / 2;
	if (cameraX < 0)
	  cameraX = 0;
	if (cameraX >= conf->getWorldWidth () - conf->getViewWidth ())
	  cameraX = conf->getWorldWidth () - conf->getViewWidth () - 1;
	if (cameraY < 0)
	  cameraY = 0;
	if (cameraY >= conf->getWorldHeight () - conf->getViewHeight ())
	  cameraY = conf->getWorldHeight () - conf->getViewHeight () - 1;
	
	lvl->draw (cameraX, cameraY);
	localPlayer->draw (cameraX, cameraY);
	for (Sint32 i = 0; i < MAX_PLAYERS; i++)
	{
	  if (players[i] != NULL)
	  {
	    players[i]->draw (cameraX, cameraY);
	  }
	}
	wpns->draw (cameraX, cameraY);
	status->draw ();
	text->draw ();
	parts->draw (cameraX, cameraY);
      }
    }
    gfx->flipScreen ();
    if (SDL_QuitRequested () > 0)
      state = STATE_QUIT;
    framesDrawn++;
  }
  endTime = time (NULL);
  if (startTime == endTime)
    endTime++;
  std::cout << "Drew " << framesDrawn << " frames in " << endTime - startTime << " seconds, for a framerate of " << (double)framesDrawn / (double)(endTime - startTime) << " fps." << std::endl;
  std::cout << "deleting net" << std::endl;
  if (net != NULL)
    delete net;
  std::cout << "deleting messages" << std::endl;
  delete messages;
  std::cout << "deleting console" << std::endl;
  delete console;
  std::cout << "deleting players" << std::endl;
  for (Sint32 i = 0; i < MAX_PLAYERS; i++)
    if (players[i] != NULL)
      delete players[i];
  delete[] players;
  std::cout << "deleting player" << std::endl;
  if (localPlayer != NULL)
    delete localPlayer;
  std::cout << "deleting mutex" << std::endl;
  SDL_DestroyMutex (updateMutex);
  std::cout << "deleting level" << std::endl;
  delete lvl;
  std::cout << "deleting weapons" << std::endl;
  delete wpns;
  std::cout << "deleting status" << std::endl;
  delete status;
  std::cout << "deleting gametext" << std::endl;
  delete text;
  std::cout << "deleting particles" << std::endl;
  delete parts;
}

int main (int argc, char* argv[])
{
  if (argc >= 2)
  {
    
    if ((strcmp (argv[1], "--version") == 0) || (strcmp (argv[1], "-v") == 0))
    {
      //      std::cout << "Cavepilot version " << _VERSION << std::endl;
      std::cout << "Cavepilot version 0.5.0" << std::endl;
      std::cout << "by Lars Helmer <lasso@spacecentre.se>" << std::endl;
      std::cout << "The latest version can be found at http://cavepilot.spacecentre.se" << std::endl;
    }
    else
      std::cout << argv[0] << ": invalid option: " << argv[1] << std::endl;
    return EXIT_SUCCESS;
  }

  localPlayer = NULL;

  conf = new Configuration ();

  gfx = new GfxCore ();

  if (gfx->isUp ())
    rockOn ();

  std::cout << "deleting gfx" << std::endl;
  delete gfx;

  return EXIT_SUCCESS;
}
