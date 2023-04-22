#include <stdlib.h>
#include <stdbool.h>
#include "sprites.h"
#include "worldmap.h"

#define INPUT (KEY_MASK & (~REG_KEYS))

/*----------Conventions----------*/
// ANY_CONSTANTS
// any_varible
// anyFunctions

/*----------Global Variables----------*/
// Game state
#define START_SCREEN 0
#define LEVEL_ONE 1
#define LEVEL_TWO 2
#define END_SCREEN 3
#define DEATH_SCREEN 4
u8 game_state = START_SCREEN;

// Sprite index 0-4 for player effects
// Sprite index 5 for relic
// Sprite index 120-127 for enemy
// Sprite index 15-19 for HP
// Sprite index 20-29 for Press Start
// Sprite index 30-?? for Game Screens

// Game screens
#define HP_SPIRTE_INDEX 15
#define PRESS_START_INDEX_START 20
#define GAME_SCREEN_INDEX_START 30

// Level change
u16 *map_ptr;

// Player
#define SPRITE_SIZE 16
#define PLAYERONE_x 120
#define PLAYERONE_y 80
#define PLAYERONE_INDEX 1
#define PLAYERONE_ATTACK_INDEX 2
#define PLAYERONE_EFFECT_INDEX 0
u8 player_hp = 5;
u8 *player_hp_ptr = &player_hp;

// Enemy 
#define ENEMY_HP 2

float enemy1_x;
float enemy1_y;
float enemy1_x_ms;
u8 enemy1_sprite = ENEMY_RIGHT;
u8 enemy1_hp = ENEMY_HP;
#define ENEMY1_INDEX 127

float enemy2_x;
float enemy2_y;
float enemy2_x_ms;
u8 enemy2_hp = ENEMY_HP;
u8 enemy2_sprite = ENEMY_RIGHT;
#define ENEMY2_INDEX 126

float enemy3_x;
float enemy3_y;
float enemy3_x_ms;
u8 enemy3_hp = ENEMY_HP;
u8 enemy3_sprite = ENEMY_RIGHT;
#define ENEMY3_INDEX 125

float enemy4_x;
float enemy4_y;
float enemy4_x_ms;
u8 enemy4_hp = ENEMY_HP;
u8 enemy4_sprite = ENEMY_RIGHT;
#define ENEMY4_INDEX 124

// Relic
float relic_x;
float relic_y;
#define RELIC_X 480
#define RELIC_Y 480
#define RELIC_INDEX 5

// Background & scrolling
#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3
#define FLOATPIXEL 256 //set to 256 2**8 (1 pixel) as REG_BG2X uses 24.8 fixed point format
#define PLAYER_MOVESPEED 1

float map_dx, map_dy; // in pixel, represents screen displacement from background

// Jumping & falling
#define TERM_VEL -1.5
#define JUMP_VEL 1.3
#define GRAVITY -0.05 // in pixel/s**2
float y_speed = 0; // upwards positive, in pixels

// Attack & Cooldown
#define ATTACK_CD 4 // 4 ticks cd @ 0.25s/4Hz, 1s
u8 attack_cd_timer = 0; // 4 ticks cd, 1s
u8 attack_tick = 0; // attack last for 2 ticks@4hz, 0.5s

// Damage
u8 iFrameCounter = 0;
u8 onFire = 0;
#define IMMUNE_DURATION 4 // in 0.25s ticks

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

/*----------Game Screen Functions----------*/
void drawPressStart(void)
{
    drawSprite(LETTER_P,PRESS_START_INDEX_START+0,80+16*0,120);
    drawSprite(LETTER_R,PRESS_START_INDEX_START+1,80+16*1,120);
    drawSprite(LETTER_E,PRESS_START_INDEX_START+2,80+16*2,120);
    drawSprite(LETTER_S,PRESS_START_INDEX_START+3,80+16*3,120);
    drawSprite(LETTER_S,PRESS_START_INDEX_START+4,80+16*4,120);
    drawSprite(LETTER_S,PRESS_START_INDEX_START+5,80+16*0,136);
    drawSprite(LETTER_T,PRESS_START_INDEX_START+6,80+16*1,136);
    drawSprite(LETTER_A,PRESS_START_INDEX_START+7,80+16*2,136);
    drawSprite(LETTER_R,PRESS_START_INDEX_START+8,80+16*3,136);
    drawSprite(LETTER_T,PRESS_START_INDEX_START+9,80+16*4,136);
}

