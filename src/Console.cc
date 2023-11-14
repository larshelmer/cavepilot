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

#include "Console.hh"
#include "GfxCore.hh"
#include "Configuration.hh"
#include "Network.hh"
#include "NetworkServer.hh"
#include "NetworkClient.hh"
#include "Player.hh"
#include "Level.hh"
#include <ctype.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern GfxCore* gfx;
extern Configuration* conf;
extern Network* net;
extern Player* localPlayer;
extern Level* lvl;

Console::Console (Messages* m)
{
  messages = m;
  if (!gfx->loadConsoleFont ())
  {
    std::cout << "Could not load console font!" << std::endl;
  }
  currentLines = 0;
  vertMargin = 5;
  horizMargin = 5;
  innerMargin = 2;
  Sint32 height = conf->getScreenHeight ();
  Sint32 width = conf->getScreenWidth ();
  inputHeight = LETTER_HEIGHT + 2 * innerMargin;
  inputWidth = width - 2 * horizMargin;
  lines = (height - inputHeight - 3 * vertMargin) / LETTER_HEIGHT;
  messageAreaHeight = lines * LETTER_HEIGHT + 2 * innerMargin;
  columns = (width - 2 * vertMargin) / LETTER_WIDTH;
  messageAreaWidth = width - 2 * horizMargin;
  text = new char*[lines];
  for (Sint32 i = 0; i < lines; text[i++] = NULL);
  current = new char[columns];
  current[0] = '\0';
  addText ("WELCOME TO CAVEPILOT VERSION 0.5.0");
  addText ("TYPE /? FOR HELP");
}

Console::~Console ()
{

}

void Console::addText (char* m)
{
  if (currentLines == lines)
  {
    delete[] text[0];
    for (Sint32 i = 0; i < (lines - 1); i++)
      text[i] = text[i + 1];
    currentLines--;
  }
  text[currentLines] = new char[strlen (m) + 1];
  strcpy (text[currentLines], m);
  currentLines++;
}

Uint8 Console::parseInput ()
{
  Uint8 state = STATE_CONSOLE;
  if (currentLines == lines)
  {
    delete[] text[0];
    for (Sint32 i = 0; i < (lines - 1); i++)
      text[i] = text[i + 1];
    currentLines--;
  }
  if (current[0] == '/')
  {
    Sint32 hash = 0;
    for (Sint32 i = 0; *(current + i) != ' ' && *(current + i) != '\0'; i++)
      hash += tolower (current[i]);
    if (hash != 257)
    {
      text[currentLines++] = current;
      if (currentLines == lines)
      {
	delete[] text[0];
	for (Sint32 i = 0; i < (lines - 1); i++)
	  text[i] = text[i + 1];
	currentLines--;
      }
    }
    switch (hash)
    {
     case 257: // /me
       if (net->isConnected ())
	 messages->addOutgoing (current);
       text[currentLines] = new char[strlen (conf->getNick ()) + strlen (current) + 4];
       sprintf (text[currentLines++], "* %s %s", conf->getNick (), (current + 4));
       current[0] = '\0';
       break;
     case 500: // /port
       if (strlen (current) > 7)
       {
	 conf->setPort (atoi (strchr (current, ' ') + 1));
	 addText ("Port set");
       }
       else
	 addText ("Illegal port");
       break;
     case 710: // /server
       if (!net->isConnected ())
       {
	 if (lvl->isLoaded ())
	 {
	   delete net;
	   net = new NetworkServer (messages);
	 }
	 else
	   addText ("No level loaded!");
       }
       break;
     case 686: // /client
       if (!net->isConnected ())
       {
	 delete net;
	 net = new NetworkClient (messages, strchr (current, ' ') + 1);
       }
       break;
     case 110: // /?
       addText ("Available commands:");
       addText ("/?                This text");
       addText ("/fullscreen       Toggle fullscreen mode");
       addText ("/port [port]      Set the port to listen/connect to");
       addText ("/server           Start a server");
       addText ("/client [host]    Connect to [host]");
       addText ("/stop             Stop server/client");
       addText ("/nick [nick]      Change your nick");
       addText ("/colour [colour]  Change your ship colour");
       addText ("/colours          Display available colours");
       addText ("/controls         Change your ship controls");
       addText ("/levels           Show available levels");
       addText ("/level [level]    Set current level");
       addText ("/quit             Exit game");
       break;
     case 1122: // /fullscreen
       addText ("Not implemented");
       break;
     case 489: // /exit
     case 498: // /quit
       state = STATE_QUIT;
       break;
     case 468: // /nick
       if (strlen (current) > 7)
       {
	 conf->setNick (strchr (current, ' ') + 1);
	 addText ("Nick changed");
       }
       else
	 addText ("Illegal nick");
       break;
     case 822: // /colours
     case 705: // /colors
       addText ("1. Red");
       addText ("2. Blue");
       addText ("3. Green");
       addText ("4. Yellow");
       break;
     case 707: // /colour
     case 590: // /color
       if (!net->isConnected ())
       {
	 int c = atoi (strchr (current, ' ') + 1);
	 if (c < REDSHIP || c > YELLOWSHIP)
	   addText ("No such colour!");
	 else
	   conf->setColour ((Uint8)c);
       }
       else
	 addText ("Not allowed during gameplay!");
       break;
     case 501: // /stop
       delete net;
       net = new Network (messages);
       break;
     case 698: // /levels
       lvl->printLevels ();
       break;
     case 583: // /level
       if (!lvl->loadLevel (atoi (strchr (current, ' ') + 1)))
	 addText ("Loading level failed!");
       break;
     case 1046: // /controls
       addText ("Not implemented");
       break;
     default:
       addText ("Unknown command");
    }
    if (hash != 257)
    {
      current = new char[columns];
      current[0] = '\0';
    }
  }
  else
  {
    if (net->isConnected ())
      messages->addOutgoing (current);
    text[currentLines] = new char[strlen (conf->getNick ()) + strlen (current) + 4];
    sprintf (text[currentLines++], "<%s> %s", conf->getNick (), current);
    current[0] = '\0';
  }
  return state;
}

