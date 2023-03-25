#include <stdlib.h>
#include <stdbool.h>
#include "sprites.h"
#include "tiles.h"
#include "worldmap.h"

#define INPUT (KEY_MASK & (~REG_KEYS))




// Global variable to track the movement of the main character in 
int PLAYERONE_x = 120;
int PLAYERONE_y = 80;
int PLAYERONE_index = 127;

int cam_offset_x = 0;
int cam_offset_y = 0;


//TODO: add button funcs
//scrolling
// #define REG_BG2PA      *(u16*)0x4000020
// #define REG_BG2PB      *(u16*)0x4000022
// #define REG_BG2PC      *(u16*)0x4000024
// #define REG_BG2PD      *(u16*)0x4000026
// #define REG_BG2X       *(u32*)0x4000028
// #define REG_BG2Y       *(u32*)0x400002C

int x= 0, y= 0;
    
void buttonR(void)
{
    x+= 10;
    REG_BG2X  = x;
}
void buttonL(void)
{
    x-= 10;
    REG_BG2Y  = x;
}
void buttonU(void)
{
    y-= 10;
    REG_BG2Y  = y;
}
void buttonD(void)
{
    y+= 10;
    REG_BG2Y  = y;
}
void checkbutton(void)
{
	// Gift function to show you how a function that can be called upon button interrupt to detect which button was pressed and run a specific function for each button could look like. You would have to define each buttonA/buttonB/... function yourself.
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

void drawSprite(int numb, int N, int x, int y)
{
	// Same as CA2, make specific sprite (based on its name/numb) appear on screen, as slide number N (each sprite needs a different, arbitrary, N >= 0)
    *(unsigned short *)(0x7000000 + 8*N) = y | 0x2000;
    *(unsigned short *)(0x7000002 + 8*N) = x | 0x4000; 
    *(unsigned short *)(0x7000004 + 8*N) = numb*8;
}

void delSprite(int N) //remove sprite N by replacing with EMPTY outside screen
{
    *(unsigned short *)(0x7000000 + 8*N) = 240 | 0x2000;
    *(unsigned short *)(0x7000002 + 8*N) = 160 | 0x4000;
    *(unsigned short *)(0x7000004 + 8*N) = 0;
	
}


void fillPalette(void)
{
    int i;
    // Fill the palette in GBA memory
    for (i = 0; i < LEN_SPRITEPAL; i++)
        spritePal[i] = sprites_palette[i];
}

void fillSprites(void)
{
    int     i;

	// Load all sprites in GBA memory
    for (i = 0; i < 128*16*16; i++)
        spriteData[i] = (sprites[i*2+1] << 8) + sprites[i*2];

	// draw all sprites on screen, but all of them outside of the screen (starting at position (240,160) the bottom right corner of the GBA screen)
    for(i = 0; i < 128; i++)
        drawSprite(EMPTY, i, 0, 0);
}

// Background Init
//load pal (same custom 256 WEB Safe colour as sprites)
void fillBGPal(void)
{
    int i;
    for (i = 0; i < LEN_TILEPAL; i++)
        BGPal[i] = tiles_palette[i];
}

void fillTileMem(void) // file tile
{
    int i;
    for (i = 0; i < LEN_TILEDATA; i++)
        tileMem[i] = tiles_data[i];

}

void fillScreenBlock(void) // mode 2 requires 8 bit tile index but VRAM only can access with 16/32 bits
{
    int i;
    for (i = 0; i < LEN_MAP/2; i++)
        se_mem[i] = (lvl1_map[i*2+1] << 8) + lvl1_map[i*2];
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
