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

#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "Messages.hh"

#define STATE_QUIT 0
#define STATE_GAME 2
#define STATE_CONSOLE 3

class Console
{
 private:
  Uint8 parseInput ();
  void addText (char*);
  char** text;
  char* current;
  Messages* messages;
  Sint32 lines;
  Sint32 columns;
  Sint32 vertMargin;
  Sint32 horizMargin;
  Sint32 innerMargin;
  Sint32 inputWidth;
  Sint32 inputHeight;
  Sint32 messageAreaWidth;
  Sint32 messageAreaHeight;
  Sint32 currentLines;
 public:
  Console (Messages*);
  ~Console ();
  Uint8 update (SDL_KeyboardEvent*);
  void draw ();
};

#endif // _CONSOLE_H 
