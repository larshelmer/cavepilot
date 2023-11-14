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

#include "NetworkClient.hh"
#include "Configuration.hh"
#include "Player.hh"
#include "Level.hh"
#include "Weapons.hh"
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#include <iostream>

extern Configuration* conf;
extern Player* localPlayer;
extern Level* lvl;
extern NetPlayer** players;
extern SDL_mutex* updateMutex;
extern Weapons* wpns;

NetworkClient::NetworkClient (Messages* m, char* s) : Network (m)
{
  if (s == NULL)
  {
    messages->addIncoming ("Error: no server to connect to");
    return;
  }
  conn = NULL;
  server = new char[strlen (s) + 1];
  strcpy (server, s);
  playerNames = new char*[MAX_PLAYERS];
  for (Sint32 i = 0; i < MAX_PLAYERS; playerNames[i++] = NULL);
  gameStarted = false;
  canStart = false;
  currentPlayers = 1;
  maxPlayers = 1;
  if ((clientThread = SDL_CreateThread(&NetworkClient::startClientLoop, (void*)this)) == NULL)
    messages->addIncoming ("Error: could not create client thread");
}

NetworkClient::~NetworkClient ()
{
  
  if (server != NULL)
    delete[] server;
  if (conn != NULL)
    conn->disconnect ();
  SDL_WaitThread (clientThread, NULL);
  if (conn != NULL)
    delete conn;
  delete localPlayer;
  localPlayer = NULL;
}

Sint32 NetworkClient::startClientLoop (void* thisPtr)
{
  NetworkClient* netClient = (NetworkClient*)thisPtr;
  return netClient->clientLoop (NULL);
}

void NetworkClient::interpretPacket (NetPacket* pkt)
{
  struct ChatPacket* cp;
  struct PlayerPacket* pp;
  struct StatusPacket* sp;
  struct ConfPacket* lp;
  struct UpdatePacket* up;
  //  struct DeathPacket* dp;
  double x, y, vel, face, dir;

  switch (pkt->type)
  {
   case NET_TYPE_CONF:
     messages->addIncoming ("Got conf packet");
     // INSERT CODE -- here we are ready to rock
     lp = (struct ConfPacket*)pkt;
     localId = lp->id;
     //     conf->setLevel (lp->levelName);
     if (!lvl->loadLevel (lp->levelName))
     {
       connected = false;
       conn->disconnect ();
     }
     maxPlayers = lvl->getPlayers ();
     if (playerNames[lp->id] != NULL)
       delete[] playerNames[lp->id];
     playerNames[lp->id] = new char[strlen (conf->getNick ()) + 1];
     strcpy (playerNames[lp->id], conf->getNick ());
     break;
   case NET_TYPE_CONNECT:
     messages->addIncoming ("Strange: Got connect packet");
     break;
   case NET_TYPE_PLAYER:
     //     std::cout << "got player" << std::endl;
     messages->addIncoming ("Got player packet");
     pp = (struct PlayerPacket*)pkt;
     if (playerNames[pp->id] != NULL)
       delete[] playerNames[pp->id];
     playerNames[pp->id] = new char[pp->nameLen];
     strcpy (playerNames[pp->id], pp->name);
     if (players[pp->id] != NULL)
       delete players[pp->id];
     std::cout << "creating player" << std::endl;
     players[pp->id] = new NetPlayer (pp->id, pp->colour, pp->name);
     std::cout << "created player" << std::endl;
     currentPlayers++;
     break;
   case NET_TYPE_UPDATE:
     //     messages->addIncoming ("Got update packet");
     if (players[pkt->id] != NULL)
     {
       //       messages->addIncoming ("Got update packet");
       up = (struct UpdatePacket*)pkt;
       x = NET_TO_DOUBLE (up->x);
       y = NET_TO_DOUBLE (up->y);
       vel = NET_TO_DOUBLE (up->vel);
       face = NET_TO_DOUBLE (up->face);
       dir = NET_TO_DOUBLE (up->dir);
       players[up->id]->update (x, y, vel, face, dir, up->fire);
     }
     break;
   case NET_TYPE_DEATH:
     messages->addIncoming ("Got death packet");
     if (players[pkt->id] != NULL)
       killPlayer ((struct DeathPacket*)pkt);
     break;
   case NET_TYPE_CHAT:
     cp = (struct ChatPacket*)pkt;
     if (strncasecmp (cp->message, "/me ", 4) == 0)
       messages->addIncoming ("* %s %s", getPlayerName (cp->id), (cp->message + 4));
     else
       messages->addIncoming ("<%s> %s", getPlayerName (cp->id), cp->message);
     break;
   case NET_TYPE_STATUS:
     messages->addIncoming ("Got status packet: %d", ((struct StatusPacket*)pkt)->status);
     sp = (struct StatusPacket*)pkt;
     switch (sp->status)
     {
      case NET_STATUS_GAME_OVER:
	canStart = false;
	messages->addIncoming ("The game is over!");
      case NET_STATUS_JOIN:
	messages->addIncoming ("%s joined the game", getPlayerName (sp->id));
	if (players[sp->id] != NULL)
	  players[sp->id]->joinGame ();
	break;
      case NET_STATUS_GAME_STARTED:
	messages->addIncoming ("Game started. Press F1 to join");
	canStart = true;
      case NET_STATUS_DISCONNECT:
	currentPlayers--;
	break;
     }
     break;
  }
}

