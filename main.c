// -----------------------------------------------------------------------------
// C-Skeleton to be used with HAM Library from www.ngine.de
// -----------------------------------------------------------------------------
#include "mygbalib.h"
u16 TICK_COUNTER = 0;

void Handler(void)
{
    REG_IME = 0x00; // Stop all other interrupt handling, while we handle this current one
    
    if ((REG_IF & INT_VBLANK) == INT_VBLANK) // 59.73 Hz roughly 60hz
    {
        checkGameState();

        // only when in start, end or death screen
        if (game_state == START_SCREEN || game_state == END_SCREEN || game_state == DEATH_SCREEN)
        {
            // flash start screen at 2hz
            if (TICK_COUNTER%30 == 0)
            {
                animatePressStart();
            }
            if(game_state == START_SCREEN)
            {
                drawTitle();
            }
            if(game_state == DEATH_SCREEN)
            {
                drawDeath();
                drawSprite(DEATH_POSE_1,PLAYERONE_INDEX,PLAYERONE_x,PLAYERONE_y);
                
            }
            if(game_state == END_SCREEN)
            {
                drawEnd();
                drawSprite(VICTORY_POSE_1,PLAYERONE_INDEX,PLAYERONE_x,PLAYERONE_y);
                drawSprite(VICTORY_POSE_2,PLAYERONE_ATTACK_INDEX,PLAYERONE_x,PLAYERONE_y-SPRITE_SIZE);
            }
        }

        // game functions for level one or two
        if (game_state == LEVEL_ONE || game_state == LEVEL_TWO)
        {   
            checkbutton();
            fallCheck(); // calulate y coords for bg and sprites
            enemy1Move(TICK_COUNTER); // s and draw sprites
            enemy2Move(TICK_COUNTER);
            
            // animate and draws main charac @ 4hz, every 0.25
            if (TICK_COUNTER%15 == 0)
            {
                damageCheck();
                animate();
                cooldownCheck();
                drawHP();
            }
        }

        TICK_COUNTER += 1;
    }
    
    if ((REG_IF & INT_TIMER0) == INT_TIMER0) // iFrame timer, every 0.25s 4hz
    {          
        iFrameCountdown();
    }

    // BUTTON INTERRUPT DOES NOT WORK WELL ON ACTUAL GBA
    // if ((REG_IF & INT_BUTTON) == INT_BUTTON)
    // {
    //     checkbutton();
    // }

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
    fillScreenBlock(lvl1_map); // load map into sbb 8

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

    REG_P1CNT |= 0x4000 | KEY_RIGHT | KEY_LEFT | KEY_UP | KEY_DOWN | KEY_A | KEY_B | KEY_START | KEY_R;
	REG_IME = 0x1;		// Enable interrupt handling

    // init map coords
    map_dx = 0;
    map_dy = 0;
    
    //spawn charac
    drawSprite(RIGHTIDLE1,PLAYERONE_INDEX,PLAYERONE_x,PLAYERONE_y);

    while(1)
    {

    }
	return 0;
}

