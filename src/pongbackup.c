

//      pong.c
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

//Satisfy GCC
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <curses.h>
#include <sqlite3.h>
#include <glib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#include "sound.h"
#include "timer.h"
#include "fancymenu.h"


#define MAX_STRING_LENGTH 256

#define LIFETIME_MAX 16
#define GRAVITY_MAX 3

//Initial ball velocity
#define EASY_BALL_VX .2
#define EASY_BALL_VY .075
#define HARD_BALL_VX .5
#define HARD_BALL_VY .4

//Amount ball velocity increases per hit
#define EASY_BALL_V_INC .05

//The ai get's to make decisions every 'x' milliseconds
#define EASY_AI_WAIT 10
#define MEDIUM_AI_WAIT 7
#define HARD_AI_WAIT 5

//The speed of the game...
#define TICKS 8 //10

#define INFO_WIN_HEIGHT 3
#define LOWEST_XRES 40
#define LOWEST_YRES 12

#define SOUND_ON 1
#define SOUND_OFF 0

#define PADDLE_SENSITIVITY 2

#define MAX_SCORE 50
#define DEFAULT_SCORE 10

#define PACKET_SIZE 1

pthread_mutex_t lock;
int shared_data;

int callback(void *, int, char **, char **);

typedef struct {
	char *name;
	unsigned int score;
} Player;

enum difficulties {
    EASY,
    MEDIUM,
    HARD
};

enum directions {
   UP,
   DOWN,
   LEFT,
   RIGHT
};

enum SPECIALS {
   P_LONG
};

enum colors {
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    BLUE_ON_BLACK,
    RED_ON_BLACK,
    YELLOW_ON_BLACK,
    GREEN_ON_BLACK
};

//Needed to backslash escape the backslashes...
char* game_title[6] = {\
"    ____                                    ______             ",
"   / __ \\____  ____  ____ __   _____  _____/ __/ /___ _      __",
"  / /_/ / __ \\/ __ \\/ __ `/ | / / _ \\/ ___/ /_/ / __ \\ | /| / /",
" / ____/ /_/ / / / / /_/ /| |/ /  __/ /  / __/ / /_/ / |/ |/ / ",
"/_/    \\____/_/ /_/\\__, / |___/\\___/_/  /_/ /_/\\____/|__/|__/  ",
"                  /____/                                       "};

char* options_title[6] = {\
"   ____        __  _                 ",\
"  / __ \\____  / /_(_)___  ____  _____",\
" / / / / __ \\/ __/ / __ \\/ __ \\/ ___/",\
"/ /_/ / /_/ / /_/ / /_/ / / / (__  ) ",\
"\\____/ .___/\\__/_/\\____/_/ /_/____/  ",\
"    /_/                              "};

char* help_title[6] = {\
"    __  __     __    ",\
"   / / / /__  / /___ ",\
"  / /_/ / _ \\/ / __ \\",\
" / __  /  __/ / /_/ /",\
"/_/ /_/\\___/_/ .___/ ",\
"            /_/      "};

char* help_text[6] = {\
"CPONGC is a Curses PONG Clone, designed to help you relive the glory days",\
"of pong on your linux terminal. The controls are very simple, up arrow to",\
"go up up, and down arrow to go down. When playing, your goal is to hit   ",\
"the ball so that your opponent is unable to return it. First to 10 points",\
"wins. Hit p to pause the game, and m to toggle sound on or off in the    ",\
"game. Have Fun!"};

char* scoreboard_title[6] = {\
"   _____                      __                         __",
"  / ___/_________  ________  / /_  ____  ____ __________/ /",
"  \\__ \\/ ___/ __ \\/ ___/ _ \\/ __ \\/ __ \\/ __ `/ ___/ __  / ",
" ___/ / /__/ /_/ / /  /  __/ /_/ / /_/ / /_/ / /  / /_/ /  ",
"/____/\\___/\\____/_/   \\___/_.___/\\____/\\__,_/_/   \\__,_/   "};


int ai_names_c = 8;
char* ai_names[8] = {"Slartibartfast", "Emperor Shaddam IV", "Gordon", "Locklear",\
"Commander Keen", "Cosmo", "xavieran", "Dalek"};


void die_no_memory(){
    printf("No more memory to allocate!\n");
    exit(1);
}

//Much thanks to http://members.cox.net/srice1/random/crandom.html
//Generate a random integer between 0 and h-1
int randint(int h)
{
    double r = rand();
    return (int) (r / (((double) RAND_MAX + (double) 1) / (double) h));
}


int sign(float x)
{
    if (x < 0) return -1;
    if (x == 0) return 0;
    return 1;
}

int my_int(float x)
{
    if ((x - ((int) x)) > .5) return ((int) x) + 1;
    return (int) x;
}

/*++++++++++++++
 BALL STRUCTURE
 ++++++++++++++*/

struct Ball {
    float x; float y; //x, y coordinates
    float px; float py; //the previous position of the ball, used when drawing it
    float vx; float vy; //the x and y velocity of the ball
};

struct Ball* make_ball(float x, float y, float vx, float vy)
{
    struct Ball* p = malloc(sizeof(struct Ball));
    if (p == NULL) die_no_memory();
    p->x = x; p->y = y;
    p->px = x; p->py = y;
    p->vx = vx; p->vy = vy;
    return p;
}

