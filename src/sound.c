//      sound.c
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


#include <stdio.h>

#ifndef SOUND_H
#define SOUND_H

enum notes {
    A = 440
    };


void fbeep(int freq, int length)
/*This function grabbed from console_beep V0.1  by Josef Pavlik <jetset@ibm.net>
 *and slightly modified to work without dest*/
{
  printf("\33[10;%d]\33[11;%d]\a\33[10]\33[11]", freq, length);
  //if (length) usleep(length*1000L);
}

#endif
