#Makefile for CPONGC
##

SHELL=/bin/sh
DEBUG_OPTIONS=-std=c99 -ggdb3 -Wall -Wextra -pedantic
GLIB = -I/usr/include/glib-2.0

SRCDIR=./src/
SOURCES_=sound.c pong.c fancymenu.c timer.c
SOURCES=$(addprefix $(SRCDIR),$(SOURCES_))

INCLUDES_=ncurses sqlite3 glib-2.0
INCLUDES=$(addprefix -l,$(INCLUDES_))
MY_INCLUDES=./src/includes/

EXE=pong


all:
	$(CC) $(INCLUDES) -I$(MY_INCLUDES) $(SOURCES) -o $(EXE)

debug:
	$(CC) $(DEBUG_OPTIONS) $(INCLUDES) -I$(MY_INCLUDES) $(SOURCES) -o $(EXE)
glib:
	$(CC) $(INCLUDES) $(GLIB) -I$(MY_INCLUDES) $(SOURCES) -o $(EXE)