void destroy_ball(struct Ball* ball)
{
    free(ball);
    ball = NULL;
}

char* str_ball(struct Ball* ball)
{
   char* msg = malloc(sizeof(char)*256);
   sprintf(msg, "(x,y):(%f,%f)   (px,py):(%f,%f)  (vx, vy):(%f,%f)",
         ball->x, ball->y, ball->px, ball->py, ball->vx, ball->vy);
   return msg;
}

struct Ball move_ball_f(struct Ball ball)
{
    ball.px = ball.x;
    ball.py = ball.y;
    ball.x += ball.vx;
    ball.y += ball.vy;
    return ball;
}

void move_ball(struct Ball* ball)
{
    *ball = move_ball_f(*ball);
}


struct Ball move_ball_xy_f(struct Ball ball, float x, float y)
{
    ball.px = ball.x;
    ball.py = ball.y;
    ball.x = x;
    ball.y = y;
    return ball;
}


void move_ball_xy(struct Ball* ball, int x, int y)
{
    *ball = move_ball_xy_f(*ball, x, y);
}


/*++++++++++++++
 PADDLE STRUCTURE
 ++++++++++++++*/
struct Paddle {
/* The paddle looks like this:
 * 0,0
 * | x,y
 * |
 * |
 * |
 * | x,y+width (width=5)
 * 0,y
 *
 */
    int x; int y; //x,y coordinates
    int px; int py; //the previous position of paddle
    int width; //the width
    int vel; //the amount it moves
};

struct Paddle* make_paddle(int x, int y, int width, int vel)
{
    struct Paddle* p = malloc(sizeof(struct Paddle));
    if (p == NULL) die_no_memory();
    p->x = x; p->y = y;
    p->px = x; p->py = y;
    p->width = width;
    p->vel = vel;
    return p;
}

void destroy_paddle(struct Paddle* paddle)
{
    free(paddle);
    paddle = NULL;
}

char* str_paddle(struct Paddle* paddle)
{
   char* msg = malloc(sizeof(char)*256);
   sprintf(msg, "(x,y):(%d,%d) (px,py):(%d,%d) width:%d vel:%d",
         paddle->x, paddle->y, paddle->px, paddle->py, paddle->width,
         paddle->vel);
   return msg;
}

struct Paddle move_paddle_dir_f(struct Paddle paddle, int dir)
//Move paddle in the specified direction
{
    switch (dir){
        case UP:
            paddle.py = paddle.y;
            paddle.y -= paddle.vel;
            break;
        case DOWN:
            paddle.py = paddle.y;
            paddle.y += paddle.vel;
            break;
    }
    return paddle;
}

void move_paddle_dir(struct Paddle* paddle, int dir)
//Move paddle in the specified direction
{
    *paddle = move_paddle_dir_f(*paddle, dir);
}


struct Paddle move_paddle_xy_f(struct Paddle paddle, int x, int y)
{
    paddle.px = paddle.x;
    paddle.py = paddle.y;
    paddle.x = x;
    paddle.y = y;
    return paddle;
}


void move_paddle_xy(struct Paddle* paddle, int x, int y)
{
    *paddle = move_paddle_xy_f(*paddle, x, y);
}


/*++++++++++++++
 GAME STRUCTURE
 ++++++++++++++*/
struct Game {
    int max_width;
    int max_height;
    int width;
    int height;
    int difficulty;
    int sound;
    int sensitivity;
    int p1_aictrl; //0 for Human, 1 for AI
    int p2_aictrl; //As above
    int p1_score;
    int p2_score;
    char* p1_name;
    char* p2_name;
    int max_score;
    int paused;
};


struct Game* make_game(int max_width, int max_height, int difficulty, int sound,\
                       int sensitivity, char* p1_name, char* p2_name,\
                       int p1_aictrl, int p2_aictrl, int max_score)
{
    struct Game* p = malloc(sizeof(struct Game));
    if (p == NULL) die_no_memory();
    p->max_width = max_width;
    p->max_height = max_height;
    p->width = max_width;
    p->height = max_height - INFO_WIN_HEIGHT - 1;
    p->difficulty = difficulty;
    p->sound = sound;
    p->sensitivity = sensitivity;
    p->p1_aictrl = p1_aictrl;
    p->p2_aictrl = p2_aictrl;
    p->p1_score = 0;
    p->p2_score = 0;
    p->p1_name = p1_name;
    p->p2_name = p2_name;
    p->max_score = max_score;
    p->paused = 0;
    return p;
}

void destroy_game(struct Game* game)
{
    free(game);
    game = NULL;
}

char* str_game(struct Game* game)
{
   char* msg = malloc(sizeof(char)*256);
   sprintf(msg, "p1 score:%d  p2 score:%d  (w, h): (%d, %d)",
         game->p1_score, game->p2_score, game->width, game->height);
   return msg;

}

/*Logic*/
/*If a function is suffixed with f, it returns the changes it would make.
 *FIXME!!!!*/




