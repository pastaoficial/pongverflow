//      fancymenu.c
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

//Satisfy GCC about implicit declaration of usleep...
#define _BSD_SOURCE

#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>


enum directions {
   UP,
   DOWN,
   LEFT,
   RIGHT
};


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

int longest_str(int str_c, char** str)
//Return the length of the longest str of an array
{
    int i;
    int* lens = malloc(sizeof(int)*str_c);
    for (i=0; i<str_c; i++){
         lens[i] = strlen(str[i]);
     }
     int biggest = 0;
     for (i=0; i<str_c; i++){
         if (lens[i] > biggest) biggest = lens[i];
     }

     return biggest;
}

struct Menu* new_menu(int x, int y, int num_items, char** items, int ITEM_COLOR, int SEL_ITEM_COLOR)
{

    struct Menu* menu = (struct Menu*) malloc(sizeof(struct Menu));

    int width = longest_str(num_items, items)+2;//the 2 is for the > selector...
    int height = num_items;
    menu->win = newwin(height, width, y, x);
    menu->num_items = num_items;
    menu->items = items;
    menu->selected = 0;
    menu->x = x;
    menu->y = y;
    menu->width = width;
    menu->height = height;
    menu->ITEM_COLOR = ITEM_COLOR;
    menu->SEL_ITEM_COLOR = SEL_ITEM_COLOR;

    return menu;
}

//FIX ME!
/*void erase_rect(WINDOW* win, int y, int x, int length, int height)
{
    int i;
    int j;
    for (i=y; i<=(y+height); i++){
        for (j=x; j<(x+length); j++){
            mvwaddch(win, i, j, ' ');
        }
    }
}*/

void move_selected(struct Menu* menu, int direction)
{
    if (direction == DOWN){
        if (++menu->selected > menu->num_items-1) menu->selected = 0;
    } else if (direction == UP){
        if (--menu->selected < 0) menu->selected = menu->num_items-1;
    }
    menu->dirty = 1;
}

void draw_menu(struct Menu* menu)
{

    if (menu->dirty) {
        //erase_rect(menu->win, menu->y, menu->x, menu->width, menu->height);
        menu->dirty = 0;
        wclear(menu->win);
    }

    int i;

    wattron(menu->win, COLOR_PAIR(menu->ITEM_COLOR));
    for (i=0; i<menu->num_items; i++){
        mvwaddstr(menu->win,i,2, menu->items[i]);
    }
    wattroff(menu->win, COLOR_PAIR(menu->ITEM_COLOR));

    wattron(menu->win, COLOR_PAIR(menu->SEL_ITEM_COLOR));
    mvwaddstr(menu->win, menu->selected, 2, menu->items[menu->selected]);
    mvwaddch(menu->win, menu->selected, 0, '>');
    wattroff(menu->win, COLOR_PAIR(menu->SEL_ITEM_COLOR));

}

int poll_menu(struct Menu* menu)
/*Show the menu and animation and return the index of the item chosen.*/
{
    /*TODO: Check for cbreak keypad &c.*/
    int ch;
    while ((ch = getch())){
        draw_menu(menu);
        switch (ch){
            case KEY_UP:
                move_selected(menu, UP);
                break;
            case KEY_DOWN:
                move_selected(menu, DOWN);
                break;
            case KEY_RIGHT:
                return menu->selected;
                break;
            }
        refresh(); //TODO: look into this refresh issues...
        wrefresh(menu->win);
        usleep(125*1000);
    }
    return 0;
}

/*int main(int argc, char** argv)
{
    char* items[3] = {"New Game", "End Game", "Quit"};
    initscr();
    noecho();
    //We want color
    start_color();
    keypad(stdscr, TRUE);
    curs_set(0);
    int BLUE_ON_BLACK = 1;
    int GREEN_ON_BLACK = 2;
    init_pair(BLUE_ON_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(GREEN_ON_BLACK, COLOR_GREEN, COLOR_BLACK);
    nodelay(stdscr, 1);

    struct Menu* menu = new_menu(10, 3, argc, argv, BLUE_ON_BLACK, GREEN_ON_BLACK);
    int x;
    x = poll_menu(menu);
    endwin();
    printf("%d\n", x);
    return 0;
}
*/
