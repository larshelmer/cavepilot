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

#include "Network.hh"
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <iostream>


void deletePacket (NetPacket* pkt)
{
  if (pkt == NULL)
    return;
  switch (pkt->type)
  {
   case NET_TYPE_UPDATE:
   case NET_TYPE_DEATH:
   case NET_TYPE_STATUS:
     delete pkt;
     break;
   case NET_TYPE_CONNECT:
     if (((struct ConnectPacket*)pkt)->name != NULL)
       delete[] ((struct ConnectPacket*)pkt)->name;
     delete pkt;
     break;
   case NET_TYPE_PLAYER:
     if (((struct PlayerPacket*)pkt)->name != NULL)
       delete[] ((struct PlayerPacket*)pkt)->name;
     delete pkt;
     break;
   case NET_TYPE_CONF:
     if (((struct ConfPacket*)pkt)->levelName != NULL)
       delete[] ((struct ConfPacket*)pkt)->levelName;
     delete pkt;
     break;
   case NET_TYPE_CHAT:
     if (((struct ChatPacket*)pkt)->message != NULL)
       delete[] ((struct ChatPacket*)pkt)->message;
     delete pkt;
     break;
  }
}

Network::Network (Messages* m)
{
  connected = false;
  messages = m;
  inqueueMutex = SDL_CreateMutex ();
  inFirst = NULL;
  inLast = NULL;
}

Network::~Network ()
{
  connected = false;
  SDL_LockMutex (inqueueMutex);
  PacketWrapper* tmp = inFirst;
  while (tmp != NULL)
  {
    tmp = inFirst->next;
    deletePacket (inFirst->pkt);
    delete inFirst;
    inFirst = tmp;
  }
  SDL_UnlockMutex (inqueueMutex);
  SDL_DestroyMutex (inqueueMutex);
}

void Network::addPacket (NetPacket* pkt)
{
  if (pkt == NULL)
    return;
  PacketWrapper* tmp = new PacketWrapper;
  tmp->pkt = pkt;
  tmp->next = NULL;
  SDL_LockMutex (inqueueMutex);
  if (inFirst == NULL)
  {
    inFirst = tmp;
    inLast = tmp;
  }
  else
  {
    inLast->next = tmp;
    inLast = tmp;
  }
  SDL_UnlockMutex (inqueueMutex);
}

NetPacket* Network::getPacket ()
{
  if (inFirst != NULL)
  {
    PacketWrapper* tmp;
    SDL_LockMutex (inqueueMutex);
    tmp = inFirst;
    inFirst = inFirst->next;
    SDL_UnlockMutex (inqueueMutex);
    NetPacket* pkt = tmp->pkt;
    delete tmp;
    return pkt;
  }
  else
    return NULL;
}

NetComm::NetComm (Sint32 s)
{
  sock = s;
  connected = true;
}

NetComm::~NetComm ()
{
  disconnect ();
}

void NetComm::cleanUp () // try to read a large chunk of data to place us at the beginning of a packet
{
  std::cout << "entering cleanup" << std::endl;
  char* tmp = new char[200];
  Sint32 amt = 200;
  while (amt >= 199)
  {
    amt = read (sock, tmp, 200);
    if (amt <= 0 && errno != EINTR)
    {
      disconnect ();
      connected = false;
      return;
    }
  }
  delete[] tmp;
  std::cout << "leaving cleanup" << std::endl;
}

void NetComm::disconnect ()
{
  if (connected)
    close (sock);
  connected = false;
}

bool NetComm::sendData (NetPacket* pkt)
{
  if (!connected)
    return false;

  Sint32 amt, remaining;
  char* str = NULL;
  Sint32 strSize = 0;

  switch (pkt->type)
  {
   case NET_TYPE_CONNECT:
     remaining = sizeof (struct ConnectPacket) - sizeof (char*);
     if (((struct ConnectPacket*)pkt)->name != NULL)
     {
       strSize = strlen (((struct ConnectPacket*)pkt)->name) + 1;
       str = ((struct ConnectPacket*)pkt)->name;
     }
     break;
   case NET_TYPE_PLAYER:
     remaining = sizeof (struct PlayerPacket) - sizeof (char*);
     if (((struct PlayerPacket*)pkt)->name != NULL)
     {
       strSize = strlen (((struct PlayerPacket*)pkt)->name) + 1;
       str = ((struct PlayerPacket*)pkt)->name;
     }
     break;
   case NET_TYPE_UPDATE:
     remaining = sizeof (struct UpdatePacket);
     break;
   case NET_TYPE_DEATH:
     remaining = sizeof (struct DeathPacket);
     break;
   case NET_TYPE_STATUS:
     remaining = sizeof (struct StatusPacket);
     break;
   case NET_TYPE_CONF:
     remaining = sizeof (struct ConfPacket) - sizeof (char*);
     if (((struct ConfPacket*)pkt)->levelName != NULL)
     {
       strSize = strlen (((struct ConfPacket*)pkt)->levelName) + 1;
       str = ((struct ConfPacket*)pkt)->levelName;
     }
     break;
   case NET_TYPE_CHAT:
     remaining = sizeof (struct ChatPacket) - sizeof (char*);
     if (((struct ChatPacket*)pkt)->message != NULL)
     {
       strSize = strlen (((struct ChatPacket*)pkt)->message) + 1;
       str = ((struct ChatPacket*)pkt)->message;
     }
     break;
   default:
     remaining = 0;
  }
  
  Sint32 count = 0;
  while (remaining > 0)
  {
    amt = write (sock, ((char*)pkt) + count, remaining);
    if (amt <= 0 && errno != EINTR)
    {
      delete pkt;
      disconnect ();
      return false;
    }
    remaining -= amt;
    count += amt;
  }
  if (str != NULL)
  {
    count = 0;
    while (strSize > 0)
    {
      amt = write (sock, str + count, strSize);
      if (amt <= 0 && errno != EINTR)
      {
	deletePacket (pkt);
	disconnect ();
	return false;
      }
      strSize -= amt;
      count += amt;
    }
  }
  return pkt;
}


