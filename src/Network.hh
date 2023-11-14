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

#ifndef _NETWORK_H
#define _NETWORK_H

#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include "Messages.hh"

#define MAX_PLAYERS 16 // just a temp value. it is really defined by each level
#define NO_WEAPON 42

#define DOUBLE_TO_NET(dbl) (htonl ((Sint32)((double)dbl * 65536.0)))
#define NET_TO_DOUBLE(net) ((double)ntohl (net) / 65536.0)

// packet types
#define NET_TYPE_CONNECT 0
#define NET_TYPE_PLAYER 1
#define NET_TYPE_UPDATE 2
#define NET_TYPE_DEATH 3
#define NET_TYPE_STATUS 4
#define NET_TYPE_CONF 5
#define NET_TYPE_CHAT 6

// status messages
#define NET_STATUS_HELLO 0
#define NET_STATUS_LOW_VERSION 1
#define NET_STATUS_NICK_TAKEN 2
#define NET_STATUS_CLIENT_OK 3
#define NET_STATUS_JOIN 4
#define NET_STATUS_GAME_OVER 5
#define NET_STATUS_GAME_STARTED 6
#define NET_STATUS_DISCONNECT 7
#define NET_STATUS_FULL 8
//#define NET_STATUS_OUT 9

struct NetPacket
{
  Uint8 type;
  Uint8 id;
};

struct ConnectPacket : public NetPacket // sent by CLIENT when connecting. Do not use "id"
{
  Uint8 colour;
  Uint8 versMaj;  // major version number, e.g., 0
  Uint8 versMin;  // minor version number, e.g., 5
  Uint8 versP;    // patch version number, e.g., 0
  Uint8 nameLen;
  char* name;
};

struct PlayerPacket : public NetPacket // sent by the SERVER to inform newly joined players of other players
{
  Uint8 colour;
  Uint8 nameLen;
  char* name;
};

struct UpdatePacket : public NetPacket
{
  Sint32 x, y, vel, face, dir;
  Uint8 fire;
};

struct DeathPacket : public NetPacket
{
  Sint32 x, y;
  Uint8 wpn;
  Uint8 player;
  Uint8 out;
};

struct StatusPacket : public NetPacket
{
  Uint8 status;
};

struct ConfPacket : public NetPacket
{
  Uint8 nameLen;
  char* levelName;
};

struct ChatPacket : public NetPacket
{
  Uint8 messLen;
  char* message;
};

struct PacketWrapper
{
  NetPacket* pkt;
  PacketWrapper* next;
};

class NetComm
{
 private:
  Sint32 sock;
 protected:
  bool connected;
  NetPacket* readData ();
  void cleanUp ();
 public:
  NetComm (Sint32);
  ~NetComm ();
  bool sendData (NetPacket*);
  bool isConnected () { return connected; }
  void disconnect ();
};

class Network
{
 protected:
  NetPacket* getPacket ();
  bool connected;
  Uint8 localId;
  Messages* messages;
  SDL_mutex* inqueueMutex;
  PacketWrapper* inFirst;
  PacketWrapper* inLast;
 public:
  Network (Messages* m);
  virtual ~Network ();
  bool isConnected () { return connected; }
  Uint8 getLocalId () { return localId; }
  void addPacket (NetPacket*);
  void debug (char* m) { messages->addIncoming ("DEBUG: %s", m); }
  virtual bool isGameStarted () { return false; }
  virtual bool startGame () { return false; }
};

void deletePacket (NetPacket*);

#endif // _NETWORK_H 