void delPressStart(void)
{
    delSprite(PRESS_START_INDEX_START+0);
    delSprite(PRESS_START_INDEX_START+1);
    delSprite(PRESS_START_INDEX_START+2);
    delSprite(PRESS_START_INDEX_START+3);
    delSprite(PRESS_START_INDEX_START+4);
    delSprite(PRESS_START_INDEX_START+5);
    delSprite(PRESS_START_INDEX_START+6);
    delSprite(PRESS_START_INDEX_START+7);
    delSprite(PRESS_START_INDEX_START+8);
    delSprite(PRESS_START_INDEX_START+9);
}

void animatePressStart(void)
{
    if (state == 0)
    {
        drawPressStart();
        state = 1;
    }
    else
    {
        delPressStart();
        state = 0;
    }
}

void drawTitle(void)
{
    drawSprite(LETTER_T,GAME_SCREEN_INDEX_START+0,56+16*0,10);
    drawSprite(LETTER_E,GAME_SCREEN_INDEX_START+1,56+16*1,10);
    drawSprite(LETTER_R,GAME_SCREEN_INDEX_START+2,56+16*2,10);
    drawSprite(LETTER_R,GAME_SCREEN_INDEX_START+3,56+16*3,10);
    drawSprite(LETTER_A,GAME_SCREEN_INDEX_START+4,56+16*4,10);
    drawSprite(LETTER_I,GAME_SCREEN_INDEX_START+5,56+16*5,10);
    drawSprite(LETTER_N,GAME_SCREEN_INDEX_START+6,56+16*6,10);
    drawSprite(LETTER_A,GAME_SCREEN_INDEX_START+7,56+16*7,10);
}

void drawEnd(void)
{
    drawSprite(LETTER_Y,GAME_SCREEN_INDEX_START+0,64+16*0,10);
    drawSprite(LETTER_O,GAME_SCREEN_INDEX_START+1,64+16*1,10);
    drawSprite(LETTER_U,GAME_SCREEN_INDEX_START+2,64+16*2,10);
    drawSprite(EMPTY   ,GAME_SCREEN_INDEX_START+3,64+16*3,10);
    drawSprite(LETTER_W,GAME_SCREEN_INDEX_START+4,64+16*4,10);
    drawSprite(LETTER_I,GAME_SCREEN_INDEX_START+5,64+16*5,10);
    drawSprite(LETTER_N,GAME_SCREEN_INDEX_START+6,64+16*6,10);
}

void drawDeath(void)
{
    drawSprite(LETTER_Y,GAME_SCREEN_INDEX_START+0,56+16*0,10);
    drawSprite(LETTER_O,GAME_SCREEN_INDEX_START+1,56+16*1,10);
    drawSprite(LETTER_U,GAME_SCREEN_INDEX_START+2,56+16*2,10);
    drawSprite(EMPTY   ,GAME_SCREEN_INDEX_START+3,56+16*3,10);
    drawSprite(LETTER_D,GAME_SCREEN_INDEX_START+4,56+16*4,10);
    drawSprite(LETTER_I,GAME_SCREEN_INDEX_START+5,56+16*5,10);
    drawSprite(LETTER_E,GAME_SCREEN_INDEX_START+6,56+16*6,10);
    drawSprite(LETTER_D,GAME_SCREEN_INDEX_START+7,56+16*7,10);
}

void delGameScreen(void)
{
    delSprite(GAME_SCREEN_INDEX_START+0);
    delSprite(GAME_SCREEN_INDEX_START+1);
    delSprite(GAME_SCREEN_INDEX_START+2);
    delSprite(GAME_SCREEN_INDEX_START+3);
    delSprite(GAME_SCREEN_INDEX_START+4);
    delSprite(GAME_SCREEN_INDEX_START+5);
    delSprite(GAME_SCREEN_INDEX_START+6);
    delSprite(GAME_SCREEN_INDEX_START+7);
}

