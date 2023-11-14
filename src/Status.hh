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

#ifndef _STATUS_H
#define _STATUS_H

#include "GfxCore.hh"
#include "Player.hh"

class Status
{
  Uint8 lives;
  Sint32 shield;
  Sint32 fuel;
  Uint8 priAmmo;
  Uint8 secAmmo;
 public:
  Status ();
  ~Status ();
  void update ();
  void draw ();
};

#endif // _STATUS_H 
