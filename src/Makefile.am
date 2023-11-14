bin_PROGRAMS = cavepilot

AM_CXXFLAGS=-W -Wall -pedantic -DDATADIR=\"$(pkgdatadir)/\"

cavepilot_SOURCES = Configuration.cc Messages.cc Network.cc NetworkServer.cc NetworkClient.cc GfxCore.cc \
		    Console.cc Level.cc Player.cc Weapons.cc Status.cc GameText.cc Particles.cc Cavepilot.cc

noinst_HEADERS = Configuration.hh Messages.hh Network.hh NetworkServer.hh NetworkClient.hh GfxCore.hh \
		  Console.hh Level.hh Player.hh Weapons.hh GameText.hh Status.hh Particles.hh

cavepilot_LDADD = @INETLIB@
