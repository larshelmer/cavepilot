# Copyright (C) 2004 Lars Helmer, see COPYING for details

NAME		= cavepilot
VERSION		= 0.5.0
CC		= g++
SOURCES		= Configuration.cc Messages.cc Network.cc NetworkServer.cc NetworkClient.cc GfxCore.cc \
		  Console.cc Level.cc Player.cc Weapons.cc Status.cc GameText.cc Cavepilot.cc
INCLUDES	= Configuration.hh Messages.hh Network.hh NetworkServer.hh NetworkClient.hh GfxCore.hh \
		  Console.hh Level.hh Player.hh Weapons.hh GameText.hh Status.hh
CFLAGS		+= -D_VERSION=\"$(VERSION)\" -Wall -W -pedantic -g `sdl-config --cflags`
LDFLAGS		= `sdl-config --libs`
OBJS		= $(SOURCES:.cpp=.o)

%.o:%.cpp
		$(CC) $(CFLAGS) -c -o $@ $<

$(NAME):	$(OBJS) $(INCLUDES)
		$(CC) $(OBJS) $(LDFLAGS) -o $(NAME)

clean:		
#		rm -f $(NAME) $(OBJS) *~
