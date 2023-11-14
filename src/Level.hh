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

#ifndef _LEVEL_H
#define _LEVEL_H

#include "Messages.hh"

struct Base
{
  Sint32 startX;
  Sint32 stopX;
  Sint32 y;
  Base* next;
};

struct LevelInfo
{
  char* name;
  char* image;
  char* cmap;
  char* version;
  char* author;
  Sint32 players;
  Base* bases;
  LevelInfo* next;
};

class Level
{
 private:
  void unload ();
  bool parseLevel (char*);
  bool loadLevel ();
  bool loaded;
  Messages* messages;
  LevelInfo* first;
  LevelInfo* current;
 public:
  Level (Messages*);
  ~Level ();
  bool isLoaded () { return loaded; }
  bool loadLevel (Sint32);
  bool loadLevel (const char*);
  void printLevels ();
  char* getLevel () { return loaded ? current->name : NULL; }
  void draw (double, double);
  int getPlayers () { return loaded ? current->players : 0; }
  Base* getBase (Uint8);
  Base* isOnBase (double, double, double, double);
  bool crash (double, double, double);
};

#endif // _LEVEL_H 
