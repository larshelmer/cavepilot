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

#ifndef _NETWORKSERVER_H
#define _NETWORKSERVER_H

#include "Network.hh"

class NetworkServer : public Network
{
 private:
  class Slot : public NetComm
  {
   private:
    bool sendConfig ();
    void disconnectNotify ();
    void addUpdate (NetPacket*);
    NetworkServer* parent;
    Uint8 id;
    bool ready;
    SDL_mutex* packetMutex;
    NetPacket* update;
    SDL_Thread* readThread;
   public:
    Slot (NetworkServer*, Uint8, Sint32);
    ~Slot ();
    bool isReady () { return ready; }
    static Sint32 startReadLoop (void*);
    Sint32 readLoop (void*);
    Uint8 getId () { return id; }
    NetPacket* getUpdate ();
  };
  static Sint32 startAcceptLoop (void*);
  Sint32 acceptLoop (void*);
  SDL_Thread* acceptThread;
  static Sint32 startServerLoop (void*);
  Sint32 serverLoop (void*);
  SDL_Thread* serverThread;
  bool sendStatus (Sint32, Uint8);
  void deletePlayer (Uint8);
  char* getPlayerName (Uint8);
  void interpretPacket (NetPacket*);
  void sendToAll (NetPacket*);
  void localUpdate ();
  void sendChat ();
  void killPlayer (DeathPacket*);
  Uint8 currentPlayers;
  Uint8 maxPlayers;
  Slot** slots;
  SDL_mutex* playerMutex;
  PacketWrapper* cPlayers;
  bool gameStarted;
  Sint32 listener;
 public:
  NetworkServer (Messages*);
  ~NetworkServer ();
  bool verifyVersion (Uint8, Uint8, Uint8);
  bool addPlayer (ConnectPacket*, Uint8);
  bool sendPlayers (Slot*);
  bool isGameStarted () { return gameStarted; }
  bool startGame ();
};

#endif // _NETWORKSERVER_H 
