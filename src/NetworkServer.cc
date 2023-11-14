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

#include "NetworkServer.hh"
#include "Configuration.hh"
#include "Player.hh"
#include "Level.hh"
#include "Weapons.hh"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include <iostream>

#define MAX_PENDING 5

extern Configuration* conf;
extern Player* localPlayer;
extern Level* lvl;
extern NetPlayer** players;
extern SDL_mutex* updateMutex;
extern Weapons* wpns;

NetworkServer::NetworkServer (Messages* m) : Network (m)
{
  currentPlayers = 0;
  maxPlayers = lvl->getPlayers ();
  slots = new Slot*[MAX_PLAYERS];
  for (int i = 0; i < MAX_PLAYERS; slots[i++] = NULL);
  playerMutex = SDL_CreateMutex ();
  cPlayers = NULL;
  gameStarted = false;
  std::cout << "ns constr: maxPlayers: " << maxPlayers << std::endl;
  if ((acceptThread = SDL_CreateThread (&NetworkServer::startAcceptLoop, (void*)this)) == NULL)
    messages->addIncoming ("Error: could not create server thread (1)");
}

NetworkServer::~NetworkServer ()
{
  connected = false;
  close (listener);
  SDL_KillThread (acceptThread); // apparently closing listener won't make accept fail...
  SDL_WaitThread (serverThread, NULL);
  for (Sint32 i = 0; i < MAX_PLAYERS; i++)
  {
    if (slots[i] != NULL)
    {
      slots[i]->disconnect ();
      delete slots[i];
    }
  }
  delete[] slots;
  SDL_LockMutex (playerMutex);
  PacketWrapper* tmp = cPlayers;
  while (tmp != NULL)
  {
    tmp = cPlayers->next;
    deletePacket (cPlayers->pkt);
    delete cPlayers;
    cPlayers = tmp;
  }
  SDL_UnlockMutex (playerMutex);
  SDL_DestroyMutex (playerMutex);
  SDL_LockMutex (updateMutex);
  delete localPlayer;
  localPlayer = NULL;
  SDL_UnlockMutex (updateMutex);
}

Sint32 NetworkServer::startAcceptLoop (void* thisPtr)
{
  NetworkServer* netServ = (NetworkServer*)thisPtr;
  return netServ->acceptLoop (NULL);
}

Sint32 NetworkServer::startServerLoop (void* thisPtr)
{
  NetworkServer* netServ = (NetworkServer*)thisPtr;
  return netServ->serverLoop (NULL);
}

void NetworkServer::localUpdate ()
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
	sendToAll ((struct NetPacket*)dp);
	deletePacket ((struct NetPacket*)dp);
	if (w == LEVEL_CRASH)
	  messages->addIncoming ("You flew into the wall");
	else if (w != PLAYER_CRASH)
	  messages->addIncoming ("You got shot down by %s\'s %s", getPlayerName (k), wpns->getWeaponName (w));
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
	sendToAll ((struct NetPacket*)up);
	deletePacket ((struct NetPacket*)up);
      }
    }
    else
      SDL_UnlockMutex (updateMutex);
  }
  else
    SDL_UnlockMutex (updateMutex);
}

void NetworkServer::sendChat ()
{
  char* mess = messages->getOutgoing ();
  if (mess != NULL)
  {
    ChatPacket* cp = new ChatPacket;
    cp->type = NET_TYPE_CHAT;
    cp->id = localId;
    cp->messLen = strlen (mess) + 1;
    cp->message = mess;
    sendToAll ((struct NetPacket*)cp);
    deletePacket (cp);
  }
}

void NetworkServer::sendToAll (NetPacket* pkt)
{
  for (Sint32 i = 0; i < maxPlayers; i++)
    if (slots[i] != NULL)
      if (slots[i]->isConnected () && slots[i]->isReady ())
	if (slots[i]->getId () != pkt->id)
	  slots[i]->sendData ((struct NetPacket*)pkt);
}

