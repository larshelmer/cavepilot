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

#include "Messages.hh"
#include <cstring>
#include <stdarg.h>

#include <iostream>

char* Messages::getIncoming ()
{
  if (inFirst != NULL)
  {
    char* tmp;
    Message* tmsg;
    SDL_LockMutex (inMutex);
    tmsg = inFirst;
    inFirst = inFirst->next;
    SDL_UnlockMutex (inMutex);
    tmp = tmsg->message;
    delete tmsg;
    return tmp;
  }
  else
    return NULL;
}

char* Messages::getOutgoing ()
{
  if (outFirst != NULL)
  {
    char* tmp;
    Message* tmsg;
    SDL_LockMutex (outMutex);
    tmsg = outFirst;
    outFirst = outFirst->next;
    SDL_UnlockMutex (outMutex);
    tmp = tmsg->message;
    delete tmsg;
    return tmp;
  }
  else
    return NULL;
}

void Messages::addIncoming (char* mess, ...)
{
  char buffer[512];
  va_list ap;
  va_start (ap, mess);
  vsprintf (buffer, mess, ap);
  strcat (buffer, "\0");
  va_end (ap);
  char* tmp;
  tmp = new char[strlen (buffer) + 1];
  strcpy (tmp, buffer);
  //  std::cout << "Messages got: " << tmp << std::endl;
  Message* tmsg;
  tmsg = new Message;
  tmsg->message = tmp;
  tmsg->next = NULL;
  SDL_LockMutex (inMutex);
  if (inFirst == NULL)
  {
    inFirst = tmsg;
    inLast = tmsg;
  }
  else
  {
    inLast->next = tmsg;
    inLast = tmsg;
  }
  SDL_UnlockMutex (inMutex);
  //  delete[] mess;
}

void Messages::addOutgoing (char* mess)
{
  char* tmp;
  tmp = new char[strlen (mess) + 1];
  strcpy (tmp, mess);
  Message* tmsg;
  tmsg = new Message;
  tmsg->message = tmp;
  tmsg->next = NULL;
  SDL_LockMutex (outMutex);
  if (outFirst == NULL)
  {
    outFirst = tmsg;
    outLast = tmsg;
  }
  else
  {
    outLast->next = tmsg;
    outLast = tmsg;
  }
  SDL_UnlockMutex (outMutex);
  //  delete[] mess;
}

Messages::Messages ()
{
  inMutex = SDL_CreateMutex ();
  inFirst = NULL;
  inLast = NULL;
  outMutex = SDL_CreateMutex ();
  outFirst = NULL;
  outLast = NULL;
}

Messages::~Messages ()
{
  if (inFirst != NULL)
  {
    Message* tmp;
    SDL_LockMutex (inMutex);
    while (inFirst != NULL)
    {
      tmp = inFirst;
      inFirst = inFirst->next;
      if (tmp->message != NULL)
	delete[] tmp->message;
      delete tmp;
    }
    inFirst = NULL;
    inLast = NULL;
    SDL_UnlockMutex (inMutex);
  }
  SDL_DestroyMutex (inMutex);
  if (outFirst != NULL)
  {
    Message* tmp;
    SDL_LockMutex (outMutex);
    while (outFirst != NULL)
    {
      tmp = outFirst;
      outFirst = outFirst->next;
      if (tmp->message != NULL)
	delete[] tmp->message;
      delete tmp;
    }
    outFirst = NULL;
    outLast = NULL;
    SDL_UnlockMutex (outMutex);
  }
  SDL_DestroyMutex (outMutex);
}
