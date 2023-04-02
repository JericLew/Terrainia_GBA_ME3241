#include <stdlib.h>
#include <stdbool.h>
#include "sprites.h"
#include "tiles.h"
#include "worldmap.h"

#define INPUT (KEY_MASK & (~REG_KEYS))

extern void damagePlayer(u32 x,u32 y, u32 width, u32 height, u32 color);

/*----------Global Variables----------*/
// Tracking Player position and sprite index
#define SPRITE_SIZE 16
#define PLAYERONE_x 120
#define PLAYERONE_y 80
#define PLAYERONE_INDEX 0
#define PLAYERONE_ATTACK_INDEX 1

float enemy1_x = 200;
float enemy1_y = 80;
#define ENEMY1_INDEX 127
#define ENEMY1_SPRITE PLAYERONE

// background & scrolling variables
#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3
#define FLOATPIXEL 256 //set to 256 2**8 (1 pixel) as REG_BG2X uses 24.8 fixed point format
float map_dx, map_dy; // in pixel, represents screen displacement from background

// jumping & falling
#define GRAVITY -0.05 // in pixel/s**2
float y_speed = 0; // upwards positive, in pixels

// Attack & Cooldown
#define ATTACK_CD 4 // 4 ticks cd, 1s
u8 attack_cd_timer = 0; // 4 ticks cd, 1s
u8 attack_tick = 0; // attack last for 2 ticks@4hz, 0.5s

// Animation
#define IDLE 0
#define RUN 1
#define MATTACK 2
u8 state = 0;
u8 pose = IDLE; // 0 is idle, 1 is run, 2 is Melee attack
u8 direction = RIGHT;

/*----------Falling & Jumping Functions----------*/
void jump(void)
{
    if (canPlayerMove(UP) && !canPlayerMove(DOWN)) // if player can move up and player is on the ground, set upward speed
    {
        y_speed = 1.3; // in pixel/s
    }
}

// function called by VBLANK at ard 60hz to check for falling
void fallcheck(void)
{
    u16 buttons = INPUT;
    bool ground_check;

    // if player is going upwards and head hits smt above, stop upward speed
    if (y_speed > 0 && !canPlayerMove(UP)) 
    {
        y_speed = 0;
    }

    // if player is floating (nothing below) or player is jumping (upward speed)
    if (canPlayerMove(DOWN) || y_speed > 0) 
    {   
        y_speed += GRAVITY ; // add affect of gravity to speed

        // if falling speed is larger than terminal vel of -1.5 pixel/s or if down button is pressed, fall at terminal vel
        if (y_speed<-1.5 ||(buttons & KEY_DOWN) == KEY_DOWN)
        {
            y_speed = -1.5;
        }

        // check if will hit ground after adding displacement this tick
        ground_check = lvl1_map[(PLAYERONE_x + (int)(map_dx) + 4)/8 + (PLAYERONE_y+ (int)(map_dy - y_speed) + SPRITE_SIZE)/8*64] != 0x00
        && lvl1_map[(PLAYERONE_x + (int)(map_dx) + 11)/8 + (PLAYERONE_y + (int)(map_dy - y_speed) + SPRITE_SIZE)/8*64] != 0x00;

        // if will collide on next tick, place on top of ground to prevent landing inside ground tile
        map_dy -= y_speed;
        enemy1_y += y_speed;
        if (ground_check) 
        {
            REG_BG2Y = FLOATPIXEL * ((int)(map_dy)/8*8); // special stuff to ensure it lands on tile
            y_speed = 0;
        }
        else
        {
            REG_BG2Y  = FLOATPIXEL * (int)(map_dy);
        }
    }
}

float enemy1_x_movement = 0.5;

void enemy1Move(u16 tick_counter) // move and draw enemy1
{   
    if (tick_counter%180 == 0)
    {
        enemy1_x_movement *= -1;
    }
    enemy1_x += enemy1_x_movement;
    if (enemy1_x < 0 || enemy1_y < 0)
    {
        delSprite(ENEMY1_INDEX);
    }
    else
    {
        drawSprite(ENEMY1_SPRITE, ENEMY1_INDEX, (int)enemy1_x,(int)enemy1_y);
    }}

/*----------Attack & Cooldown Functions----------*/