void NetworkClient::killPlayer (DeathPacket* dp)
{
  std::cout << "in net::killplayer" << std::endl;
  double x = NET_TO_DOUBLE (dp->x);
  double y = NET_TO_DOUBLE (dp->y);
  players[dp->id]->killPlayer (x, y, dp->wpn, dp->player, dp->out == true);
  if (dp->wpn == PLAYER_CRASH)
  {
    if (dp->player == localId)
    {
      SDL_LockMutex (updateMutex);
      localPlayer->collision ();
      SDL_UnlockMutex (updateMutex);
    }
    messages->addIncoming ("%s crashed with %s", getPlayerName (dp->id), getPlayerName (dp->player));
  }
  else if (dp->wpn == LEVEL_CRASH)
    messages->addIncoming ("%s flew into the wall", getPlayerName (dp->id));
  else
    messages->addIncoming ("%s got shot down by %s\'s %s", getPlayerName (dp->id), getPlayerName (dp->player), wpns->getWeaponName (dp->wpn));
  std::cout << "end net::killplayer" << std::endl;
}

char* NetworkClient::getPlayerName (Uint8 id)
{
  if (id < MAX_PLAYERS)
    return playerNames[id];
  return NULL;
}

Sint32 NetworkClient::clientLoop (void* ptr)
{
  ptr = 0;

  struct hostent* hostlist;
  struct sockaddr_in sa;
  Sint32 sock;

  messages->addIncoming ("Looking up %s...", server);
  if ((hostlist = gethostbyname (server)) == NULL)
  {
    messages->addIncoming ("Error: gethostbyname - %s", errno);
    return 0;
  }
  messages->addIncoming ("Opening socket...", server);
  if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
  {
    messages->addIncoming ("Error: socket - %s", errno);
    return 0;
  }
  memset (&sa, 0, sizeof (struct sockaddr_in));
  sa.sin_port = htons (conf->getPort ());
  memcpy (&sa.sin_addr, hostlist->h_addr_list[0], hostlist->h_length);
  sa.sin_family = AF_INET;
  messages->addIncoming ("Connecting...", server);
  if (connect (sock, (struct sockaddr*)&sa, sizeof (sa)) < 0)
  {
    messages->addIncoming ("Error: connect - %s", errno);
    return 0;
  }
  messages->addIncoming ("Creating connection...", server);
  connected = true;
  conn = new Connection (this, sock);

  NetPacket* pkt;
  messages->addIncoming ("Starting loop...", server);

  Sint32 turn = 0;

  while (connected)
  {
    if (!conn->isConnected ())
    {
      std::cout << "not connected" << std::endl;
      connected = false;
      while ((pkt = getPacket ()) != NULL)
      {
	if (pkt->type == NET_TYPE_STATUS)
	  interpretPacket (pkt);
	deletePacket (pkt);
      }
      messages->addIncoming ("Disconnected");
      continue;
    }
    if (turn < maxPlayers)
    {
      pkt = conn->getUpdate (turn);
      if (pkt != NULL)
      {
	interpretPacket (pkt);
	deletePacket (pkt);
      }
      turn++;
    }
    else if (turn == maxPlayers)
    {
      if (conn->isReady ())
	localUpdate ();
      turn++;
    }
    else
    {
      pkt = getPacket ();
      if (pkt != NULL)
      {
	interpretPacket (pkt);
	deletePacket (pkt);
      }
      if (conn->isReady ())
	sendChat ();
      turn = 0;
    }
    SDL_Delay (10);
  }
  return 0;
}

