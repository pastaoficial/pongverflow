//      fancymenu.h
//
//      Copyright 2009 Emmanuel Jacyna <xavieran.lives@gmail.com>
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.


struct Menu {
    WINDOW* win;
    int selected;
    int dirty;
    int ITEM_COLOR;
    int SEL_ITEM_COLOR;
    int num_items;
    int x, y;
    int width, height;
    char** items;
};

struct Menu* new_menu(int startx, int starty, int num_items, char** items, int ITEM_COLOR, int SEL_ITEM_COLOR);

void draw_menu(struct Menu* menu);
void move_selected(struct Menu* menu, int direction);
int poll_menu(struct Menu* menu);