// Check if attack is on CD, if CD decreases timer, if Mattack happened, second Mattack
bool cooldown_check(void)
{
    if (attack_cd_timer != 0)
    {
        attack_cd_timer -= 1;
    }
    if (attack_tick)
    {
        pose = MATTACK;
        attack_tick = 0;
        attack_cd_timer = ATTACK_CD;   
    }
}

void attack(void)
{    
    if (attack_cd_timer == 0 && !attack_tick)
    {
        pose = MATTACK;
        attack_tick = 1;
    }
}


/*----------Button Functions----------*/

void buttonR(void)
{
    if (canPlayerMove(RIGHT))
    {
        map_dx += 1;
        REG_BG2X  = (int)(map_dx) *FLOATPIXEL;
        pose = RUN;
        direction = RIGHT;

        enemy1_x -= 1;
    }
}

void buttonL(void)
{
    if (canPlayerMove(LEFT))
    {
        map_dx -= 1;
        REG_BG2X  = (int)(map_dx)*FLOATPIXEL;
        pose = RUN;
        direction = LEFT;

        enemy1_x += 1;
    }
}

void buttonU(void)
{
    jump();
}

void buttonA(void)
{
    attack();
}

// checks which button is pressed and calls a function related to button pressed
void checkbutton(void)
{
    u16 buttons = INPUT;
    
    if ((buttons & KEY_A) == KEY_A)
    {
        buttonA();
    }
    if ((buttons & KEY_B) == KEY_B)
    {
        // buttonB();
    }
    if ((buttons & KEY_SELECT) == KEY_SELECT)
    {
        // buttonSel();
    }
    if ((buttons & KEY_START) == KEY_START)
    {
        // buttonS();
    }
    if ((buttons & KEY_RIGHT) == KEY_RIGHT)
    {
        buttonR();
    }
    if ((buttons & KEY_LEFT) == KEY_LEFT)
    {
        buttonL();
    }
    if ((buttons & KEY_UP) == KEY_UP)
    {
        buttonU();
    }
    if ((buttons & KEY_DOWN) == KEY_DOWN)
    {
        // buttonD();
    }
}

/*----------Sprite Functions----------*/

// draw a specific sprite from sprite data index at specific coords (N is abitary sprite index, 0 to 127)
void drawSprite(int numb, int N, int x_coord, int y_coord)
{
    *(unsigned short *)(0x7000000 + 8*N) = y_coord | 0x2000;
    *(unsigned short *)(0x7000002 + 8*N) = x_coord | 0x4000; 
    *(unsigned short *)(0x7000004 + 8*N) = numb*8;
}

// remove sprite N by replacing with EMPTY outside screen (240,160)
void delSprite(int N)
{
    *(unsigned short *)(0x7000000 + 8*N) = 240 | 0x2000;
    *(unsigned short *)(0x7000002 + 8*N) = 160 | 0x4000;
    *(unsigned short *)(0x7000004 + 8*N) = 0;
	
}

// load sprite palette in GBA memory (custom 256 WEB Safe colour)
void fillPalette(void)
{
    int i;
    for (i = 0; i < LEN_SPRITEPAL; i++)
        spritePal[i] = sprites_palette[i];
}

void fillSprites(void)
{
    int     i;
	// Load all sprites in GBA memory
    for (i = 0; i < 128*16*16; i++)
        spriteData[i] = (sprites[i*2+1] << 8) + sprites[i*2];

	// draw all sprites on screen, but all of them outside screen (240,160)
    for(i = 0; i < 128; i++)
        drawSprite(EMPTY, i, 240, 160);
}

/*----------Background Functions----------*/

// load BG pal in GBA memory (same custom 256 WEB Safe colour as sprites)
void fillBGPal(void)
{
    int i;
    for (i = 0; i < LEN_TILEPAL; i++)
        BGPal[i] = tiles_palette[i];
}

// load BG tiles data into cbb 0
void fillTileMem(void)
{
    int i;
    for (i = 0; i < LEN_TILEDATA; i++)
        tileMem[i] = tiles_data[i];

}

// load map data into sbb 8
// mode 2 requires 8 bit tile index but VRAM only can access with 16/32 bits
void fillScreenBlock(void)
{
    int i;
    for (i = 0; i < LEN_MAP/2; i++)
        se_mem[i] = (lvl1_map[i*2+1] << 8) + lvl1_map[i*2];
}

/*----------Collision Functions----------*/

