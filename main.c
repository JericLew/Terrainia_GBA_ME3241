// -----------------------------------------------------------------------------
// C-Skeleton to be used with HAM Library from www.ngine.de
// -----------------------------------------------------------------------------
#include "mygbalib.h"
u8 animate_counter = 0;


void Handler(void)
{
    REG_IME = 0x00; // Stop all other interrupt handling, while we handle this current one
    
    if ((REG_IF & INT_VBLANK) == INT_VBLANK) // 59.73 Hz roughly 60hz
    {
        fallcheck();
        enemy1Move();
        if (animate_counter == 15)
            {
                animate_counter = 0;
                animate();
            }
        animate_counter += 1;
    }
    
    if ((REG_IF & INT_TIMER0) == INT_TIMER0) // Animation and CD timer, every 0.25s 4hz
    {   
        // animate();          
        cooldown_check();
    }

    if ((REG_IF & INT_BUTTON) == INT_BUTTON)
    {
        checkbutton();
    }


    REG_IF = REG_IF; // Update interrupt table, to confirm we have handled this interrupt
    
    REG_IME = 0x01;  // Re-enable interrupt handling
}


// -----------------------------------------------------------------------------
// Project Entry Point
// -----------------------------------------------------------------------------
int main(void)
{

    // Set Mode 2 (0x2), enable OBJ (0x40 and 0x1000) and BG2
    *(unsigned short *) 0x4000000 = 0x40 | 0x2 | 0x1000 | BG2_ENABLE;

 
    // mode 2 only BG2 and BG3 for rotational/affine
    // set up BG2 for 512x512 using cbb0 and sbb 8
    // bit 15-14 map size 128x128, 256x256, 512x512, 1024x1024 (for mode 2)
    // bit 13 is cont layer
    // bit 12 - 8 address of map data (which SBB)
    // bit 7 pal mode 16 or 256 (always 256 for mode 2)
    // bit 6 mosaic mode
    // bit 5-4 unused
    // bit 3-2 address of tile data (which CBB)
    // bit 1-0 layer priority 00 highest, 11 lowest
    // 1000 1000 1000 0000
    REG_BG2CNT |= 0x8880;
    REG_DISPSTAT |= 0x0008;

    fillBGPal();    // load BGpal
    fillTileMem();  // load tiles into cbb 0
    fillScreenBlock();  // load map into sbb 8

    //sprite stuff//
    fillPalette();  // load sprite pal
    fillSprites();  // load sprites


    // Set Handler Function for interrupts and enable selected interrupts
	REG_IME = 0x0;		// Disable interrupt handling
    REG_INT = (int)&Handler;
    REG_IE |= INT_TIMER0 | INT_TIMER1 | INT_BUTTON | INT_VBLANK; // Enable Timmer and Button interrupts
   
    // Set Timer Mode (fill that section and replace TMX with selected timer number)

    REG_TM0D = 49510; // every 0.25s 4hz
    REG_TM0CNT = TIMER_FREQUENCY_256 | TIMER_INTERRUPTS | TIMER_ENABLE;

    // REG_TM1D = 58958; // every 0.025s 40hz
    // REG_TM1CNT = TIMER_FREQUENCY_64 | TIMER_INTERRUPTS | TIMER_ENABLE;   

    REG_P1CNT |= 0x4000 | KEY_RIGHT | KEY_LEFT | KEY_UP | KEY_DOWN | KEY_A | KEY_B;
	REG_IME = 0x1;		// Enable interrupt handling

    // init map coords
    map_dx = 0;
    map_dy = 0;
    REG_BG2X = map_dx;
    REG_BG2Y = map_dy;
    
    //spawn charac
    drawSprite(RIGHTIDLE1,127,PLAYERONE_x,PLAYERONE_y);
    drawSprite(enemytest,0,enemy1_x,enemy1_y);
    while(1)
    {

    }
	return 0;
}