void NetworkClient::sendChat ()
{
  char* mess = messages->getOutgoing ();
  if (mess != NULL)
  {
    ChatPacket* cp = new ChatPacket;
    cp->type = NET_TYPE_CHAT;
    cp->id = localId;
    cp->messLen = strlen (mess) + 1;
    cp->message = mess;
    conn->sendData ((struct NetPacket*)cp);
    deletePacket (cp);
  }
}

void NetworkClient::localUpdate ()
{
  double x, y, vel, face, dir;
  Uint8 k, w, o;
  SDL_LockMutex (updateMutex);
  if (localPlayer->isInGame ())
  {
    if (localPlayer->hasMoved ())
    {
      if (localPlayer->isDead ())
      {
	x = localPlayer->getWorldX ();
	y = localPlayer->getWorldY ();
	k = localPlayer->getKiller ();
	w = localPlayer->getKillerWeapon ();
	o = localPlayer->isOut () ? 1 : 0;
	SDL_UnlockMutex (updateMutex);
	DeathPacket* dp = new DeathPacket;
	dp->type = NET_TYPE_DEATH;
	dp->id = localId;
	dp->x = DOUBLE_TO_NET (x);
	dp->y = DOUBLE_TO_NET (y);
	dp->wpn = w;
	dp->player = k;
	dp->out = o;
	conn->sendData ((struct NetPacket*)dp);
	deletePacket ((struct NetPacket*)dp);
	if (w == LEVEL_CRASH)
	  messages->addIncoming ("You flew into the wall"); 
	else if (w != PLAYER_CRASH)
	  messages->addIncoming ("You got shot down by %s\'s %s", getPlayerName (k), wpns->getWeaponName (w));
	if (o)
	  canStart = false;
      }
      else
      {
	UpdatePacket* up = new UpdatePacket;
	x = localPlayer->getWorldX ();
	y = localPlayer->getWorldY ();
	vel = localPlayer->getVelocity ();
	face = localPlayer->getFacing ();
	dir = localPlayer->getDirection ();
	if (localPlayer->firedPrimary ())
	  up->fire = localPlayer->getPrimaryWpn ();
	else if (localPlayer->firedSecondary ())
	  up->fire = localPlayer->getSecondaryWpn ();
	else
	  up->fire = NO_WEAPON;
	SDL_UnlockMutex (updateMutex);
	up->type = NET_TYPE_UPDATE;
	up->id = localId;
	up->x = DOUBLE_TO_NET (x);
	up->y = DOUBLE_TO_NET (y);
	up->vel = DOUBLE_TO_NET (vel);
	up->face = DOUBLE_TO_NET (face);
	up->dir = DOUBLE_TO_NET (dir);
	conn->sendData ((struct NetPacket*)up);
	deletePacket ((struct NetPacket*)up);
      }
    }
    else
      SDL_UnlockMutex (updateMutex);
  }
  else
    SDL_UnlockMutex (updateMutex);
}

