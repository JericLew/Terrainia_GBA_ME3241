#include <stdlib.h>
#include <stdbool.h>
#include "sprites.h"
#include "worldmap.h"

#define INPUT (KEY_MASK & (~REG_KEYS))

/*----------Global Variables----------*/
// Game state
#define START_SCREEN 0
#define LEVEL_ONE 1
#define LEVEL_TWO 2
#define END_SCREEN 3
#define DEATH_SCREEN 4
u8 game_state = START_SCREEN;

// start screen
#define HP_SPIRTE_INDEX 15
#define LETTER_SPRITE_INDEX 20

// Tracking Player position and sprite index
#define SPRITE_SIZE 16
#define PLAYERONE_x 120
#define PLAYERONE_y 80
#define PLAYERONE_INDEX 0
#define PLAYERONE_ATTACK_INDEX 1
u8 player_hp = 5;
u8 *player_hp_ptr = &player_hp;
extern void damagePlayer(u8 *player_hp_ptr);

// Enemy position and variable
#define ENEMY_HP 2

float enemy1_x = 224;
float enemy1_y = 112;
float enemy1_x_ms = 0.0;
u8 enemy1_hp = ENEMY_HP;
#define ENEMY1_INDEX 127
#define ENEMY1_SPRITE PLAYERONE

float enemy2_x = 336;
float enemy2_y = 200;
float enemy2_x_ms = 0.5;
u8 enemy2_hp = ENEMY_HP;
#define ENEMY2_INDEX 126
#define ENEMY2_SPRITE PLAYERONE

// background & scrolling variables
#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3
#define FLOATPIXEL 256 //set to 256 2**8 (1 pixel) as REG_BG2X uses 24.8 fixed point format
#define PLAYER_MOVESPEED 1


float map_dx, map_dy; // in pixel, represents screen displacement from background

// jumping & falling
#define TERM_VEL -1.5
#define JUMP_VEL 1.3
#define GRAVITY -0.05 // in pixel/s**2
float y_speed = 0; // upwards positive, in pixels

// Attack & Cooldown
#define ATTACK_CD 4 // 4 ticks cd @ 0.25s/4Hz, 1s
u8 attack_cd_timer = 0; // 4 ticks cd, 1s
u8 attack_tick = 0; // attack last for 2 ticks@4hz, 0.5s

// Animation
#define IDLE 0
#define RUN 1
#define MATTACK 2
u8 state = 0;
u8 pose = IDLE; // 0 is idle, 1 is run, 2 is Melee attack
u8 player_direction = RIGHT;

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
void fillScreenBlock(u16 *map_ptr)
{
    int i;
    for (i = 0; i < LEN_MAP/2; i++)
        se_mem[i] = (map_ptr[i*2+1] << 8) + map_ptr[i*2];
}

/*----------Level Change Functions----------*/
u16 *map_ptr;

void map_update(void)
{
    if (game_state == LEVEL_ONE)
    {
        map_ptr = lvl1_map;
    }
    if (game_state == LEVEL_TWO)
    {
        map_ptr = lvl2_map;
    }
    fillScreenBlock(map_ptr);
}

void enemy_update(void)
{
    if (game_state == LEVEL_ONE)
    {  
        enemy1_hp = ENEMY_HP;
        enemy2_hp = ENEMY_HP;
        enemy1_x = 200;
        enemy1_y = 112;

        enemy2_x = 336;
        enemy2_y = 200;
    }
    if (game_state == LEVEL_TWO)
    {
        enemy1_hp = ENEMY_HP;
        enemy2_hp = ENEMY_HP;
        enemy1_x = 200;
        enemy1_y = 112;

        enemy2_x = 336;
        enemy2_y = 200;
    }
}

void check_map_change(void)
{
    if (game_state == LEVEL_ONE)
    {
        if (map_dy >= 10*4*8 && map_dx >= 0 && map_dx<= 256)
        {
            game_state = LEVEL_TWO;
            map_update();
            enemy_update();
            map_dx = 0;
            map_dy = 0;
            REG_BG2X = (int)map_dx;
            REG_BG2Y = (int)map_dy;
        }
    }
    // if (game_state == LEVEL_TWO)
    // {
    //     map_ptr = lvl2_map;
    // }
}