//Shamelessly stolen from: http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect/565282#565282
// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines
// intersect the intersection point may be stored in the floats i_x and i_y.
int lines_intersect(float p0_x, float p0_y, float p1_x, float p1_y,
                    float p2_x, float p2_y, float p3_x, float p3_y)
{
    float s1_x, s1_y, s2_x, s2_y;
    s1_x = p1_x - p0_x;     s1_y = p1_y - p0_y;
    s2_x = p3_x - p2_x;     s2_y = p3_y - p2_y;

    float s, t;
    s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        return 1;
    }

    return 0; // No collision
}

int ball_intersect_paddle(struct Ball* ball, struct Paddle* paddle)
{
    if (lines_intersect(ball->x, ball->y, ball->px, ball->py,
                        paddle->x, paddle->y + paddle->width + 1, //Add one so that we hit the actual bottom of the paddle
                        paddle->x, paddle->y)){
        return 1;
    }
    return 0;
}

//Return 0 if ball within paddle, 1 if ball above paddle, -1 if ball below paddle
int ball_in_paddle(struct Ball* ball, struct Paddle* paddle)
{
    if (ball->y > (paddle->y + paddle->width)){
        return -1;
    } else if (ball->y < paddle->y){
        return 1;
    }

    return 0;
}

//Calculate the distance from the center of the paddle to where the ball has
//struck.
//Return the percentage such that p == 1 means that the ball has hit
//the very edge, and p == 0 means that the ball has hit the center
float collision_dist_prcnt(struct Ball* ball, struct Paddle* paddle)
{
    float paddle_center = (float) paddle->y + ((float) paddle->width / 2.0);
    float dist = abs(paddle_center - ball->y);
    float prcnt = dist / ((float) paddle->width / 2.0);
    return prcnt;
}