Sint32 NetworkServer::serverLoop (void* ptr)
{
  ptr = 0;

  Sint32 turn = 0;
  NetPacket* pkt;
  messages->addIncoming ("Starting main server loop...");

  while (connected)
  {
    if (turn < maxPlayers)
    {
      if (slots[turn] != NULL)
      {
	if (slots[turn]->isConnected () && slots[turn]->isReady ())
	{
	  pkt = slots[turn]->getUpdate ();
	  if (pkt != NULL)
	  {
	    sendToAll (pkt);
	    interpretPacket (pkt);
	    deletePacket (pkt);
	  }
	}
      }
      turn++;
    }
    else if (turn == maxPlayers)
    {
      localUpdate ();
      turn++;
    }
    else
    {
      pkt = getPacket ();
      if (pkt != NULL)
      {
	sendToAll (pkt);
	interpretPacket (pkt);
	deletePacket (pkt);
      }
      sendChat ();
      turn = 0;
    }
    SDL_Delay (10);
  }
  return 0;
}

void NetworkServer::interpretPacket (NetPacket* pkt)
{
  ChatPacket* cp;
  StatusPacket* sp;
  UpdatePacket* up;
  //  struct DeathPacket* dp;
  double x, y, vel, face, dir;

  //  std::cout << "in interpret" << std::endl;

  switch (pkt->type)
  {
   case NET_TYPE_CONF:
     messages->addIncoming ("Strange: Got conf packet");
     break;
   case NET_TYPE_CONNECT:
     messages->addIncoming ("Strange: Got connect packet");
     break;
   case NET_TYPE_PLAYER:
     messages->addIncoming ("Got player packet - ignoring");
     break;
   case NET_TYPE_UPDATE:
     //     std::cout << "Got update packet" << std::endl;
     if (players[pkt->id - 1] != NULL)
     {
       up = (UpdatePacket*)pkt;
       x = NET_TO_DOUBLE (up->x);
       y = NET_TO_DOUBLE (up->y);
       vel = NET_TO_DOUBLE (up->vel);
       face = NET_TO_DOUBLE (up->face);
       dir = NET_TO_DOUBLE (up->dir);
       players[up->id - 1]->update (x, y, vel, face, dir, up->fire);
     }
     break;
   case NET_TYPE_DEATH:
     // INSERT CODE -- collisions, crashes, weapon specification
     messages->addIncoming ("Got death packet");
     if (players[pkt->id - 1] != NULL)
       killPlayer ((DeathPacket*)pkt);
     break;
   case NET_TYPE_CHAT:
     //     messages->addIncoming ("Got chat packet");
     cp = (ChatPacket*)pkt;
     if (strncasecmp (cp->message, "/me ", 4) == 0)
       messages->addIncoming ("* %s %s", getPlayerName (cp->id), (cp->message + 4));
     else
       messages->addIncoming ("<%s> %s", getPlayerName (cp->id), cp->message);
     break;
   case NET_TYPE_STATUS:
     sp = (StatusPacket*)pkt;
     switch (sp->status)
     {
      case NET_STATUS_JOIN:
	if (pkt->id != localId)
	{
	  messages->addIncoming ("%s joined the game", getPlayerName (sp->id));
	  if (players[sp->id - 1] != NULL)
	    players[sp->id - 1]->joinGame ();
	}
	break;
	// case NET_STATUS_GAME_OVER: ???
      case NET_STATUS_DISCONNECT:
	messages->addIncoming ("%s disconnected", getPlayerName (sp->id));
	if (players[sp->id - 1] != NULL)
	  players[sp->id - 1]->partGame ();
	deletePlayer (sp->id);
	break;
     }
     break;
  }
}

void NetworkServer::killPlayer (DeathPacket* dp)
{
  double x = NET_TO_DOUBLE (dp->x);
  double y = NET_TO_DOUBLE (dp->y);
  players[dp->id - 1]->killPlayer (x, y, dp->wpn, dp->player, dp->out == true);
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
  if (dp->out)
  {
    Sint32 plyIn = 0;
    if (!localPlayer->isOut ())
      plyIn++;
    for (Sint32 i = 0; i < maxPlayers; i++)
      if (players[i] != NULL)
	if (players[i]->isInGame ())
	  if (!players[i]->isOut ())
	    plyIn++;
    if (plyIn <= 1) // the game is over
    {
      StatusPacket* sp = new StatusPacket;
      sp->type = NET_TYPE_STATUS;
      sp->id = localId;
      sp->status = NET_STATUS_GAME_OVER;
      sendToAll (sp);
      delete sp;
      gameStarted = false;
      messages->addIncoming ("The game is over!");
    }
  }
}

