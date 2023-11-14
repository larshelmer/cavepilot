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

#include "Level.hh"
#include "Configuration.hh"
#include "GfxCore.hh"
#include "Cavepilot.hh"
#include <iostream>
#include <fstream>
#include <glob.h>
#include <cstring>

#ifdef _WINDOWS
#define SUFFIX ".lvl.txt"
#undef DATADIR
#define DATADIR "share/"
#else
#define SUFFIX ".lvl"
#ifdef _DEVEL
#undef DATADIR
#define DATADIR "share/"
#endif
#endif

extern Configuration* conf;
extern GfxCore* gfx;

Level::Level (Messages* m)
{
  loaded = false;
  messages = m;
  current = NULL;
  first = NULL;
  char* pattern = new char[strlen (DATADIR) + strlen ("levels/") + strlen (SUFFIX) + 2];
  *pattern = 0;
  strcat (pattern, DATADIR);
  strcat (pattern, "levels/");
  strcat (pattern, "*");
  strcat (pattern, SUFFIX);
  std::cout << "dir: " << pattern << std::endl;
  glob_t gbuf;
  messages->addIncoming ("Looking for levels...");
  if (glob (pattern, 0, NULL, &gbuf) == 0)
  {
    Sint32 found = 0;
    for (Uint32 i = 0; i < gbuf.gl_pathc; i++)
    {
      if (parseLevel (gbuf.gl_pathv[i]))
	found++;
    }
    globfree (&gbuf);
    messages->addIncoming ("Found %d levels.", found);
    // INSERT CODE -- look for levels in homedir
  }
  else
    messages->addIncoming ("No levels found!");
  delete[] pattern;
  current = NULL;
}

Level::~Level ()
{
  LevelInfo* tmp = first;
  while (tmp != NULL)
  {
    if (tmp->name != NULL)
      delete[] tmp->name;
    if (tmp->image != NULL)
      delete[] tmp->image;
    if (tmp->cmap != NULL)
      delete[] tmp->cmap;
    if (tmp->author != NULL)
      delete[] tmp->author;
    if (tmp->version != NULL)
      delete[] tmp->version;
    Base* tbase = tmp->bases;
    while (tbase != NULL)
    {
      Base* tbase2 = tbase->next;
      delete tbase;
      tbase = tbase2;
    }
    LevelInfo* tmp2 = tmp->next;
    delete tmp;
    tmp = tmp2;
  }
}

bool Level::parseLevel (char* path)
{
  char buffer[256];
  std::ifstream* file;
  file = new std::ifstream (path);
  bool success = true;
  Sint32 baseCount = 0;
  if (file->is_open ())
  {
    if (current == NULL)
    {
      first = new LevelInfo;
      current = first;
    }
    else
    {
      current->next = new LevelInfo;
      current = current->next;
    }
    memset (current, 0, sizeof (LevelInfo));
    while (!file->eof ())
    {
      file->getline (buffer, 200);
      if (buffer[0] == '#')
	continue;
      if (strncasecmp (buffer, "name: ", 6) == 0)
      {
	current->name = new char[strlen (buffer) - 5];
	strcpy (current->name, buffer + 6);
	continue;
      }
      if (strncasecmp (buffer, "image: ", 7) == 0)
      {
	current->image = new char[strlen (buffer) - 6];
	strcpy (current->image, buffer + 7);
	continue;
      }
      if (strncasecmp (buffer, "map: ", 5) == 0)
      {
	current->cmap = new char[strlen (buffer) - 4];
	strcpy (current->cmap, buffer + 5);
	continue;
      }
      if (strncasecmp (buffer, "version: ", 9) == 0)
      {
	current->version = new char[strlen (buffer) - 8];
	strcpy (current->version, buffer + 9);
	continue;
      }
      if (strncasecmp (buffer, "author: ", 8) == 0)
      {
	current->author = new char[strlen (buffer) - 7];
	strcpy (current->author, buffer + 8);
	continue;
      }
      if (strncasecmp (buffer, "players: ", 9) == 0)
      {
	current->players = atoi (buffer + 9);	
	continue;
      }
      if (strncasecmp (buffer, "base: ", 6) == 0)
      {
	Base* base;
	if (current->bases == NULL)
	{
	  current->bases = new Base;
	  base = current->bases;
	}
	else
	{
	  for (base = current->bases; base->next != NULL; base = base->next);
	  base->next = new Base;
	  base = base->next;
	}
	base->next = NULL;
	char* tmp = buffer + 6;
	char* tok = tmp;
	tmp = strchr (tmp, ' ');
	*tmp++ = '\0';
	base->startX = atoi (tok);
	tok = tmp;
	tmp = strchr (tmp, ' ');
	*tmp++ = '\0';
	base->stopX = atoi (tok);
	tok = tmp;
	base->y = atoi (tok);
	baseCount++;
      }
    }
    file->close ();
    delete file;
  }
  else
    success = false;
  if (success)
  {
    if (current->players <= 0)
      success = false;
    else if (baseCount != current->players)
      success = false;
    else if (current->image == NULL)
      success = false;
    else if (current->cmap == NULL)
      success = false;
    if (!success)
    {
      if (current->name != NULL)
	delete[] current->name;
      if (current->image != NULL)
	delete[] current->image;
      if (current->cmap != NULL)
	delete[] current->cmap;
      if (current->author != NULL)
	delete[] current->author;
      if (current->version != NULL)
	delete[] current->version;
      for (Base* tmp = current->bases; tmp != NULL;)
      {
	Base* tmp2 = tmp->next;
	delete tmp;
	tmp = tmp2;
      }
    }
  }
  return success;
}