Uint8 Console::update (SDL_KeyboardEvent* key)
{
  Uint8 state = STATE_CONSOLE;
  if (key != NULL)
  {
    if (key->keysym.sym == SDLK_F1)
    {
      if (lvl->isLoaded () && net->startGame ())
      {
	localPlayer->joinGame ();
      }
    }
    else if (key->keysym.sym == SDLK_RETURN)
    {
      if (strlen (current) > 0)
	state = parseInput ();
    }
    else if (key->keysym.sym == SDLK_ESCAPE)
    {
      if (net->isConnected () && net->isGameStarted ()) // INSERT CODE -- should check if the game is up n running
	state = STATE_GAME;
    }
    else if (key->keysym.sym == SDLK_BACKSPACE)
    {
      if (strlen (current) > 0)
	current[strlen (current) - 1] = '\0';
    }
    else if (key->keysym.sym == SDLK_SPACE)
    {
      if ((Sint32)strlen (current) < columns)
      {
	strcat (current, " ");
      }
    }
    else
    {
      char* tmp = SDL_GetKeyName (key->keysym.sym);
      if ((isprint (tmp[0]) != 0 && (Sint32)strlen (current) < columns))
      {
	if (strlen (tmp) == 1)
	{
	  char* t = new char[2];
	  t[0] = (char)key->keysym.unicode;
	  t[1] = '\0';
	  strcat (current, t);
	  delete[] t;
	}
	else if (strcmp ("world 69", tmp) == 0)
	  strcat (current, "å");
	else if (strcmp ("world 68", tmp) == 0)
	  strcat (current, "ä");
	else if (strcmp ("world 86", tmp) == 0)
	  strcat (current, "ö");
      }
    }
  }
  // INSERT CODE -- check incoming messages here
  char* tmp;
  if ((tmp = messages->getIncoming ()) != NULL)
  {
    if (currentLines == lines)
    {
      delete[] text[0];
      for (Sint32 i = 0; i < (lines - 1); i++)
	text[i] = text[i + 1];
      currentLines--;
    }
    text[currentLines++] = tmp;
  }
  return state;
}

void Console::draw ()
{
  gfx->drawConsole (vertMargin, horizMargin, messageAreaHeight, messageAreaWidth, inputHeight, inputWidth);
  for (Sint32 i = 0; i < currentLines; i++)
    gfx->drawConsoleText (horizMargin + innerMargin, vertMargin + innerMargin + i * LETTER_HEIGHT, text[i]);
  strcat (current, "_");
  gfx->drawConsoleText (horizMargin + innerMargin, conf->getScreenHeight () - vertMargin - inputHeight + innerMargin, current);
  current[strlen (current) - 1] = '\0';
}
