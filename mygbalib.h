#include <stdlib.h>
#include <stdbool.h>
#include "sprites.h"
#include "tiles.h"
#include "worldmap.h"

#define INPUT (KEY_MASK & (~REG_KEYS))



// 0 left,1 right, 2 up,3 down


/*----------Global Variables----------*/
// Tracking Player position and sprite index
#define SPRITE_SIZE 16
#define PLAYERONE_x 120
#define PLAYERONE_y 80
#define PLAYERONE_index 127

// background scrolling variables
#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3
#define MOVESPEED 256 //set to 256 2**8 (1 pixel) as REG_BG2X uses 24.8 fixed point format
#define GRAV_RATE 40

int map_dx, map_dy;

// jumping and falling
#define GRAVITY (-MOVESPEED * 2/GRAV_RATE) // 0.050 pixel/s per 0.025s, 2 pixel/s per 1s

u8 falling = 0;
int y_speed = 0;

// animation
u8 state = 0;
u8 pose = 0; // 0 is idle 1 is run
u8 direction = 0; // 0 is right 1 is left

// function called by TIMER1 at 40hz to check for falling
// issue with changing 40hz to #define
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
        falling = 1;
        y_speed += (GRAVITY) ; // change y_speed due to grav

        // if down button is pressed when midair, fall at faster speed
        if ((buttons & KEY_DOWN) == KEY_DOWN) 
        {
            y_speed = -256*(56/GRAV_RATE); // inital jump speed 1.4 pixel per 0.025s, 56 pixels/7 tiles per 1s
        }

        // check if will hit ground after adding displacement this tick
        ground_check = lvl1_map[(PLAYERONE_x + map_dx/256 + 4)/8 + (PLAYERONE_y+ (map_dy - y_speed)/256 + SPRITE_SIZE)/8*64] != 0x00
        && lvl1_map[(PLAYERONE_x + map_dx/256 + 11)/8 + (PLAYERONE_y + (map_dy - y_speed)/256 + SPRITE_SIZE)/8*64] != 0x00;

        // if will collide on next tick, place on ground
        if (ground_check) 
        {
            map_dy = (map_dy - y_speed)/256/8*8*256; // make map_dy multiple of 256*8 (aka tile height)
            REG_BG2Y = map_dy;
            falling = 0;
            y_speed = 0;
        }
        else
        {
            map_dy -= y_speed;
            REG_BG2Y  = map_dy;            
        }
    }
}

/*----------Button Functions----------*/

void buttonR(void)
{
    if (canPlayerMove(RIGHT))
    {
        map_dx += MOVESPEED;
        REG_BG2X  = map_dx;
        pose = 1;
        direction = 0;
    }
}
void buttonL(void)
{
    if (canPlayerMove(LEFT))
    {
        map_dx -= MOVESPEED;
        REG_BG2X  = map_dx;
        pose = 1;
        direction = 1;
    }
}
void buttonU(void)
{
    if (canPlayerMove(UP) && !canPlayerMove(DOWN)) // if player can move up and player is on the ground, set upward speed
    {
        y_speed = 256*1.4; // inital jump speed 1.4 pixel per 0.025s, 56 pixels/7 tiles per 1s
    }
}
// void buttonD(void)
// {
// }

// checks which button is pressed and calls a function related to button pressed
void checkbutton(void)
{
    u16 buttons = INPUT;
    
    if ((buttons & KEY_A) == KEY_A)
    {
        //buttonA();
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
void drawSprite(int numb, int N, int map_dx, int map_dy)
{
    *(unsigned short *)(0x7000000 + 8*N) = map_dy | 0x2000;
    *(unsigned short *)(0x7000002 + 8*N) = map_dx | 0x4000; 
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
    bot_check = lvl1_map[(PLAYERONE_x + map_dx/256 + 4)/8 + (PLAYERONE_y+ map_dy/256 + SPRITE_SIZE )/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + map_dx/256 + 11)/8 + (PLAYERONE_y + map_dy/256 + SPRITE_SIZE)/8*64] == 0x00;

    top_check = lvl1_map[(PLAYERONE_x + map_dx/256 + 4)/8 + (PLAYERONE_y + map_dy/256 - 1)/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + map_dx/256 + 11)/8 + (PLAYERONE_y + map_dy/256 - 1)/8*64] == 0x00;

    left_check = lvl1_map[(PLAYERONE_x + map_dx/256 - 1)/8 + (PLAYERONE_y + map_dy/256 + 4)/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + map_dx/256 - 1)/8 + (PLAYERONE_y + map_dy/256 + 11)/8*64] == 0x00;

    right_check = lvl1_map[(PLAYERONE_x + map_dx/256 + SPRITE_SIZE)/8 + (PLAYERONE_y + map_dy/256 + 4)/8*64] == 0x00
    && lvl1_map[(PLAYERONE_x + map_dx/256 + SPRITE_SIZE)/8 + (PLAYERONE_y + map_dy/256 + 11)/8*64] == 0x00;

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
    if (state && !direction)
    {
        switch (pose)
        {
            case 0:
                drawSprite(RIGHTIDLE1,127,120,80);
                break;
                
            case 1:
                drawSprite(RIGHTRUN1,127,120,80);
                pose = 0;
                break;
        }
        state = 0;
    }
    else if(!state && !direction)
    {
        switch (pose)
        {
            case 0:
                drawSprite(RIGHTIDLE2,127,120,80);
                break;
            case 1:
                drawSprite(RIGHTRUN2,127,120,80);
                pose = 0;
                break;
        }
        state = 1;
    }

        if (state && direction)
    {
        switch (pose)
        {
            case 0:
                drawSprite(LEFTIDLE1,127,120,80);
                break;
                
            case 1:
                drawSprite(LEFTRUN1,127,120,80);
                pose = 0;
                break;
        }
        state = 0;
    }
    else if(!state && direction)
    {
        switch (pose)
        {
            case 0:
                drawSprite(LEFTIDLE2,127,120,80);
                break;
            case 1:
                drawSprite(LEFTRUN2,127,120,80);
                pose = 0;
                break;
        }
        state = 1;
    }
}

//TODO: Add game funcs

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
