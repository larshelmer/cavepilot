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

#ifndef _GAMETEXT_H
#define _GAMETEXT_H

#include "Messages.hh"

struct Text
{
  char* message;
  Sint32 timer;
  Text* next;
};

class GameText
{
  Text* first;
  Messages* messages;
 public:
  GameText (Messages*);
  ~GameText ();
  void update (bool getText);
  void draw ();
};

#endif // _GAMETEXT_H 