// check if player is colliding with map
// check specifically pixel 4 and pixel 11 for each direction
// and see if next pixel in the direction is a empty tile
bool canPlayerMove(u8 direction)
{   
    bool bot_check, top_check, left_check, right_check;
    bot_check = lvl1_map[(PLAYERONE_x + (int)map_dx + 4)/8 + (PLAYERONE_y+ (int)map_dy + SPRITE_SIZE )/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + (int)map_dx + 11)/8 + (PLAYERONE_y + (int)map_dy + SPRITE_SIZE)/8*64] == 0x00;

    top_check = lvl1_map[(PLAYERONE_x + (int)map_dx + 4)/8 + (PLAYERONE_y + (int)map_dy - 1)/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + (int)map_dx + 11)/8 + (PLAYERONE_y + (int)map_dy - 1)/8*64] == 0x00;

    left_check = lvl1_map[(PLAYERONE_x + (int)map_dx - 1)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + (int)map_dx - 1)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] == 0x00;

    right_check = lvl1_map[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] == 0x00;

    if (direction == LEFT)
    {
        return (left_check);
    }
    if (direction == RIGHT)
    {
        return (right_check);
    }
    if (direction == UP)
    {
        return (top_check);
    }
    if (direction == DOWN)
    {
        return (bot_check);
    }

    return FALSE;
}

/*----------Animate Functions----------*/
void animate(void)
{
    if (state && direction == RIGHT)
    {
        switch (pose)
        {
            case IDLE:
                drawSprite(RIGHTIDLE1,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                break;
            case RUN:
                drawSprite(RIGHTRUN1,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                pose = IDLE;
                break;
            case MATTACK:
                drawSprite(RIGHTATTACK1,PLAYERONE_INDEX,120,80);
                drawSprite(RIGHTATTACKSWORD1,PLAYERONE_ATTACK_INDEX,120+SPRITE_SIZE,80);
                pose = IDLE;
                break;
        }
        state = 0;
    }
    else if(!state && direction == RIGHT) // right 2
    {
        switch (pose)
        {
            case IDLE:
                drawSprite(RIGHTIDLE2,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                break;
            case RUN:
                drawSprite(RIGHTRUN2,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                pose = IDLE;
                break;
            case MATTACK:
                drawSprite(RIGHTATTACK2,PLAYERONE_INDEX,120,80);
                drawSprite(RIGHTATTACKSWORD2,PLAYERONE_ATTACK_INDEX,120+SPRITE_SIZE,80);
                pose = IDLE;
                break;
        }
        state = 1;
    }

    else if (state && direction == LEFT)
    {
        switch (pose)
        {
            case IDLE:
                drawSprite(LEFTIDLE1,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                break;
            case RUN:
                drawSprite(LEFTRUN1,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                pose = IDLE;
                break;
            case MATTACK:
                drawSprite(LEFTATTACK1,PLAYERONE_INDEX,120,80);
                drawSprite(LEFTATTACKSWORD1,PLAYERONE_ATTACK_INDEX,120-SPRITE_SIZE,80);
                pose = IDLE;
                break;
        }
        state = 0;
    }
    else if(!state && direction == LEFT)
    {
        switch (pose)
        {
            case IDLE:
                drawSprite(LEFTIDLE2,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                break;
            case RUN:
                drawSprite(LEFTRUN2,PLAYERONE_INDEX,120,80);
                delSprite(PLAYERONE_ATTACK_INDEX);
                pose = IDLE;
                break;
            case MATTACK:
                drawSprite(LEFTATTACK2,PLAYERONE_INDEX,120,80);
                drawSprite(LEFTATTACKSWORD2,PLAYERONE_ATTACK_INDEX,120-SPRITE_SIZE,80);
                pose = IDLE;
                break;
        }
        state = 1;
    }
}

// void drawLaser(void)
// {
// 	// Gift function showing you how to draw an example sprite defined in sprite.h on screen, using drawSprite()
// 	// Note that this code uses largeer sprites with a palette, so the main code needs to be initialized in graphical mode 2, using:
//     //		*(unsigned short *) 0x4000000 = 0x40 | 0x2 | 0x1000;
// 	// at the beginning of main() in main.c

//     switch(lPlat) {
//         case 16:
//         {
//             drawSprite(LASER, NPLATS*3 + 5 + NROCK + NMET, LaserX, LaserY);
//             break;
//         }
//         case 9:
//         {
//             drawSprite(LASER, NPLATS*2 + 5 + NROCK + NMET, LaserX, LaserY);
//             break;
//         }
//         default:
//             break;
//     }
// }