NetPacket* NetComm::readData ()
{
  if (!connected)
    return NULL;

  NetPacket* pkt;
  Sint32 amt, remaining;
  Uint8 type;
  bool hasStr = false;
  char* str = NULL;

  amt = read (sock, &type, 1);
  if (amt <= 0 && errno != EINTR) // read error or closed link
  {
    disconnect ();
    return NULL;
  }
  else if (errno == EINTR) // read interrupted, calling function is welcome to retry
    return NULL;
  switch (type)
  {
   case NET_TYPE_CONNECT:
     pkt = new ConnectPacket;
     remaining = sizeof (struct ConnectPacket) - 1 - sizeof (char*);
     hasStr = true;
     break;
   case NET_TYPE_PLAYER:
     pkt = new PlayerPacket;
     remaining = sizeof (struct PlayerPacket) - 1 - sizeof (char*);
     hasStr = true;
     break;
   case NET_TYPE_UPDATE:
     pkt = new UpdatePacket;
     remaining = sizeof (struct UpdatePacket) - 1;
     break;
   case NET_TYPE_DEATH:
     pkt = new DeathPacket;
     remaining = sizeof (struct DeathPacket) - 1;
     break;
   case NET_TYPE_STATUS:
     pkt = new StatusPacket;
     remaining = sizeof (struct StatusPacket) - 1;
     break;
   case NET_TYPE_CONF:
     pkt = new ConfPacket;
     remaining = sizeof (struct ConfPacket) - 1 - sizeof (char*);
     hasStr = true;
     break;
   case NET_TYPE_CHAT:
     pkt = new ChatPacket;
     remaining = sizeof (struct ChatPacket) - 1 - sizeof (char*);
     hasStr = true;
     break;
   default:
     std::cout << "unknown packet type. the communication might be askew!" << std::endl;
     cleanUp ();
     return NULL; // something is fishy in the net-pond
  }
  pkt->type = type;
  
  Sint32 count = 1;
  while (remaining > 0)
  {
    amt = read (sock, ((char*)pkt) + count, remaining);
    if (amt <= 0 && errno != EINTR)
    {
      delete pkt;
      disconnect ();
      return NULL;
    }
    remaining -= amt;
    count += amt;
  }
  if (hasStr)
  {
    Sint32 strSize = 0;
    switch (pkt->type)
    {
     case NET_TYPE_CONNECT:
       ((struct ConnectPacket*)pkt)->name = new char[((struct ConnectPacket*)pkt)->nameLen];
       str = ((struct ConnectPacket*)pkt)->name;
       strSize = ((struct ConnectPacket*)pkt)->nameLen;
       break;
     case NET_TYPE_PLAYER:
       ((struct PlayerPacket*)pkt)->name = new char[((struct PlayerPacket*)pkt)->nameLen];
       str = ((struct PlayerPacket*)pkt)->name;
       strSize = ((struct PlayerPacket*)pkt)->nameLen;
       break;
     case NET_TYPE_CONF:
       ((struct ConfPacket*)pkt)->levelName = new char[((struct ConfPacket*)pkt)->nameLen];
       str = ((struct ConfPacket*)pkt)->levelName;
       strSize = ((struct ConfPacket*)pkt)->nameLen;
       break;
     case NET_TYPE_CHAT:
       ((struct ChatPacket*)pkt)->message = new char[((struct ChatPacket*)pkt)->messLen];
       str = ((struct ChatPacket*)pkt)->message;
       strSize = ((struct ChatPacket*)pkt)->messLen;
       break;
    }
    count = 0;
    while (strSize > 0)
    {
      amt = read (sock, str + count, strSize);
      if (amt <= 0 && errno != EINTR)
      {
	deletePacket (pkt);
	disconnect ();
	return NULL;
      }
      strSize -= amt;
      count += amt;
    }
  }
  return pkt;
}