bool NetworkClient::startGame ()
{
  if (!connected || !canStart)
  {
    messages->addIncoming ("Cannot join game yet");
    return false;
  }
  StatusPacket* sp = new StatusPacket;
  sp->type = NET_TYPE_STATUS;
  sp->id = localId;
  sp->status = NET_STATUS_JOIN;
  if (!conn->sendData (sp))
  {
    messages->addIncoming ("Join failed");
    return false;
  }
  gameStarted = true;
  messages->addIncoming ("Game joined");
  return true;
}

NetworkClient::Connection::Connection (NetworkClient* p, Sint32 s) : NetComm (s)
{
  ready = false;
  parent = p;
  packetMutex = SDL_CreateMutex ();
  updates = new NetPacket*[MAX_PLAYERS];
  for (Sint32 i = 0; i < MAX_PLAYERS; updates[i++] = NULL);
  if ((readThread = SDL_CreateThread(&Connection::startReadLoop, (void*)this)) == NULL)
    disconnect ();
}

NetworkClient::Connection::~Connection ()
{
  ready = false;
  SDL_WaitThread (readThread, NULL);
  SDL_LockMutex (packetMutex);
  for (Sint32 i = 0; i < MAX_PLAYERS; i++)
  {
    if (updates[i] != NULL)
    {
      delete updates[i];
      updates[i] = NULL;
    }
  }
  delete[] updates;
  SDL_UnlockMutex (packetMutex);
  SDL_DestroyMutex (packetMutex);
}

void NetworkClient::Connection::addUpdate (NetPacket* pkt, Uint8 i)
{
  if (i >= MAX_PLAYERS)
    return;
  SDL_LockMutex (packetMutex);
  if (updates[i] == NULL)
    updates[i] = pkt;
  else
  {
    if (updates[i]->type == NET_TYPE_DEATH)
      delete pkt;
    else
    {
      delete updates[i];
      updates[i] = pkt;
    }
  }
  SDL_UnlockMutex (packetMutex);
}

NetPacket* NetworkClient::Connection::getUpdate (Uint8 i)
{
  NetPacket* tmp;
  SDL_LockMutex (packetMutex);
  tmp = updates[i];
  updates[i] = NULL;
  SDL_UnlockMutex (packetMutex);
  return tmp;
}