/*----------Start Screen Functions----------*/
void drawStartScreen(void)
{
    drawSprite(letter_p,LETTER_SPRITE_INDEX,80+16*0,10);
    drawSprite(letter_r,LETTER_SPRITE_INDEX+1,80+16*1,10);
    drawSprite(letter_e,LETTER_SPRITE_INDEX+2,80+16*2,10);
    drawSprite(letter_s,LETTER_SPRITE_INDEX+3,80+16*3,10);
    drawSprite(letter_s,LETTER_SPRITE_INDEX+4,80+16*4,10);
    drawSprite(letter_s,LETTER_SPRITE_INDEX+5,80+16*0,26);
    drawSprite(letter_t,LETTER_SPRITE_INDEX+6,80+16*1,26);
    drawSprite(letter_a,LETTER_SPRITE_INDEX+7,80+16*2,26);
    drawSprite(letter_r,LETTER_SPRITE_INDEX+8,80+16*3,26);
    drawSprite(letter_t,LETTER_SPRITE_INDEX+9,80+16*4,26);
}

void delStartScreen(void)
{
    delSprite(LETTER_SPRITE_INDEX);
    delSprite(LETTER_SPRITE_INDEX+1);
    delSprite(LETTER_SPRITE_INDEX+2);
    delSprite(LETTER_SPRITE_INDEX+3);
    delSprite(LETTER_SPRITE_INDEX+4);
    delSprite(LETTER_SPRITE_INDEX+5);
    delSprite(LETTER_SPRITE_INDEX+6);
    delSprite(LETTER_SPRITE_INDEX+7);
    delSprite(LETTER_SPRITE_INDEX+8);
    delSprite(LETTER_SPRITE_INDEX+9);
}

void animateStart(void)
{
    if (state == 0)
    {
        drawStartScreen();
        state = 1;
    }
    else
    {
        delStartScreen();
        state = 0;
    }
}

/*----------Collision Functions----------*/

