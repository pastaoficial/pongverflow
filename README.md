## Description

pongverflow is the client server version of the classic pong game.

the project is based on [!CPONGC](https://github.com/xavieran/CPONG) and use it sockets and pthread to supports connections.

this game was built for the ekoparty security conference as a secinfo challege, so don't get surprised if you found bugs

thanks to the glibc project for the beautiful data structures

## Future work

docs and fix

## Compilation

gcc `pkg-config --cflags glib-2.0` -lncurses -lsqlite3 -lpthread -lglib-2.0 -I/usr/include/glib-2.0 -I./src/includes/ ./src/sound.c ./src/pong.c ./src/fancymenu.c ./src/timer.c `pkg-config --libs glib-2.0` -ggdb -o pong
