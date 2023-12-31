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

#ifndef _MESSAGES_H
#define _MESSAGES_H

#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"

struct Message
{
  char* message;
  Message* next;
};

class Messages
{
 public:
  Messages ();
  ~Messages ();
  void addOutgoing (char*);
  void addIncoming (char*, ...);
  char* getIncoming ();
  char* getOutgoing ();
 private:
  SDL_mutex* inMutex;
  SDL_mutex* outMutex;
  Message* inFirst;
  Message* inLast;
  Message* outFirst;
  Message* outLast;
};

#endif // _MESSAGES_H 