bool Level::crash (double x, double y, double f)
{
  if (loaded)
    return gfx->crash (x, y, f);
  return false;
}

void Level::draw (double cameraX, double cameraY)
{
  if (!loaded)
    return;
  gfx->drawLevel (cameraX, cameraY);
}

Base* Level::getBase (Uint8 id)
{
  if (loaded && id < current->players)
  {
    Base* tmp = current->bases;
    for (Sint32 i = 0; i < id; i++)
      tmp = tmp->next;
    return tmp;
  }
  return NULL;
}

Base* Level::isOnBase (double startX, double endX, double startY, double endY)
{
  Base* tmp = current->bases;
  for (int i = 0; i < current->players; i++)
  {
    if (((int)startX >= tmp->startX) && ((int)endX <= tmp->stopX))
      if (((int)startY <= tmp->y) && ((int)endY >= tmp->y))
	return tmp;
    tmp = tmp->next;
  }
  return NULL;
}

void Level::printLevels ()
{
  LevelInfo* tmp = first;
  Sint32 i = 1;
  while (tmp != NULL)
  {
    messages->addIncoming ("%d. %s", i++, tmp->name);
    tmp = tmp->next;
  }
}

bool Level::loadLevel ()
{
  char* path = new char[strlen (DATADIR) + strlen ("levels/") + strlen (current->image) + 1];
  *path = 0;
  strcat (path, DATADIR);
  strcat (path, "levels/");
  strcat (path, current->image);
  messages->addIncoming ("Loading level %s...", current->name);
  if (!gfx->loadLevelImage (path))
  {
    messages->addIncoming ("Could not load levelfile: %s", current->image);
    delete[] path;
    current = NULL;
    return false;
  }
  delete[] path;
  path = new char[strlen (DATADIR) + strlen ("levels/") + strlen (current->cmap) + 1];
  *path = 0;
  strcat (path, DATADIR);
  strcat (path, "levels/");
  strcat (path, current->cmap);
  if (!gfx->loadCollisionImage (path))
  {
    messages->addIncoming ("Could not load levelfile: %s", current->cmap);
    delete[] path;
    current = NULL;
    return false;
  }
  delete[] path;
  messages->addIncoming ("Author: %s", current->author);
  messages->addIncoming ("Version: %s", current->version);
  return true;
}

bool Level::loadLevel (Sint32 ix)
{
  loaded = false;
  if (ix <= 0 || first == NULL)
    return false;
  LevelInfo* tmp = first;
  for (Sint32 i = 1; i < ix; i++)
  {
    tmp = tmp->next;
    if (tmp == NULL)
      return false;
  }
  if (current != NULL && (strcmp (current->name, tmp->name) == 0))
    loaded = true;
  else
  {
    current = tmp;
    loaded = loadLevel ();
  }
  return loaded;
}

bool Level::loadLevel (const char* name)
{
  if (name == NULL)
    return false;
  if (loaded && (strcmp (current->name, name) == 0))
    return true;
  loaded = false;
  LevelInfo* tmp = first;
  current = NULL;
  while (current == NULL && tmp != NULL)
  {
    if (strcmp (tmp->name, name) == 0)
    {
      current = tmp;
      loaded = loadLevel ();
    }
    else
      tmp = tmp->next;
  }
  return loaded;
}
