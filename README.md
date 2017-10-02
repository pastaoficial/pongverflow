== Compile with ==

gcc `pkg-config --cflags glib-2.0` -lncurses -lsqlite3 -lpthread -lglib-2.0 -I/usr/include/glib-2.0 -I./src/includes/ ./src/sound.c ./src/pong.c ./src/fancymenu.c ./src/timer.c `pkg-config --libs glib-2.0` -ggdb -o pong