// check if player is colliding with map
// check specifically pixel 4 and pixel 11 for each direction
// and see if next pixel in the direction is a empty tile
bool canPlayerMove(u8 direction, u16 *map_ptr)
{   
    bool bot_check, top_check, left_check, right_check;
    bot_check = map_ptr[(PLAYERONE_x + (int)map_dx + 4)/8 + (PLAYERONE_y+ (int)map_dy + SPRITE_SIZE )/8*64] == 0x00
    && map_ptr[(PLAYERONE_x + (int)map_dx + 11)/8 + (PLAYERONE_y + (int)map_dy + SPRITE_SIZE)/8*64] == 0x00;

    top_check = map_ptr[(PLAYERONE_x + (int)map_dx + 4)/8 + (PLAYERONE_y + (int)map_dy - 1)/8*64] == 0x00
    && map_ptr[(PLAYERONE_x + (int)map_dx + 11)/8 + (PLAYERONE_y + (int)map_dy - 1)/8*64] == 0x00;

    left_check = map_ptr[(PLAYERONE_x + (int)map_dx - 1)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] == 0x00
    && map_ptr[(PLAYERONE_x + (int)map_dx - 1)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] == 0x00;

    right_check = map_ptr[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] == 0x00
    && map_ptr[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] == 0x00;

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

/*----------Moving, Falling & Jumping Functions----------*/
void move(u8 direction)
{   
    switch (direction)
    {
    case RIGHT:
        if (canPlayerMove(RIGHT,map_ptr))
        {
            map_dx += PLAYER_MOVESPEED;
            REG_BG2X  = (int)(map_dx) *FLOATPIXEL;
            pose = RUN;
            player_direction = RIGHT;

            enemy1_x -= PLAYER_MOVESPEED;
            enemy2_x -= PLAYER_MOVESPEED;
        }
        break;
    case LEFT:
        if (canPlayerMove(LEFT,map_ptr))
        {
            map_dx -= PLAYER_MOVESPEED;
            REG_BG2X  = (int)(map_dx)*FLOATPIXEL;
            pose = RUN;
            player_direction = LEFT;

            enemy1_x += PLAYER_MOVESPEED;
            enemy2_x += PLAYER_MOVESPEED;
        }
        break;
    }
}

void jump(void)
{
    if (canPlayerMove(UP,map_ptr) && !canPlayerMove(DOWN,map_ptr)) // if player can move up and player is on the ground, set upward speed
    {
        y_speed = JUMP_VEL; // in pixel/s
    }
}

// function called by VBLANK at ard 60hz to check for falling
void fallcheck(void)
{
    u16 buttons = INPUT;
    bool ground_check;

    // if player is going upwards and head hits smt above, stop upward speed
    if (y_speed > 0 && !canPlayerMove(UP,map_ptr)) 
    {
        y_speed = 0;
    }

    // if player is floating (nothing below) or player is jumping (upward speed)
    if (canPlayerMove(DOWN,map_ptr) || y_speed > 0) 
    {   
        y_speed += GRAVITY ; // add affect of gravity to speed

        // if falling speed is larger than terminal vel of -1.5 pixel/s or if down button is pressed, fall at terminal vel
        if (y_speed< TERM_VEL || (buttons & KEY_DOWN) == KEY_DOWN)
        {
            y_speed = TERM_VEL;
        }

        // check if will hit ground after adding displacement this tick
        ground_check = map_ptr[(PLAYERONE_x + (int)(map_dx) + 4)/8 + (PLAYERONE_y+ (int)(map_dy - y_speed) + SPRITE_SIZE)/8*64] != 0x00
        && map_ptr[(PLAYERONE_x + (int)(map_dx) + 11)/8 + (PLAYERONE_y + (int)(map_dy - y_speed) + SPRITE_SIZE)/8*64] != 0x00;

        // if will collide on next tick, place on top of ground to prevent landing inside ground tile
        map_dy -= y_speed;
        enemy1_y += y_speed;
        enemy2_y += y_speed;
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

/*----------Enemy Functions----------*/

void enemy1Move(u16 tick_counter) // move and draw enemy1
{   
    if (tick_counter%180 == 0)
    {
        enemy1_x_ms *= -1;
    }
    enemy1_x += enemy1_x_ms;
    // if enemy1 out of screen or dead, delSprite
    if (enemy1_x < 0 || enemy1_y < 0 || enemy1_hp == 0)
    {
        delSprite(ENEMY1_INDEX);
    }
    else
    {
        drawSprite(ENEMY1_SPRITE, ENEMY1_INDEX, (int)enemy1_x,(int)enemy1_y);
    }
}

void enemy2Move(u16 tick_counter) // move and draw enemy2
{   
    if (tick_counter%180 == 0)
    {
        enemy2_x_ms *= -1;
    }
    enemy2_x += enemy2_x_ms;

    // if enemy2 out of screen or dead, delSprite
    if (enemy2_x < 0 || enemy2_y < 0 || enemy2_hp == 0)
    {
        delSprite(ENEMY2_INDEX);
    }
    else
    {
        drawSprite(ENEMY2_SPRITE, ENEMY2_INDEX, (int)enemy2_x,(int)enemy2_y);
    }
}

/*----------Attack & Cooldown Functions----------*/

// Check if attack is on CD, if CD decreases timer, if Mattack happened, second Mattack
void cooldown_check(void)
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

/*----------Damage and Health Functions----------*/
u8 iFrameCounter = 0;
#define IMMUNE_DURATION 4 // in 0.25s ticks

void damage_check(void)
{
    // check right attack on enemy
    if (pose == MATTACK && player_direction == RIGHT)
    {
        // if enemy1 is alive and within range
        if (enemy1_hp>0 && (int)enemy1_x >= 120 + SPRITE_SIZE/2 && (int)enemy1_x <= 120 + SPRITE_SIZE*2 && (int)enemy1_y >= 80 - SPRITE_SIZE/2 && (int)enemy1_y <= 80 + SPRITE_SIZE*1.5)
        {
            enemy1_hp -= 1;
        }
        // if enemy2 is alive and within range       
        if (enemy2_hp>0 && (int)enemy2_x >= 120 + SPRITE_SIZE/2 && (int)enemy2_x <= 120 + SPRITE_SIZE*2 && (int)enemy2_y >= 80 - SPRITE_SIZE/2 && (int)enemy2_y <= 80 + SPRITE_SIZE*1.5)
        {
            enemy2_hp -= 1;
        }
    }
    // check left attack on enemy
    if (pose == MATTACK && player_direction == LEFT)
    {
        // if enemy1 is alive and within range
        if (enemy1_hp>0 && (int)enemy1_x <= 120 + SPRITE_SIZE/2 && (int)enemy1_x + SPRITE_SIZE >= 120 - SPRITE_SIZE && (int)enemy1_y >= 80 - SPRITE_SIZE/2 && (int)enemy1_y <= 80 + SPRITE_SIZE*1.5)
        {
            enemy1_hp -= 1;
        }
        // if enemy2 is alive and within range       
        if (enemy2_hp>0 && (int)enemy2_x <= 120 + SPRITE_SIZE/2 && (int)enemy2_x + SPRITE_SIZE >= 120 - SPRITE_SIZE && (int)enemy2_y >= 80 - SPRITE_SIZE/2 && (int)enemy2_y <= 80 + SPRITE_SIZE*1.5)
        {
            enemy2_hp -= 1;
        }
    }
    
    // if iFrame == 0 means not immune
    if (!iFrameCounter)
    {
        // check if enemy1 damages player && enemy1 hp > 0 && player hp>0
        if (!iFrameCounter && player_hp > 0 && enemy1_hp > 0 && enemy1_x + SPRITE_SIZE/2 >= 120 && enemy1_x + SPRITE_SIZE/2 <= 120 + SPRITE_SIZE
        && enemy1_y + SPRITE_SIZE/2 >= 80 && enemy1_y + SPRITE_SIZE/2 <= 80 + SPRITE_SIZE)
        {
            damagePlayer(player_hp_ptr);
            iFrameCounter = IMMUNE_DURATION;
        }
        // check if enemy2 damages player && enemy2 hp > 0 && player hp>0
        if (!iFrameCounter && player_hp > 0 && enemy2_hp > 0 && enemy2_x + SPRITE_SIZE/2 >= 120 && enemy2_x + SPRITE_SIZE/2 <= 120 + SPRITE_SIZE
        && enemy2_y + SPRITE_SIZE/2 >= 80 && enemy2_y + SPRITE_SIZE/2 <= 80 + SPRITE_SIZE)
        {
            damagePlayer(player_hp_ptr);
            iFrameCounter = IMMUNE_DURATION;
        }        
    }
}

void iFrame(void)
{
    if (iFrameCounter != 0)
    {
        iFrameCounter -= 1;
    }
}
void drawHP(void)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        delSprite(HP_SPIRTE_INDEX + i);
    }

    for (i = 0; i < player_hp; i++)
    {
        drawSprite(PLAYERONE,HP_SPIRTE_INDEX + i,0 + i * SPRITE_SIZE,0);
    }
}

/*----------Button Functions----------*/

void buttonR(void)
{
    move(RIGHT);
}

void buttonL(void)
{
    move(LEFT);
}

void buttonU(void)
{
    jump();
}

void buttonA(void)
{
    attack();
}

void buttonS(void)
{
    if (game_state == 0)
    {
        game_state = 1;
        map_update();
        delStartScreen();
    }
}

// checks which button is pressed and calls a function related to button pressed
void checkbutton(void)
{
    u16 buttons = INPUT;

    // Start Button only works when in START, END or DEATH SCREEN
    if (game_state == START_SCREEN || game_state == END_SCREEN || game_state == DEATH_SCREEN)
    {
        if ((buttons & KEY_START) == KEY_START)
        {
            buttonS();
        }
    }

    // all other game buttons work only when in LEVEL ONE or TWO
    if (game_state == LEVEL_ONE || game_state == LEVEL_TWO)
    {
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
}

/*----------Animate Functions----------*/
void animate(void)
{
    if (state && player_direction == RIGHT)
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
    else if(!state && player_direction == RIGHT) // right 2
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

    else if (state && player_direction == LEFT)
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
    else if(!state && player_direction == LEFT)
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

    if (iFrameCounter % 2 == 1)
    {
        delSprite(PLAYERONE_INDEX);
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