Sint32 NetworkClient::Connection::readLoop (void* ptr)
{
  ptr = 0;

  NetPacket* pkt;
  Sint32 counter = 0;

  while (!ready)
  {
    pkt = readData ();
    if (pkt == NULL && !connected)
    {
      parent->disconnectNotify ("Error: read");
      return 0;
    }
    if (pkt->type == NET_TYPE_STATUS)
    {
      if (((struct StatusPacket*)pkt)->status == NET_STATUS_FULL)
      {
	parent->addPacket (pkt);
	disconnect ();
	return 0;
      }
      else if (((struct StatusPacket*)pkt)->status == NET_STATUS_HELLO)
      {
	parent->debug ("Got hello");
	parent->addPacket (pkt);
	break;
      }
      else
	deletePacket (pkt);
    }
    else
      deletePacket (pkt);
    if (counter == 5) // to many wrong packets
    {
      parent->disconnectNotify ("Error: login negotiation failed");
      deletePacket (pkt);
      disconnect ();
      return 0;
    }
    counter++;
    SDL_Delay (20);
  }

  // INSERT CODE -- send ConnectPacket

  ///// TESTCODE /////
  ConnectPacket* cp = new ConnectPacket;
  cp->type = NET_TYPE_CONNECT;
  cp->id = 0;
  cp->versMaj = 0;
  cp->versMin = 5;
  cp->versP = 0;
  cp->name = new char[strlen (conf->getNick ()) + 1];
  strcpy (cp->name, conf->getNick ());
  cp->nameLen = strlen (cp->name) + 1;
  parent->debug ("sending ConnectPacket");
  if (!sendData (cp))
  {
    parent->disconnectNotify ("Error: login negotiation failed");
    ready = false;
    disconnect ();
    deletePacket (cp);
    return 0;
  }
  deletePacket (cp);
  ///// -------- /////

  counter = 0;

  while (!ready)
  {
    pkt = readData ();
    if (pkt == NULL && !connected)
    {
      parent->disconnectNotify ("Error: read 2");
      return 0;
    }
    if (pkt->type == NET_TYPE_STATUS)
    {
      if (((struct StatusPacket*)pkt)->status == NET_STATUS_LOW_VERSION)
      {
	parent->addPacket (pkt);
	disconnect ();
	return 0;
      }
      else if (((struct StatusPacket*)pkt)->status == NET_STATUS_NICK_TAKEN)
      {
	parent->debug ("Nick is already taken");
	parent->addPacket (pkt);
	disconnect ();
	return 0;
      }
      else
	deletePacket (pkt);
    }
    if (pkt->type == NET_TYPE_CONF)
    {
      parent->debug ("Got conf");
      parent->addPacket (pkt);
      id = pkt->id;
      break;
    }
    else
      deletePacket (pkt);
    if (counter == 5) // to many wrong packets
    {
      parent->disconnectNotify ("Error: login negotiation failed");
      disconnect ();
      return 0;
    }
    SDL_Delay (20);
    counter++;
  }

  if (localPlayer != NULL)
    delete localPlayer; 
  localPlayer = new Player (id, conf->getColour ());
  ready = true;

  SDL_Delay (50); // give a chance to load the level etc...

  StatusPacket* sp = new StatusPacket;
  sp->type = NET_TYPE_STATUS;
  sp->id = id;
  sp->status = NET_STATUS_CLIENT_OK;
  parent->debug ("sending ok");
  if (!sendData (sp))
  {
    parent->disconnectNotify ("Error: login negotiation failed");
    ready = false;
    disconnect ();
  }
  deletePacket (sp);

  parent->debug ("starting loop...");
  while (connected)
  {
    pkt = readData ();
    if (pkt == NULL && !connected)
    {
      parent->disconnectNotify ("Error: read");
      break;
    }
    //    std::cout << "got packet" << std::endl;
    if (pkt->id == id)
    {
      std::cout << "own id" << std::endl;
      deletePacket (pkt);
      continue;
    }
    switch (pkt->type)
    {
     case NET_TYPE_CONF:
     case NET_TYPE_CONNECT:
       deletePacket (pkt);  // silently ignored
       break;
     case NET_TYPE_UPDATE:
     case NET_TYPE_DEATH:
       addUpdate (pkt, pkt->id);
       break;
     case NET_TYPE_PLAYER:
     case NET_TYPE_CHAT:
       parent->addPacket (pkt);
       break;
     case NET_TYPE_STATUS:
       switch (((struct StatusPacket*)pkt)->status)
       {
	case NET_STATUS_JOIN:
	case NET_STATUS_GAME_OVER:
	case NET_STATUS_DISCONNECT:
	case NET_STATUS_GAME_STARTED:
	  parent->addPacket (pkt);
	  break;
	case NET_STATUS_HELLO:
	case NET_STATUS_LOW_VERSION:
	case NET_STATUS_NICK_TAKEN:
	case NET_STATUS_CLIENT_OK:
	case NET_STATUS_FULL:
	default:
	  deletePacket (pkt);  // silently ignored
	  break;
       }
       break;
    }
    SDL_Delay (10);
  }

  return 0;
}

Sint32 NetworkClient::Connection::startReadLoop (void* thisPtr)
{
  Connection* conn = (Connection*)thisPtr;
  return conn->readLoop (NULL);  
}
