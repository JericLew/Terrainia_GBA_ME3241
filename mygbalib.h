#include <stdlib.h>
#include <stdbool.h>
#include "sprites.h"
#include "tiles.h"
#include "worldmap.h"

#define INPUT (KEY_MASK & (~REG_KEYS))

#define sprite_size 16

// Global variable to track the movement of the main character in 
int PLAYERONE_x = 120;
int PLAYERONE_y = 80;
int PLAYERONE_index = 127;
// scrolling
int map_dx, map_dy;
u16 movespeed = 256; //set to 256 2**8 as REG_BG2X uses 24.8 fixed point format


/*----------Button Functions----------*/

void buttonR(void)
{
    map_dx+= movespeed;
    REG_BG2X  = map_dx;
}
void buttonL(void)
{
    map_dx-= movespeed;
    REG_BG2X  = map_dx;
}
void buttonU(void)
{
    map_dy-= movespeed;
    REG_BG2Y  = map_dy;
}
void buttonD(void)
{
    if (canPlayerMove())
    {
    map_dy+= movespeed;
    REG_BG2Y  = map_dy;
    }
}

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
        buttonD();
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
// UNDONE, only for bot left and right and downwards
// finish for top left and top right
bool canPlayerMove(void)
{   
    bool bot_left,bot_right;
    bot_left = lvl1_map[(PLAYERONE_x+map_dx/256)/8 + (PLAYERONE_y + sprite_size + map_dy/256)/8*64] == 0x00;
    bot_right = lvl1_map[(PLAYERONE_x + sprite_size + map_dx/256)/8 + (PLAYERONE_y + sprite_size + map_dy/256)/8*64] == 0x00;
    return (bot_left && bot_right);
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
