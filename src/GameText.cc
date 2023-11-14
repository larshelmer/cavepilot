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

#include "GameText.hh"
#include "GfxCore.hh"
#include "Configuration.hh"
#include <iostream>

extern Configuration* conf;
extern GfxCore* gfx;
extern double timeScale;

GameText::GameText (Messages* mess)
{
  messages = mess;
  first = NULL;
  if (!gfx->loadSmallFont ())
  {
    std::cout << "Could not load game font!" << std::endl;
  }
}

GameText::~GameText ()
{
  for (Text* tmp = first; tmp != NULL;)
  {
    Text* tmp2 = tmp->next;
    delete[] tmp->message;
    delete tmp;
    tmp = tmp2;
  }
}

void GameText::update (bool GetText)
{
  Text* tmp;
  Text* prev = NULL;
  for (tmp = first; tmp != NULL; tmp = tmp->next)
  {
    tmp->timer++;
    if (tmp->timer >= ((double)conf->getTextTime () / timeScale))
    {
      if (tmp->next == NULL)
      {
	delete[] tmp->message;
	delete tmp;
	tmp = NULL;
	if (prev != NULL)
	  prev->next = NULL;
	else
	  first = NULL;
	break;
      }
    }
    prev = tmp;
  }
  if (GetText)
  {
    char* mess = messages->getIncoming ();
    if (mess == NULL)
      return;
    if (first == NULL)
    {
      first = new Text;
      first->message = mess;
      first->timer = 0;
      first->next = NULL;
    }
    else
    {
      tmp = new Text;
      tmp->message = mess;
      tmp->timer = 0;
      tmp->next = first;
      first = tmp;
    }
  }
}

void GameText::draw ()
{
  Sint32 i = 0;
  for (Text* tmp = first; tmp != NULL; tmp = tmp->next)
  {
    gfx->drawGameText (20, 42 + i++ * SMALL_LETTER_HEIGHT, tmp->message);
  }
}