//Basically, move the ball and paddle around a bit...
//For a background...
void update_background(struct Ball* ball, struct Paddle* paddle, int max_x, int max_y)
{
    move_ball(ball);
    if (((ball->y + 1) > max_y) || (ball->y < 0)) ball->vy = -ball->vy;
    if ((ball->x > max_x) || (ball->x < 0)) ball->vx = -ball->vx;

    //Move paddle towards ball
    switch (ball_in_paddle(ball, paddle)) {
        case 0:
            break;
        case 1:
            if (ball->vx < 0) move_paddle_dir(paddle , UP);
            break;
        case -1:
            if (ball->vx < 0) move_paddle_dir(paddle, DOWN);
            break;
        }
}
// Drawing Functions:
void initialize_colors()
{
    //We want color
    start_color();
    init_pair(RED, COLOR_RED, COLOR_RED);
    init_pair(GREEN, COLOR_GREEN, COLOR_GREEN);
    init_pair(YELLOW, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLUE);
    init_pair(MAGENTA, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(CYAN, COLOR_CYAN, COLOR_CYAN);
    init_pair(WHITE, COLOR_WHITE, COLOR_WHITE);
    init_pair(BLUE_ON_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
    init_pair(YELLOW_ON_BLACK, COLOR_YELLOW, COLOR_BLACK);
    init_pair(GREEN_ON_BLACK, COLOR_GREEN, COLOR_BLACK);
}

int die()
/*End curses.*/
{
    endwin();
    return 0;
}


//Some drawing functions:

void erase_rect(WINDOW* win, int y, int x, int length, int height)
{
    int i;
    int j;
    for (i=y; i<=(y+height); i++){
        for (j=x; j<(x+length); j++){
            mvwaddch(win, i, j, ' ');
        }
    }
}

void erase_ball(WINDOW* win, struct Ball* ball)
{
    mvwaddch(win, my_int(ball->py), my_int(ball->px), ' ');
}

void draw_ball(WINDOW* win, struct Ball* ball, int color)
{
    mvwaddch(win, my_int(ball->y), my_int(ball->x), 'O' | COLOR_PAIR(color));
}

void erase_draw_ball(WINDOW* win, struct Ball* ball, int color)
{
    erase_ball(win, ball);
    draw_ball(win, ball, color);
}

void erase_paddle(WINDOW* win, struct Paddle* paddle)
{
    erase_rect(win, paddle->py, paddle->px, 1, paddle->width);
}

void draw_paddle(WINDOW* win, struct Paddle* paddle, int color)
{
    int y;
    int bottom = paddle->y + paddle->width;
    wattron(win, COLOR_PAIR(color));
    for (y = paddle->y; y <= bottom; y++){
        mvwaddch(win, y, paddle->x, '|');
    }
    wattroff(win, COLOR_PAIR(color));
}

void erase_draw_paddle(WINDOW* win, struct Paddle* paddle, int color)
{
    //Erase the paddle
    erase_paddle(win, paddle);
    //Now draw it again
    draw_paddle(win, paddle, color);
}

void draw_strings(WINDOW* win, int y, int x, char** str_arr, int arr_c)
{
    int i;
    for (i=0; i < arr_c; i++){
        mvwaddstr(win, i + y, x, str_arr[i]);
    }
}

void draw_background(WINDOW* win, struct Ball* ball, struct Paddle* paddle)
{
    erase_draw_ball(win, ball, RED);
    erase_draw_paddle(win, paddle, BLUE);
}

/* +++++++++++++++ */
/* PLAY THE GAME!!!*/
/* --------------- */


int play_game(struct Game* game)
{
    clear();
    refresh();

    WINDOW* win = newwin(game->height, game->width, INFO_WIN_HEIGHT, 0);
    WINDOW* info_win = newwin(INFO_WIN_HEIGHT, game->width, 0, 0);


    Timer* timer = sgl_timer_new();

    //int ball_vx = EASY_BALL_VX;
    //int ball_vy = EASY_BALL_VY;
    //FIX THIS ^^^
    struct Ball* ball = make_ball(game->width/2, game->height/2, .2, .075);
    struct Paddle* paddle1 = make_paddle(1, game->height/2, 5, game->sensitivity);
    struct Paddle* paddle2 = make_paddle(game->width-2, game->height/2, 5, game->sensitivity);

    //Used for checking where paddles are going to be...
    struct Paddle check_pad;

    int ai_wait = 0;
    int ai_wait_time = 0;
    switch (game->difficulty){
        case 1: ai_wait_time = EASY_AI_WAIT; break;
        case 2: ai_wait_time = MEDIUM_AI_WAIT; break;
        case 3: ai_wait_time = HARD_AI_WAIT; break;
    }

    int key;
    int game_won = 0;
    char* pause_message = "PAUSED";
    //Use this for any printing, etc.
    char* tmp_str = malloc(sizeof(char) * MAX_STRING_LENGTH);


    while ((key = getch()) != 'q'){

        switch (key) {
            case ERR:break;//Ignore this key...
            case 'm':
                if (game->sound) game->sound = 0;
                else game->sound = 1;
                break;

            case 'p':
                if (game->paused){
                     game->paused = 0;
                     erase_rect(win, game->max_height / 3, game->max_width / 2 - strlen(pause_message), strlen(pause_message), 1);
                } else {
                    game->paused = 1;
                    mvwaddstr(win, game->max_height / 3, game->max_width / 2 - strlen(pause_message), pause_message);
                }
                break;

            case 'w':
                if (game->paused || game_won) break;
                if (game->p1_aictrl) break;//The AI is controlling this paddle, so don't let player move it...
                check_pad = move_paddle_dir_f(*paddle1, UP);
                if (check_pad.y + paddle1->vel < 0){
                    move_paddle_xy(paddle1, paddle1->x, game->height - paddle1->width);
                }else { move_paddle_dir(paddle1, UP);
                }
                break;

            case 's':
                if (game->paused || game_won) break;
                if (game->p1_aictrl) break;//The AI is controlling this paddle, so don't let player move it...
                check_pad = move_paddle_dir_f(*paddle1, UP);
                if ((paddle1->y + paddle1->vel + paddle1->width)  > game->height){
                    move_paddle_xy(paddle1, paddle1->x, 0);
                }else { move_paddle_dir(paddle1, DOWN);
                }
                break;

            case KEY_UP:
                if (game->paused || game_won) break;
                if (game->p2_aictrl) break;//The AI is controlling this paddle, so don't let player move it...
                check_pad = move_paddle_dir_f(*paddle2, UP);
                if ((check_pad.y + paddle2->vel) < 0){
                    move_paddle_xy(paddle2, paddle2->x, game->height - paddle2->width);
                }else { move_paddle_dir(paddle2, UP);
                }
                break;

            case KEY_DOWN:
                if (game->paused || game_won) break;
                if (game->p2_aictrl) break;//The AI is controlling this paddle, so don't let player move it...
                check_pad = move_paddle_dir_f(*paddle2, DOWN);
                if (((check_pad.y + check_pad.width) - paddle2->vel)  > game->height){
                    move_paddle_xy(paddle2, paddle2->x, 0);
                }else { move_paddle_dir(paddle2, DOWN);
                }
                break;
            default:
                if (game_won){
                    clear();
                    refresh();
                    return 0;
                }
                break;
        }

        //AI Checks whether it has the ball and then moves
        if (ai_wait > ai_wait_time){
            if (game->p1_aictrl && ball->vx < 0){
                switch (ball_in_paddle(ball, paddle1)) {
                    case 0:
                        break;
                    case 1:
                        move_paddle_dir(paddle1, UP);
                        break;
                    case -1:
                        move_paddle_dir(paddle1, DOWN);
                        break;
                }
            }else if (game->p2_aictrl && ball->vx > 0){
                switch (ball_in_paddle(ball, paddle2)) {
                    case 0:
                        break;
                    case 1:
                        move_paddle_dir(paddle2, UP);
                        break;
                    case -1:
                        move_paddle_dir(paddle2, DOWN);
                        break;
                }
            }
            ai_wait = 0;
        }


        if ((sgl_timer_elapsed_milliseconds(timer) > TICKS) && !(game->paused || game_won)){
            sgl_timer_reset(timer);
            ai_wait++;

            //Ball has collided with paddle1
            if ((ball->x <= paddle1->x) && ball_intersect_paddle(ball, paddle1)){
                if (game->sound) fbeep(660, 10);
                //Move the ball 1 forward from paddle, this paddle is on left
                move_ball_xy(ball, paddle1->x + 1, ball->y);
                //Flip the ball's direction
                ball->vx = -ball->vx;

                float prcnt = collision_dist_prcnt(ball, paddle1);
                float vxi = (1 - prcnt) * EASY_BALL_V_INC;
                float vyi = prcnt * EASY_BALL_V_INC;

                ball->vx = sign(ball->vx) * (fabsf(ball->vx) + vxi);
                ball->vy = sign(ball->vy) * (fabsf(ball->vy) + vyi);

            } else if (ball->x <= paddle1->x){
                if (game->sound) fbeep(550, 20);
                //Increase score...
                game->p2_score++;
                ball->vx = EASY_BALL_VX;
                ball->vy = EASY_BALL_VY;
                move_ball_xy(ball, game->width / 2, game->height / 2);
            }

            //Ball has collided with paddle2
            if ((ball->x >= paddle2->x) && ball_intersect_paddle(ball, paddle2)){
                fbeep(660, 10);
                //Move the ball 1 back from the paddle, this paddle is the one on right...
                move_ball_xy(ball, paddle2->x - 1, ball->y);
                ball->vx = -ball->vx;

                float prcnt = collision_dist_prcnt(ball, paddle2);
                float vxi = (1 - prcnt) * EASY_BALL_V_INC;
                float vyi = prcnt * EASY_BALL_V_INC;

                ball->vx = sign(ball->vx) * (fabsf(ball->vx) + vxi);
                ball->vy = sign(ball->vy) * (fabsf(ball->vy) + vyi);

            } else if (ball->x >= paddle2->x){
                if (game->sound) fbeep(550, 20);
                game->p1_score++;
                ball->vx = -(EASY_BALL_VX);
                ball->vy = EASY_BALL_VY;
                move_ball_xy(ball, game->width / 2, game->height / 2);
            }


            //We need to erase it _now_ because???
            erase_ball(win, ball);

            if ((ball->y + 1 > game->height - 1) || (ball->y < 1)) ball->vy = -ball->vy;
            move_ball(ball);
        }

        if (game->p1_score == game->max_score){
            game_won = 1;
            snprintf(tmp_str, MAX_STRING_LENGTH, "%s Wins!", game->p1_name);
            mvwaddstr(win, game->max_height / 3, game->max_width / 2 - strlen(tmp_str), tmp_str);
        }else if (game->p2_score == game->max_score){
            game_won = 1;
            snprintf(tmp_str, MAX_STRING_LENGTH, "%s Wins!", game->p2_name);
            mvwaddstr(win, game->max_height / 3, game->max_width / 2 - strlen(tmp_str), tmp_str);
        }


        erase_draw_ball(win, ball, RED);
        erase_draw_paddle(win, paddle1, BLUE);
        erase_draw_paddle(win, paddle2, GREEN);

        //Display some info
        box(info_win, 0 , 0);
        box(win, 0, 0);
        snprintf(tmp_str, MAX_STRING_LENGTH, "%s: %d", game->p1_name, game->p1_score);
        mvwaddstr(info_win, 1, 1, tmp_str);
        snprintf(tmp_str, MAX_STRING_LENGTH, "%s: %d", game->p2_name, game->p2_score);
        mvwaddstr(info_win, 1, game->width - strlen(tmp_str) - 1, tmp_str);

        wrefresh(info_win);
        wrefresh(win);
        refresh();
    }

    clear();
    refresh();
    return 0;
}


/*MASSIVE BUNCH OF MENU AND GUI STUFF FOLLOWS*/


struct Game* change_resolution(WINDOW* win, struct Game* game)
{
    clear();
    refresh();

    WINDOW* demo_win = newwin(game->height, game->width, 0, 0);

    int max_xres, max_yres;
    getmaxyx(win, max_yres, max_xres);

    max_yres -= INFO_WIN_HEIGHT + 1;//Why minus 1?

    char* width_str = malloc(sizeof(char) * MAX_STRING_LENGTH);
    char* height_str = malloc(sizeof(char) * MAX_STRING_LENGTH);

    int selected = 0;

    Timer* timer = sgl_timer_new();

    int c;
    while ((c = getch()) != KEY_ENTER)
    {
        switch (c){
            case KEY_RIGHT:
                if (selected == 0){
                    game->width += 5;
                    if (game->width > max_xres) game->width = max_xres;
                } else if (selected == 1){
                    game->height += 5;
                    if (game->height > max_yres) game->height = max_yres;
                } else if (selected == 2){
                    clear();
                    refresh();
                    return game;
                }

                demo_win = newwin(game->height, game->width, 0, 0);
                clear();
                refresh();
                break;

            case KEY_LEFT:
                if (selected == 0){
                    game->width -= 5;
                    if (game->width < LOWEST_XRES) game->width = LOWEST_XRES;
                } else if (selected == 1){
                    game->height -= 5;
                    if (game->height < LOWEST_YRES) game->height = LOWEST_YRES;
                }

                demo_win = newwin(game->height, game->width, 0, 0);
                clear();
                refresh();
                break;

            case KEY_UP:
                if (selected == 0) selected = 2;
                else selected -= 1;
                break;

            case KEY_DOWN:
                if (selected == 2) selected = 0;
                else selected += 1;
                break;
            }

        snprintf(width_str, MAX_STRING_LENGTH, "Width: < %d >", game->width);
        snprintf(height_str, MAX_STRING_LENGTH, "Height: < %d >", game->height);

        wattron(win, COLOR_PAIR(BLUE_ON_BLACK));
        mvwaddstr(win, 1, 1, width_str);
        mvwaddstr(win, 2, 1, height_str);
        mvwaddstr(win, 3, 1, "Back");
        wattroff(win, COLOR_PAIR(BLUE_ON_BLACK));

        wattron(win, COLOR_PAIR(GREEN_ON_BLACK));
        if (selected == 0) mvwaddstr(win, 1, 1, width_str);
        else if (selected == 1) mvwaddstr(win, 2, 1, height_str);
        else if (selected == 2) mvwaddstr(win, 3, 1, "Back");
        wattroff(win, COLOR_PAIR(GREEN_ON_BLACK));


        if (sgl_timer_elapsed_milliseconds(timer) > 20){
            sgl_timer_reset(timer);
            erase_rect(win, 1, 1, strlen(width_str), 2);
            box(demo_win, 0, 0);
            wrefresh(demo_win);
        }
    }

    game->max_height = game->height;
    game->max_width = game->width;

    clear();
    refresh();

    return game;
}

void scoreboard(WINDOW* screen, struct Ball* ball, struct Paddle* paddle, int max_x, int max_y)
{
	sqlite3 *db;
	char *err_msg = 0;

	int rc = sqlite3_open("pong.db3", &db);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(0);
	}

    clear();
    refresh();

	char *sql = "SELECT nick, score FROM scoreboard ORDER BY score DESC";

	GSList* list = NULL, *iterator = NULL;
	// llena la single linked list en el callback
	rc = sqlite3_exec(db, sql, callback, &list, &err_msg);
	if(rc != SQLITE_OK) {
		fprintf(stderr, "Failed to select data\n");
		fprintf(stderr, "SQL error: %s\n", err_msg);
	
		sqlite3_free(err_msg);
		sqlite3_close(db);

	}
	sqlite3_close(db);

	

    Timer* timer = sgl_timer_new();
    move_ball_xy(ball, ball->x, max_y / 2);
    int key, offset=0;

    while ((key = getch()) != 'q'){

		switch (key) {
			case KEY_DOWN:
				offset = offset>=0?++offset:0;
				break;
			case KEY_UP:
				offset = offset>0?--offset:0;
				break;
		}

        //Move and bound ball
        if (sgl_timer_elapsed_milliseconds(timer) > 20){
            sgl_timer_reset(timer);
            update_background(ball, paddle, max_x, max_y - 4);
            draw_background(screen, ball, paddle);
        }

        draw_strings(screen, 1, max_x / 2 - strlen(scoreboard_title[0]), scoreboard_title, 5);

		int count=0;
		for(iterator = list; iterator; iterator = iterator->next){
			char* msg = (char *)malloc(sizeof(char)*MAX_STRING_LENGTH);

			if (count >= offset && count-offset < 20) {
				sprintf(msg, "%5i %25s %15i", count+1,((Player *)iterator->data)->name, ((Player *)iterator->data)->score);
				mvwaddstr(screen, 7+count-offset, 26, msg);
				wmove(screen, 8+count-offset, 0);
				clrtoeol();
			
			}

			free(msg);
			count++;
		}

	    wrefresh(screen);
    }
	g_slist_free(list);
    clear();
}

int callback(void *list, int argc, char **argv, char **azColName) {
	Player *p = (Player*)malloc(sizeof(Player));
	p->name = (char *)malloc(sizeof(char)*MAX_STRING_LENGTH);
	
	strcpy(p->name, argv[0]);
	p->score = atoi(argv[1]);
	
	*((GSList **)list) = g_slist_append(*((GSList **)list), p);

	return 0;
}

void help_menu(WINDOW* screen, struct Ball* ball, struct Paddle* paddle, int max_x, int max_y)
{
    clear();
    refresh();

    Timer* timer = sgl_timer_new();

    move_ball_xy(ball, ball->x, max_y / 2);

    int key;

    while ((key = getch()) != 'q'){

        //Move and bound ball
        if (sgl_timer_elapsed_milliseconds(timer) > 20){
            sgl_timer_reset(timer);
            update_background(ball, paddle, max_x, max_y - 4);
            draw_background(screen, ball, paddle);
        }

        draw_strings(screen, 1, max_x / 2 - strlen(help_title[0]), help_title, 6);
        draw_strings(screen, 8, 3, help_text, 6);

        wrefresh(screen);
    }
    clear();
}


struct Game* options_menu(WINDOW* screen, struct Game* game, struct Ball* ball, struct Paddle* paddle, int max_x, int max_y)
{
    clear();
    refresh();

    move_ball_xy(ball, ball->x, max_y / 2);

    char* opts_menu_l[8] = {"Difficulty", "Sound", "Paddle Sensitivity",\
        "Screen Size", "Left Control", "Right Control",\
        "Play to", "Save & Return"};

    struct Menu* opts_menu = new_menu(max_x / 8, 8, 8, opts_menu_l, BLUE_ON_BLACK, GREEN_ON_BLACK);

    WINDOW* info_win = newwin(3, max_x, max_y - 3, 0);

    Timer* timer = sgl_timer_new();

    char* tmp_str = malloc(sizeof(char) * MAX_STRING_LENGTH);

    int key;

    while (key != 'q'){
        switch (key = getch()){
            case KEY_DOWN:
                move_selected(opts_menu, DOWN);
                wclear(info_win);
                break;

            case KEY_UP:
                wclear(info_win);
                move_selected(opts_menu, UP);
                break;

            case KEY_LEFT:
                switch (opts_menu->selected){
                    case 0: //Difficulty
                        if (game->difficulty == 0) game->difficulty = 2;
                        else game->difficulty--;
                        break;
                    case 1: //Sound
                        if (game->sound) game->sound = 0;
                        else game->sound = 1;
                        break;
                    case 2: //Paddle sensitivity
                        if (game->sensitivity == 1) game->sensitivity = 4;//DEHARDCODE THE 4
                        else game->sensitivity--;
                        break;
                    case 4: //Left control selection...
                        game->p1_aictrl = (game->p1_aictrl ? 0 : 1); break;
                    case 5: //Right control selection
                        game->p2_aictrl = (game->p2_aictrl ? 0 : 1); break;
                    case 6: //Change maximum score...
                        game->max_score--;
                        if (game->max_score < 1) game->max_score = MAX_SCORE;
                        break;
                }
                break;

            case KEY_RIGHT:
            case KEY_ENTER:
                switch (opts_menu->selected){
                    case 0: //Difficulty
                        if (game->difficulty == 2) game->difficulty = 0;
                        else game->difficulty++;
                        break;
                    case 1: //Sound
                        if (game->sound) game->sound = 0;
                        else game->sound = 1;
                        break;
                    case 2: //Paddle sensitivity
                        if (game->sensitivity == 4) game->sensitivity = 1;
                        else game->sensitivity++;
                        break;
                    case 3: //Change Resolution
                        change_resolution(screen, game);
                        break;
                    case 4: //Left control selection...
                        if (game->p1_aictrl) game->p1_aictrl = 0;
                        else game->p1_aictrl = 1;
                        break;
                    case 5: //Right control selection
                        if (game->p2_aictrl) game->p2_aictrl = 0;
                        else game->p2_aictrl = 1;
                        break;
                    case 6: //Change maximum score...
                        game->max_score++;
                        if (game->max_score > MAX_SCORE) game->max_score = 1;
                        break;
                    case 7: //Quit
                        return game;
                        break;
                }
                break;
        }

        //Move and bound ball
        if (sgl_timer_elapsed_milliseconds(timer) > 20){
            sgl_timer_reset(timer);
            update_background(ball, paddle, max_x, max_y - 4);
            draw_background(screen, ball, paddle);
            box(info_win, 0, 0);
        }

        draw_strings(screen, 1, max_x / 2 - strlen(options_title[0]), options_title, 6);
        draw_menu(opts_menu);

        /************************************************************
         * NOTE: The spaces in strings below are there for a purpose!
         * They erase double digits. eg. "1 " erases "10"
         */
        snprintf(tmp_str, MAX_STRING_LENGTH, "%d ", game->difficulty + 1);//+1 so we don't get a difficulty of '0'
        mvwaddstr(screen, opts_menu->y, opts_menu->x + opts_menu->width + 1, tmp_str);//print difficulty

        mvwaddstr(screen, opts_menu->y + 1, opts_menu->x + opts_menu->width + 1, game->sound ? "On " : "Off");//print sound

        snprintf(tmp_str, MAX_STRING_LENGTH, "%d ", game->sensitivity);
        mvwaddstr(screen, opts_menu->y + 2, opts_menu->x + opts_menu->width + 1, tmp_str);//print sensitivity

        snprintf(tmp_str, MAX_STRING_LENGTH, "%dx%d", game->width, game->height); //Print the resolution
        mvwaddstr(screen, opts_menu->y + 3, opts_menu->x + opts_menu->width + 1, tmp_str);

        mvwaddstr(screen, opts_menu->y + 4, opts_menu->x + opts_menu->width + 1, game->p1_aictrl ? "Computer" : "Human   ");

        mvwaddstr(screen, opts_menu->y + 5, opts_menu->x + opts_menu->width + 1, game->p2_aictrl ? "Computer" : "Human   ");

        snprintf(tmp_str, MAX_STRING_LENGTH, "%d ", game->max_score);
        mvwaddstr(screen, opts_menu->y + 6, opts_menu->x + opts_menu->width + 1, tmp_str);

        //Write a helpful hint in the info_win
        switch (opts_menu->selected){
            case 0:
                mvwaddstr(info_win, 1, 1, "Change the difficulty, higher numbers are more difficult"); break;
            case 1:
                mvwaddstr(info_win, 1, 1, "Toggle sound on or off"); break;
            case 2:
                mvwaddstr(info_win, 1, 1, "Adjust paddle sensitivity, higher numbers are less sensitive"); break;
            case 3:
                mvwaddstr(info_win, 1, 1, "Change the playing screen size"); break;
            case 4:
                mvwaddstr(info_win, 1, 1, "Toggle control between Computer and Human for the left paddle"); break;
            case 5:
                mvwaddstr(info_win, 1, 1, "Toggle control between Computer and Human for the right paddle"); break;
            case 6:
                mvwaddstr(info_win, 1, 1, "Change the maximum score needed to win"); break;
            case 7:
                mvwaddstr(info_win, 1, 1, "Save changes and go back to main menu"); break;
        }

        wrefresh(opts_menu->win);
        wrefresh(info_win);
    }

    clear();
    refresh();
    return game;
}


void *receive_client(void *arg)
{
	int cl = (long)arg;
	char *buf = (char *)malloc(sizeof(char)*PACKET_SIZE);

	read(cl, buf, PACKET_SIZE);

	pthread_mutex_lock(&lock);
	shared_data = buf[0];
	pthread_mutex_unlock(&lock);

	close(cl);
	return NULL;
}


void *init_server(void *arg)
{
	int c;
	int rc;
	int s;
	int val = 1;
	int max_xres, max_yres;
	int cl;

	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	rc = getaddrinfo(NULL, "4444", &hints, &res);
	if(rc)
	{
		return NULL;
	}

	s = socket(res->ai_family, SOCK_STREAM, res->ai_protocol);
	if(s < 0)
	{
		perror("socket");
		return NULL;
	}

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	rc = bind(s, res->ai_addr, res->ai_addrlen);
	
	if(rc < 0)
	{
		perror("connect");
		return NULL;
	}

	rc = listen(s, 2);
	if(rc < 0)
	{
		perror("listen");
		return NULL;
	}

	while(1)
	{
		struct sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		pthread_t cthread;

		cl = accept(s, &addr, &addrlen);
		if(cl < 0)
		{
			perror("accept");
			return NULL;
		}

		pthread_create(&cthread, NULL, receive_client, (void *)(long)cl);
	}

}

int main_menu(WINDOW* screen)
{
    int max_x, max_y;
    getmaxyx(screen, max_y, max_x);

    char* title_menu_l[5] = {"Play!", "Options", "Help", "Quit", "Scoreboard"};
    struct Menu* title_menu = new_menu(max_x / 8, 8, 5, title_menu_l, BLUE_ON_BLACK, GREEN_ON_BLACK);

    //For the moving background...
    struct Ball* ball = make_ball(max_x / 2, max_y / 4, HARD_BALL_VX, HARD_BALL_VY);
    struct Paddle* paddle = make_paddle(0, max_y / 2, 5, 2);

    Timer* timer = sgl_timer_new();

	//Thread initialization
	if(pthread_mutex_init(&lock, NULL) != 0) {
		printf("mutex init failed\n");
	}

	pthread_t thread;
	pthread_create(&thread, NULL, init_server, (void *)NULL);

    //Default settings...
    int difficulty = EASY;
    int sound = SOUND_OFF;

    char* random_name = ai_names[randint(ai_names_c)];
    struct Game* game = make_game(max_x, max_y, difficulty, sound, PADDLE_SENSITIVITY, random_name, getenv("USER"), 1, 0, DEFAULT_SCORE);
    
	int key;
    while (key != 'q'){
		pthread_mutex_lock(&lock);
		if(shared_data == (int)'q') {
			printf("saliendo");
			exit(0);
		}
		pthread_mutex_unlock(&lock);

        switch (key = getch()){
            case KEY_DOWN:
                move_selected(title_menu, DOWN);
                break;
            case KEY_UP:
                move_selected(title_menu, UP);
                break;

            case KEY_RIGHT:
            case KEY_ENTER:
                switch (title_menu->selected){
                    case 0: //Play game
                        play_game(game);
                        break;
                    case 1: //Options
                        game = options_menu(screen, game, ball, paddle, max_x, max_y);
                        clear();
                        break;
                    case 2: //Help
                        help_menu(screen, ball, paddle, max_x, max_y);
                        break;
                    case 3: //Quit
                        return 0;
                        break;
					case 4: //Scoreboard
						scoreboard(screen, ball, paddle, max_x, max_y);
						break;
                }
                break;
        }

        //Render the background
        if (sgl_timer_elapsed_milliseconds(timer) > 20){
            sgl_timer_reset(timer);
            update_background(ball, paddle, max_x, max_y);
            draw_background(screen, ball, paddle);
            refresh();
        }

        draw_strings(screen, 1, max_x / 2 - strlen(game_title[0]), game_title, 6);

        draw_menu(title_menu);
        wrefresh(title_menu->win);
		//aca va el socket creacion y dispatcher, y se vuela el bucle de teclas
		//los nombres de los participantes se puedenmodificar en la estructura game que parece
		//quela voy a tenerque hacer global
		//en el pthread object tengo que verla cantidad de clientes conectados
		//si es unoentonces lodejo mover el teclado con el protocolo
		//sino lo dejoentrar cuando elotro le da play y seteo nombre de segundo jugardor 
    }

    clear();
    refresh();
    return 0;
}

int main(int argc, char** argv)
{
    //Eliminate compiler warning, we're not using these for now...
    (void) argc;
    (void) argv;

    //Initialize rand seed...
    srand(time(NULL));

    //Set the curses stuff up...
    WINDOW* screen = initscr();
    initialize_colors();
    cbreak();
    keypad(stdscr, TRUE); //Allow curses to return Arrow keys, pagedown, home, etc.
    noecho(); //Stop echoing of keypresses
    curs_set(0); //turn cursor off
    nodelay(stdscr, 1);

    main_menu(screen);
    die();
    printf("%d", randint(4));
    return 0;
}