void updateHP(void)
{

    if (player_hp<5)
    {
        delSprite(HP_SPIRTE_INDEX + player_hp);
    }

}

void initHP(void)
{
    int i;
    for (i = 0; i < player_hp; i++)
    {
        drawSprite(HEART,HP_SPIRTE_INDEX + i,0 + i * SPRITE_SIZE,0);
    }
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

void mapUpdate(void)
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

void enemyUpdate(void)
{
    if (game_state == LEVEL_ONE)
    {  
        enemy1_hp = ENEMY_HP;
        enemy2_hp = ENEMY_HP;
        enemy3_hp = ENEMY_HP;
        enemy4_hp = ENEMY_HP;
        enemy1_x_ms = 0.25;
        enemy2_x_ms = 0.25;
        enemy3_x_ms = 0.5;
        enemy4_x_ms = 0.25;

        enemy1_x = 240;
        enemy1_y = 112;

        enemy2_x = 176;
        enemy2_y = 240;

        enemy3_x = 176;
        enemy3_y = 432;

        enemy4_x = 368;
        enemy4_y = 400;
        
        relic_x = -16;
        relic_y = -16;

    }
    if (game_state == LEVEL_TWO)
    {
        enemy1_hp = ENEMY_HP;
        enemy2_hp = ENEMY_HP;
        enemy3_hp = ENEMY_HP;
        enemy4_hp = ENEMY_HP;
        enemy1_x_ms = 0.5;
        enemy2_x_ms = 0.75;
        enemy3_x_ms = 0.75;
        enemy4_x_ms = 0.75;

        enemy1_x = 304;
        enemy1_y = 104;

        enemy2_x = 224;
        enemy2_y = 272;

        enemy3_x = 168;
        enemy3_y = 328;

        enemy4_x = 232;
        enemy4_y = 456;

        relic_x = RELIC_X;
        relic_y = RELIC_Y;
    }
}

// game state updater
// check for condition to change game state
void checkGameState(void)
{
    u16 buttons = INPUT;

    if (player_hp == 0)
    {
        game_state = DEATH_SCREEN;
    }

    if (game_state == START_SCREEN || game_state == END_SCREEN || game_state == DEATH_SCREEN)
    {
        if ((buttons & KEY_START) == KEY_START)
        {
            game_state = LEVEL_ONE;
            delPressStart();
            delGameScreen();
            mapUpdate();
            enemyUpdate();
            player_hp = 5;
            initHP();
            map_dx = 0;
            map_dy = 0;
            REG_BG2X = (int)map_dx;
            REG_BG2Y = (int)map_dy;
        }
    }
    if (game_state == LEVEL_ONE)
    {
        if (map_dy >= 400 && map_dx >= 328 && map_dx<= 512)
        {
            game_state = LEVEL_TWO;
            mapUpdate();
            enemyUpdate();
            map_dx = 0;
            map_dy = 0;
            REG_BG2X = (int)map_dx;
            REG_BG2Y = (int)map_dy;
        }
    }
    if (game_state == LEVEL_TWO)
    {
        if (map_dy >= 400 && map_dx >= 328 && map_dx<= 512)
        {
            game_state = END_SCREEN;
        }
    }
}

/*----------Collision Functions----------*/

// check if player is colliding with map
// check specifically pixel 4 and pixel 11 for each direction
// and see if next pixel in the direction is a empty tile
bool canPlayerMove(u8 direction, u16 *map_ptr)
{   
    bool bot_check, top_check, left_check, right_check;
    bot_check = map_ptr[(PLAYERONE_x + (int)map_dx + 4)/8 + (PLAYERONE_y+ (int)map_dy + SPRITE_SIZE )/8*64] <= SKY
    && map_ptr[(PLAYERONE_x + (int)map_dx + 11)/8 + (PLAYERONE_y + (int)map_dy + SPRITE_SIZE)/8*64] <= SKY;

    top_check = map_ptr[(PLAYERONE_x + (int)map_dx + 4)/8 + (PLAYERONE_y + (int)map_dy - 1)/8*64] <= SKY
    && map_ptr[(PLAYERONE_x + (int)map_dx + 11)/8 + (PLAYERONE_y + (int)map_dy - 1)/8*64] <= SKY;

    left_check = map_ptr[(PLAYERONE_x + (int)map_dx - 1)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] <= SKY
    && map_ptr[(PLAYERONE_x + (int)map_dx - 1)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] <= SKY;

    right_check = map_ptr[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] <= SKY
    && map_ptr[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] <= SKY;

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
            enemy3_x -= PLAYER_MOVESPEED;
            enemy4_x -= PLAYER_MOVESPEED;
            relic_x  -= PLAYER_MOVESPEED;
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
            enemy3_x += PLAYER_MOVESPEED;
            enemy4_x += PLAYER_MOVESPEED;
            relic_x  += PLAYER_MOVESPEED;
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
void fallCheck(void)
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
        ground_check = map_ptr[(PLAYERONE_x + (int)(map_dx) + 4)/8 + (PLAYERONE_y+ (int)(map_dy - y_speed) + SPRITE_SIZE)/8*64] > SKY
        && map_ptr[(PLAYERONE_x + (int)(map_dx) + 11)/8 + (PLAYERONE_y + (int)(map_dy - y_speed) + SPRITE_SIZE)/8*64] > SKY;

        // if will collide on next tick, place on top of ground to prevent landing inside ground tile
        map_dy -= y_speed;
        enemy1_y += y_speed;
        enemy2_y += y_speed;
        enemy3_y += y_speed;
        enemy4_y += y_speed;
        relic_y  += y_speed;
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

void enemy1Move(u16 tick_counter) // move and draw enemy
{   
    if (tick_counter%180 == 0)
    {
        enemy1_x_ms *= -1;
    }
    if (enemy1_x_ms>0)
    {
        enemy1_sprite = ENEMY_RIGHT;
    }
    else
    {
        enemy1_sprite = ENEMY_LEFT;
    }
    enemy1_x += enemy1_x_ms;
    // if enemy out of screen or dead, delSprite
    if (enemy1_x < 0 || enemy1_y < 0 || enemy1_x > 240 || enemy1_y > 160 || enemy1_hp == 0)
    {
        delSprite(ENEMY1_INDEX);
    }
    else
    {
        drawSprite(enemy1_sprite, ENEMY1_INDEX, (int)enemy1_x,(int)enemy1_y);
    }
}

void enemy2Move(u16 tick_counter)
{   
    if (tick_counter%180 == 0)
    {
        enemy2_x_ms *= -1;
    }
    if (enemy2_x_ms>0)
    {
        enemy2_sprite = ENEMY_RIGHT;
    }
    else
    {
        enemy2_sprite = ENEMY_LEFT;
    }
    enemy2_x += enemy2_x_ms;
    if (enemy2_x < 0 || enemy2_y < 0 || enemy2_x > 240 || enemy2_y > 160 || enemy2_hp == 0)
    {
        delSprite(ENEMY2_INDEX);
    }
    else
    {
        drawSprite(enemy2_sprite, ENEMY2_INDEX, (int)enemy2_x,(int)enemy2_y);
    }
}

void enemy3Move(u16 tick_counter)
{   
    if (tick_counter%180 == 0)
    {
        enemy3_x_ms *= -1;
    }
    if (enemy3_x_ms>0)
    {
        enemy3_sprite = ENEMY_RIGHT;
    }
    else
    {
        enemy3_sprite = ENEMY_LEFT;
    }
    enemy3_x += enemy3_x_ms;
    if (enemy3_x < 0 || enemy3_y < 0 || enemy3_x > 240 || enemy3_y > 160 ||enemy3_hp == 0)
    {
        delSprite(ENEMY3_INDEX);
    }
    else
    {
        drawSprite(enemy3_sprite, ENEMY3_INDEX, (int)enemy3_x,(int)enemy3_y);
    }
}

void enemy4Move(u16 tick_counter)
{   
    if (tick_counter%180 == 0)
    {
        enemy4_x_ms *= -1;
    }
    if (enemy4_x_ms>0)
    {
        enemy4_sprite = ENEMY_RIGHT;
    }
    else
    {
        enemy4_sprite = ENEMY_LEFT;
    }
    enemy4_x += enemy4_x_ms;
    if (enemy4_x < 0 || enemy4_y < 0 || enemy4_x > 240 || enemy4_y > 160 || enemy4_hp == 0)
    {
        delSprite(ENEMY4_INDEX);
    }
    else
    {
        drawSprite(enemy4_sprite, ENEMY4_INDEX, (int)enemy4_x,(int)enemy4_y);
    }
}

void relicMove() // move and draw relic
{   
    if (relic_x < 0 || relic_y < 0 || relic_x > 240 || relic_y > 160)
    {
        delSprite(RELIC_INDEX);
    }
    else
    {
        drawSprite(RELIC, RELIC_INDEX, (int)relic_x,(int)relic_y);
    }
}
/*----------Attack & Cooldown Functions----------*/

// Check if attack is on CD, if CD decreases timer, if Mattack happened, second Mattack
void cooldownCheck(void)
{
    if (attack_cd_timer != 0)
    {
        attack_cd_timer -= 1;
    }

    // do second attack
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

/*----------Damage Functions----------*/
extern void damagePlayer(u8 *player_hp_ptr);

bool isPlayerInLava(u16 *map_ptr)
{   
    bool left_check, right_check;

    left_check = map_ptr[(PLAYERONE_x + (int)map_dx)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] == LVA
    || map_ptr[(PLAYERONE_x + (int)map_dx)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] == LVA;

    right_check = map_ptr[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 4)/8*64] == LVA
    || map_ptr[(PLAYERONE_x + (int)map_dx + SPRITE_SIZE)/8 + (PLAYERONE_y + (int)map_dy + 11)/8*64] == LVA;

    return left_check || right_check;
}

void damageCheck(void)
{ 
    // check right attack on enemy
    if (pose == MATTACK && player_direction == RIGHT)
    {
        // if enemy is alive and within range
        if (enemy1_hp>0 && (int)enemy1_x > 120 + SPRITE_SIZE/2 && (int)enemy1_x < 120 + SPRITE_SIZE*1.75 && (int)enemy1_y + SPRITE_SIZE > 80 && (int)enemy1_y < 80 + SPRITE_SIZE)
        {
            enemy1_hp -= 1;
        }     
        if (enemy2_hp>0 && (int)enemy2_x > 120 + SPRITE_SIZE/2 && (int)enemy2_x < 120 + SPRITE_SIZE*1.75 && (int)enemy2_y + SPRITE_SIZE > 80 && (int)enemy2_y < 80 + SPRITE_SIZE)
        {
            enemy2_hp -= 1;
        }
        if (enemy3_hp>0 && (int)enemy3_x > 120 + SPRITE_SIZE/2 && (int)enemy3_x < 120 + SPRITE_SIZE*1.75 && (int)enemy3_y + SPRITE_SIZE > 80 && (int)enemy3_y < 80 + SPRITE_SIZE)
        {
            enemy3_hp -= 1;
        }
        if (enemy4_hp>0 && (int)enemy4_x > 120 + SPRITE_SIZE/2 && (int)enemy4_x < 120 + SPRITE_SIZE*1.75 && (int)enemy4_y + SPRITE_SIZE > 80 && (int)enemy4_y < 80 + SPRITE_SIZE)
        {
            enemy4_hp -= 1;
        }
    }
    // check left attack on enemy
    if (pose == MATTACK && player_direction == LEFT)
    {
        // if enemy is alive and within range
        if (enemy1_hp>0 && (int)enemy1_x + SPRITE_SIZE < 120 + SPRITE_SIZE/2 && (int)enemy1_x + SPRITE_SIZE > 120 - SPRITE_SIZE*3/4 && (int)enemy1_y + SPRITE_SIZE > 80 && (int)enemy1_y < 80 + SPRITE_SIZE)
        {
            enemy1_hp -= 1;
        }   
        if (enemy2_hp>0 && (int)enemy2_x + SPRITE_SIZE < 120 + SPRITE_SIZE/2 && (int)enemy2_x + SPRITE_SIZE > 120 - SPRITE_SIZE*3/4 && (int)enemy2_y + SPRITE_SIZE > 80 && (int)enemy2_y < 80 + SPRITE_SIZE)
        {
            enemy2_hp -= 1;
        }
        if (enemy3_hp>0 && (int)enemy3_x + SPRITE_SIZE < 120 + SPRITE_SIZE/2 && (int)enemy3_x + SPRITE_SIZE > 120 - SPRITE_SIZE*3/4 && (int)enemy3_y + SPRITE_SIZE > 80 && (int)enemy3_y < 80 + SPRITE_SIZE)
        {
            enemy3_hp -= 1;
        }
        if (enemy4_hp>0 && (int)enemy4_x + SPRITE_SIZE < 120 + SPRITE_SIZE/2 && (int)enemy4_x + SPRITE_SIZE > 120 - SPRITE_SIZE*3/4 && (int)enemy4_y + SPRITE_SIZE > 80 && (int)enemy4_y < 80 + SPRITE_SIZE)
        {
            enemy4_hp -= 1;
        }
    }
    
    // player damage goes here
    // if iFrame == 0 means not immune && player hp > 0
    if (!iFrameCounter &&  player_hp > 0)
    {
        // check if enemy damages player && enemy hp > 0
        if (!iFrameCounter && enemy1_hp > 0 && enemy1_x + SPRITE_SIZE - 3 > 120 && enemy1_x + 3 < 120 + SPRITE_SIZE
        && enemy1_y + SPRITE_SIZE > 80 && enemy1_y + 3 < 80 + SPRITE_SIZE)
        {
            damagePlayer(player_hp_ptr);
            iFrameCounter = IMMUNE_DURATION;
        }
        if (!iFrameCounter && enemy2_hp > 0 && enemy2_x + SPRITE_SIZE - 3 > 120 && enemy2_x + 3 < 120 + SPRITE_SIZE
        && enemy2_y + SPRITE_SIZE > 80 && enemy2_y + 3 < 80 + SPRITE_SIZE)
        {
            damagePlayer(player_hp_ptr);
            iFrameCounter = IMMUNE_DURATION;
        }
        if (!iFrameCounter && enemy3_hp > 0 && enemy3_x + SPRITE_SIZE - 3 > 120 && enemy3_x + 3 < 120 + SPRITE_SIZE
        && enemy3_y + SPRITE_SIZE > 80 && enemy3_y + 3 < 80 + SPRITE_SIZE)
        {
            damagePlayer(player_hp_ptr);
            iFrameCounter = IMMUNE_DURATION;
        }
        if (!iFrameCounter && enemy4_hp > 0 && enemy4_x + SPRITE_SIZE - 3 > 120 && enemy4_x + 3 < 120 + SPRITE_SIZE
        && enemy4_y + SPRITE_SIZE > 80 && enemy4_y + 3 < 80 + SPRITE_SIZE)
        {
            damagePlayer(player_hp_ptr);
            iFrameCounter = IMMUNE_DURATION;
        }
        // check if player is in lava
        if (isPlayerInLava(map_ptr))
        {
            onFire = 1;
            damagePlayer(player_hp_ptr);
            iFrameCounter = IMMUNE_DURATION;
        }
        else
        {
            onFire = 0;
        }
    }
}

void iFrameCountdown(void)
{
    if (iFrameCounter != 0)
    {
        iFrameCounter -= 1;
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

void buttonB(void)
{
    jump();
}
// checks which button is pressed and calls a function related to button pressed
void checkButton(void)
{
    u16 buttons = INPUT;

    if ((buttons & KEY_A) == KEY_A)
    {
        buttonA();
    }
    if ((buttons & KEY_B) == KEY_B)
    {
        buttonB();
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

    if (onFire)
    {
        if (state)
        {
        drawSprite(FIRE_1,PLAYERONE_EFFECT_INDEX,PLAYERONE_x,PLAYERONE_y);            
        }
        else
        {
        drawSprite(FIRE_2,PLAYERONE_EFFECT_INDEX,PLAYERONE_x,PLAYERONE_y);            
        }
    }
    else
    {
        delSprite(PLAYERONE_EFFECT_INDEX);
    }

    if (iFrameCounter % 2 == 1)
    {
        delSprite(PLAYERONE_INDEX);
    }
}