Sint32 NetworkServer::acceptLoop (void* ptr)
{
  ptr = 0;

  Sint32 client;
  struct sockaddr_in sa;
  socklen_t saLen;

  std::cout << "in acceptloop" << std::endl;

  messages->addIncoming ("Starting up server...");

  if ((listener = socket (PF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
  {
    messages->addIncoming ("Error: socket - %s", errno);
    return 0;
  }
  saLen = sizeof (sa);
  memset (&sa, 0, saLen);
  sa.sin_family = AF_INET;
  sa.sin_port = htons (conf->getPort ());
  sa.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (listener, (struct sockaddr*)&sa, saLen) < 0)
  {
    messages->addIncoming ("Error: bind - %s", errno);
    return 0;
  }
  if (listen (listener, MAX_PENDING) < 0)
  {
    messages->addIncoming ("Error: listen - %s", errno);
    return 0;
  }

  localId = 0;
  PlayerPacket* pp = new PlayerPacket;
  pp->type = NET_TYPE_PLAYER;
  pp->id = localId;
  pp->colour = conf->getColour ();
  pp->name = new char[strlen (conf->getNick ()) + 1];
  strcpy (pp->name, conf->getNick ());
  pp->nameLen = strlen (pp->name) + 1;
  cPlayers = new PacketWrapper;
  cPlayers->next = NULL;
  cPlayers->pkt = pp;

  if (localPlayer != NULL)
    delete localPlayer;
  localPlayer = new Player (localId, conf->getColour ());
  currentPlayers++;

  connected = true;

  if ((serverThread = SDL_CreateThread(&NetworkServer::startServerLoop, (void*)this)) == NULL)
  {
    messages->addIncoming ("Error: could not create server thread (2)");
    connected = false;
  }

  if (connected)
    messages->addIncoming ("Press F1 to start game");

  char dottedIP[15];

  while (connected)
  {
    memset (&sa, 0, saLen);
    if ((client = accept (listener, (struct sockaddr*)&sa, &saLen)) < 0)
    {
      // INSERT CODE -- should verify that the socket is still valid
      messages->addIncoming ("Warning: accept - %s", errno);
      continue;
    }
    inet_ntop (AF_INET, &sa.sin_addr, dottedIP, 15);
    messages->addIncoming ("Connection from %s", dottedIP);
    if (currentPlayers >= maxPlayers)
    {
      messages->addIncoming ("Connection rejected: server full");
      sendStatus (client, NET_STATUS_FULL);
      close (client);
      continue;
    }
    messages->addIncoming ("Sending hello...");
    if (!sendStatus (client, NET_STATUS_HELLO))
    {
      messages->addIncoming ("Hello failed. Closing connection");
      continue;
    }
    Sint32 i;
    for (i = 0; i < maxPlayers; i++)
    {
      if (slots[i] == NULL)
      {
	slots[i] = new Slot (this, i + 1, client);
	break;
      }
      else if (!slots[i]->isConnected ())
      {
	delete slots[i];
	slots[i] = new Slot (this, i + 1, client);
	break;
      }
    }
    if (i == MAX_PLAYERS)
      messages->addIncoming ("The slots-array is full. You shouldn't see this.");
    SDL_Delay (10);
  }
  close (listener);
  return 0;
}

void NetworkServer::deletePlayer (Uint8 id)
{
  SDL_LockMutex (playerMutex);
  PacketWrapper* prev = NULL;
  for (PacketWrapper* tmp = cPlayers; tmp != NULL; tmp = tmp->next)
  {
    if (id == ((struct PlayerPacket*)tmp->pkt)->id)
    {
      if (prev == NULL)
      {
	cPlayers = tmp->next;
	deletePacket (tmp->pkt);
	delete tmp;
      }
      else
      {
	prev->next = tmp->next;
	deletePacket (tmp->pkt);
	delete tmp;
      }
      break;
    }
    prev = tmp;
  }
  SDL_UnlockMutex (playerMutex);
  currentPlayers--;
}

bool NetworkServer::sendStatus (Sint32 sock, Uint8 status)
{
  StatusPacket* pkt = new StatusPacket;
  pkt->type = NET_TYPE_STATUS;
  pkt->id = localId;
  pkt->status = status;
  Sint32 count = 0;
  Sint32 remaining = sizeof (struct StatusPacket);
  Sint32 amt;
  while (remaining > 0)
  {
    amt = write (sock, ((char*)pkt) + count, remaining);
    if (amt <= 0 && errno != EINTR)
    {
      close (sock);
      return false;
    }
    remaining -= amt;
    count += amt;
  }
  return true;
}

bool NetworkServer::verifyVersion (Uint8 maj, Uint8 min, Uint8 p)
{
  // INSERT CODE -- proper version checking
  maj += 0;
  p += 0;

  if (min < 5)
    return false;
  return true;
}

bool NetworkServer::addPlayer (ConnectPacket* pkt, Uint8 id)
{
  PlayerPacket* pp = new PlayerPacket;
  messages->addIncoming ("name: %s", pkt->name);
  pp->type = NET_TYPE_PLAYER;
  pp->id = id;
  pp->colour = pkt->colour;
  pp->nameLen = pkt->nameLen;
  pp->name = pkt->name;
  delete pkt;
  PacketWrapper* tmp = new PacketWrapper;
  tmp->pkt = pp;
  tmp->next = NULL;
  SDL_LockMutex (playerMutex);
  if (cPlayers == NULL)
    cPlayers = tmp;
  else
  {
    PacketWrapper* c = cPlayers;
    if (strcmp (((struct PlayerPacket*)c->pkt)->name, pp->name) == 0) // verify this!
    {
      deletePacket (pp);
      delete tmp;
      SDL_UnlockMutex (playerMutex);
      return false;
    }
    while (c->next != NULL)
    {
      if (strcmp (((struct PlayerPacket*)c->next->pkt)->name, pp->name) == 0) // verify this!
      {
	deletePacket (pp);
	delete tmp;
	SDL_UnlockMutex (playerMutex);
	return false;
      }
      else 
	c = c->next;
    }
    c->next = tmp;
  }
  SDL_UnlockMutex (playerMutex);
  PlayerPacket* pp2 = new PlayerPacket;
  pp2->type = pp->type;
  pp2->id = pp->id;
  pp2->colour = pp->colour;
  pp2->nameLen = pp->nameLen;
  pp2->name = new char[strlen (pp->name) + 1];
  strcpy (pp2->name, pp->name);
  if (players[pp2->id - 1] != NULL)
    delete players[pp2->id - 1];
  players[pp2->id - 1] = new NetPlayer (pp2->id, pp2->colour, pp2->name);
  addPacket (pp2);
  messages->addIncoming ("%s connected", pp->name);
  currentPlayers++;
  return true;
}

bool NetworkServer::sendPlayers (Slot* sender)
{
  Uint8 id = sender->getId ();
  SDL_LockMutex (playerMutex);
  PacketWrapper* tmp = cPlayers;
  while (tmp != NULL && sender->isConnected ())
  {
    if (id != ((struct PlayerPacket*)tmp->pkt)->id)
    {
      if (!sender->sendData (tmp->pkt))
      {
	if (sender->isConnected ())
	{
	  if (!sender->sendData (tmp->pkt))
	  {
	    SDL_UnlockMutex (playerMutex);
	    return false;
	  }
	}
	else
	{
	  SDL_UnlockMutex (playerMutex);
	  return false;
	}
      }
    }
    tmp = tmp->next;
  }
  SDL_UnlockMutex (playerMutex);
  return true;
}

char* NetworkServer::getPlayerName (Uint8 id)
{
  char* name = NULL;
  SDL_LockMutex (playerMutex);
  for (PacketWrapper* tmp = cPlayers; tmp != NULL; tmp = tmp->next)
  {
    if (id == tmp->pkt->id)
    {
      name = new char[strlen (((struct PlayerPacket*)tmp->pkt)->name) + 1];
      strcpy (name, ((struct PlayerPacket*)tmp->pkt)->name);
      break;
    }
  }
  SDL_UnlockMutex (playerMutex);
  return name;
}

bool NetworkServer::startGame ()
{
  if (!connected && gameStarted)
  {
    messages->addIncoming ("Cannot start game");
    return false;
  }
  gameStarted = true;
  StatusPacket* sp = new StatusPacket;
  sp->type = NET_TYPE_STATUS;
  sp->id = localId;
  sp->status = NET_STATUS_GAME_STARTED;
  addPacket (sp);
  messages->addIncoming ("Game started");
  StatusPacket* sp2 = new StatusPacket;
  sp2->type = NET_TYPE_STATUS;
  sp2->id = localId;
  sp2->status = NET_STATUS_JOIN;
  addPacket (sp2);
  return true;
}

NetworkServer::Slot::Slot (NetworkServer* p, Uint8 i, Sint32 s) : NetComm (s)
{
  parent = p;
  id = i;
  ready = false;
  packetMutex = SDL_CreateMutex ();
  update = NULL;
  if ((readThread = SDL_CreateThread(&Slot::startReadLoop, (void*)this)) == NULL)
    disconnect ();
}

NetworkServer::Slot::~Slot ()
{
  ready = false;
  SDL_WaitThread (readThread, NULL);
  SDL_LockMutex (packetMutex);
  if (update != NULL)
  {
    delete update;
    update = NULL;
  }
  SDL_UnlockMutex (packetMutex);
  SDL_DestroyMutex (packetMutex);
}

void NetworkServer::Slot::addUpdate (NetPacket* pkt)
{
  SDL_LockMutex (packetMutex);
  if (update == NULL)
    update = pkt;
  else
  {
    if (update->type == NET_TYPE_DEATH)
      delete pkt;
    else
    {
      delete update;
      update = pkt;
    }
  }
  SDL_UnlockMutex (packetMutex);
}

NetPacket* NetworkServer::Slot::getUpdate ()
{
  NetPacket* tmp;
  SDL_LockMutex (packetMutex);
  tmp = update;
  update = NULL;
  SDL_UnlockMutex (packetMutex);
  return tmp;
}

Sint32 NetworkServer::Slot::readLoop (void* ptr)
{
  ptr = 0;

  NetPacket* pkt;
  Sint32 counter = 0;

  while (!ready)
  {
    pkt = readData ();
    if (pkt == NULL && !connected)
    {
      disconnectNotify ();
      break;
    }
    if (pkt->type == NET_TYPE_CONNECT)
    {
      ConnectPacket* cp = (struct ConnectPacket*)pkt; // verify that this works!
      if (!parent->verifyVersion (cp->versMaj, cp->versMin, cp->versP))
      {
	parent->debug ("verifyVersion failed");
	StatusPacket* tmp = new StatusPacket;
	tmp->type = NET_TYPE_STATUS;
	tmp->id = 0;
	tmp->status = NET_STATUS_LOW_VERSION;
	sendData (tmp);
	delete tmp;
	deletePacket (pkt);
	disconnect ();
	//	disconnectNotify ();
	return 0;
      }
      else
	parent->debug ("verifyVersion succeeded");
      if (!parent->addPlayer (cp, id))
      {
	parent->debug ("addPlayer failed");
	StatusPacket* tmp = new StatusPacket;
	tmp->type = NET_TYPE_STATUS;
	tmp->id = 0;
	tmp->status = NET_STATUS_NICK_TAKEN;
	sendData (tmp);
	delete tmp;
	disconnect ();
	//	disconnectNotify ();
	return 0;
      }
      break;
    }
    else
    {
      parent->debug ("Did NOT get ConnectPacket");
      deletePacket (pkt);
      if (counter == 5) // too many wrong packets and you're fucked...
      {
	disconnect ();
	//	disconnectNotify ();
	return 0;
      }
      counter++;
      SDL_Delay (20);
    }
  }

  if (!sendConfig ())
  {
    parent->debug ("sendConfig failed");
    disconnect ();
    disconnectNotify ();
    return 0;
  }
  parent->debug ("sendConfig succeeded");

  counter = 0;
  
  while (!ready)
  {
    pkt = readData ();
    if (pkt == NULL && !connected)
    {
      disconnectNotify ();
      return 0;
    }
    if (pkt->type == NET_TYPE_STATUS)
    {
      if (((struct StatusPacket*)pkt)->status == NET_STATUS_CLIENT_OK)
      {
	parent->debug ("Got ok from client");
	deletePacket (pkt);
	break;
      }
    }
    else
    {
      if (counter == 5) // too many wrong packets and you're fucked...
      {
	disconnect ();
	disconnectNotify ();
	deletePacket (pkt);
	return 0;
      }
      counter++;
      SDL_Delay (20);
    }
    deletePacket (pkt);
  }

  parent->debug ("sending players");
  if (!parent->sendPlayers (this))
  {
    disconnect ();
    disconnectNotify ();
    return 0;
  }

  if (parent->isGameStarted ())
  {
    StatusPacket* sp = new StatusPacket;
    sp->type = NET_TYPE_STATUS;
    sp->id = 0;
    sp->status = NET_STATUS_GAME_STARTED;
    sendData (sp);
    sp->status = NET_STATUS_JOIN;
    sendData (sp);
    for (Sint32 i = 0; i < MAX_PLAYERS; i++)
    {
      if (players[i] != NULL)
      {
	if (players[i]->isInGame ())
	{
	  sp->id = i + 1;
	  sendData (sp);
	}
      }
    }
    delete sp;
  }

  ready = true; // the client should be considered ready to receive game updates

  parent->debug ("starting loop");
  while (connected)
  {
    pkt = readData ();
    if (pkt == NULL && !connected)
    {
      disconnectNotify ();
      break;
    }
    //    std::cout << "got packet" << std::endl;
    if (pkt->id != id)
    {
      std::cout << "wrong id: " << (int)pkt->id << std::endl;
      deletePacket (pkt);
      continue;
    }
    switch (pkt->type)
    {
     case NET_TYPE_CONF:
     case NET_TYPE_CONNECT:
     case NET_TYPE_PLAYER:
       deletePacket (pkt);  // silently ignored
       break;
     case NET_TYPE_UPDATE:
     case NET_TYPE_DEATH:
       addUpdate (pkt);
       break;
     case NET_TYPE_CHAT:
       parent->addPacket (pkt);
       break;
     case NET_TYPE_STATUS:
       switch (((struct StatusPacket*)pkt)->status)
       {
	case NET_STATUS_JOIN:
	case NET_STATUS_GAME_OVER:
	case NET_STATUS_DISCONNECT:
	  parent->addPacket (pkt);
	  break;
	case NET_STATUS_HELLO:
	case NET_STATUS_LOW_VERSION:
	case NET_STATUS_NICK_TAKEN:
	case NET_STATUS_CLIENT_OK:
	case NET_STATUS_FULL:
	case NET_STATUS_GAME_STARTED:
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

bool NetworkServer::Slot::sendConfig ()
{
  // INSERT CODE
  // send configuration. if sendData fails but we're still connected, retry. otherwise return false
  ConfPacket* cp = new ConfPacket;
  cp->type = NET_TYPE_CONF;
  cp->id = id;
  cp->levelName = new char[strlen (lvl->getLevel ()) + 1];
  strcpy (cp->levelName, lvl->getLevel ());
  cp->nameLen = strlen (cp->levelName) + 1;
  sendData (cp);
  deletePacket (cp);
  return true;
}

void NetworkServer::Slot::disconnectNotify ()
{
  StatusPacket* pkt = new StatusPacket;
  pkt->type = NET_TYPE_STATUS;
  pkt->id = id;
  pkt->status = NET_STATUS_DISCONNECT;
  parent->addPacket (pkt);
}

Sint32 NetworkServer::Slot::startReadLoop (void* thisPtr)
{
  Slot* slot = (Slot*)thisPtr;
  return slot->readLoop (NULL);  
}
