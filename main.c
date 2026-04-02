#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <c64/vic.h>
#include <c64/memmap.h>
#include <c64/keyboard.h>

#define PLAYFIELD_X         1
#define PLAYFIELD_Y         2
#define PLAYFIELD_WIDTH     5
#define PLAYFIELD_HEIGHT    10
#define SCREEN_WIDTH        40
#define CELL_WIDTH          4
#define CELL_HEIGHT         2
#define SEL_WIDTH           1
#define SEL_HEIGHT          2

#define SCREEN  ((char*)0xC800)
#define FONT    ((char*)0xD000)
#define COLOR   ((char*)0xD800)

const char charset[2048] = {
	#embed "chars.bin"
};

const char cell_gfx[5][8]= {
    { 0x40,0x40,0x40,0x40, 0x40,0x40,0x40,0x40 }, //    //
    { 0x40,0x40,0x41,0x42, 0x41,0x42,0x40,0x40 }, // _/ //
    { 0x43,0x44,0x40,0x40, 0x40,0x40,0x43,0x44 }, // \_ //
    { 0x40,0x40,0x40,0x40, 0x41,0x42,0x43,0x44 }, // /\ //
    //{ 0x40,0x45,0x46,0x40, 0x45,0x40,0x40,0x46 }, // /\ // 45deg vers.
    { 0x43,0x44,0x41,0x42, 0x40,0x40,0x40,0x40 }, // \/ //
    //{ 0x46,0x40,0x40,0x45, 0x40,0x46,0x45,0x40 }, // \/ // 45deg vers.
};

char playfield[PLAYFIELD_HEIGHT+1][PLAYFIELD_WIDTH];
//NOTE: "playfield" is actually one row taller than what it should be,
//        the last row is NOT used for gameplay, it is used for the scroll animation

char sel_x= 0, sel_y=0;
char old_sel_x= 0, old_sel_y=0;

void draw_cell(char x, char y, char type, char vmask)
{
    //Set MSB of type to draw inverted
    //Use vmask to skip drawing certain rows of the cell

    char inverted= type&0x80;

    for (char ix=0; ix<CELL_WIDTH; ix++)
    {
        if (!(vmask&0x80)) SCREEN[x+ix+y*SCREEN_WIDTH]= cell_gfx[type][ix]|inverted;
        if (!(vmask&0x40)) SCREEN[x+ix+(y+1)*SCREEN_WIDTH]= cell_gfx[type][ix+CELL_WIDTH]|inverted;
    }
}

void draw_playfield_row(char x, char y, char row_n)
{
    char mask= 0x00;

    if (row_n == 0)
        mask=  (y <= PLAYFIELD_Y  ? 0x80:0x00) |
               (y+1 <= PLAYFIELD_Y? 0x40:0x00);
    if (row_n == PLAYFIELD_HEIGHT)
        mask=  (y >= PLAYFIELD_Y+PLAYFIELD_HEIGHT*CELL_HEIGHT+1  ? 0x80:0x00) |
               (y+1 >= PLAYFIELD_Y+PLAYFIELD_HEIGHT*CELL_HEIGHT+1? 0x40:0x00);

    for (char ix=0; ix<PLAYFIELD_WIDTH; ix++)
        draw_cell(x+(ix*CELL_WIDTH), y, playfield[row_n][ix], mask);
}

void draw_playfield_frame(char x, char y)
{
    unsigned char w= PLAYFIELD_WIDTH*CELL_WIDTH;
    unsigned char h= PLAYFIELD_HEIGHT*CELL_HEIGHT;

    for (char iy=0; iy<h+2; iy++)
    {
        for (char ix=0; ix<w+2; ix++)
        {
            char* ch= &SCREEN[x+ix+(y+iy)*SCREEN_WIDTH];
            char* color= &COLOR[x+ix+(y+iy)*SCREEN_WIDTH];

            //Walls
            if (iy==0)
                *ch= 0x4F, *color= VCOL_WHITE;
            else if (iy==h+1)
                *ch= 0x4B, *color= VCOL_WHITE;
            else if (ix==0)
                *ch= 0x49, *color= VCOL_WHITE;
            else if (ix==w+1)
                *ch= 0x4D, *color= VCOL_WHITE;
            else
                *color= VCOL_YELLOW;

            //Corners
            if (ix==0 && iy==0)
                *ch= 0x4E, *color= VCOL_WHITE;
            else if (ix==w+1 && iy==0)
                *ch= 0x50, *color= VCOL_WHITE;
            else if (ix==0 && iy==h+1)
                *ch= 0x4A, *color= VCOL_WHITE;
            else if (ix==w+1 && iy==h+1)
                *ch= 0x4C, *color= VCOL_WHITE;
        }
    }
}

void draw_playfield(char vscroll)
{
    char x= PLAYFIELD_X; char y= PLAYFIELD_Y;

    if (!vscroll) draw_playfield_frame(x, y);

    for (char row=0; row<PLAYFIELD_HEIGHT+1; row++)
        draw_playfield_row(x+1, y+1+row*CELL_HEIGHT-vscroll, row);
}

// void playfield_tile_vscroll(char amount)
// {
//     if (amount == 0) return;
//
//     for (int iy=PLAYFIELD_Y+1; iy<PLAYFIELD_Y+1+PLAYFIELD_HEIGHT*CELL_HEIGHT-amount; iy++)
//     {
//         for (int ix=PLAYFIELD_X+1; ix<PLAYFIELD_X+PLAYFIELD_WIDTH*CELL_WIDTH+1; ix++)
//         {
//             if (amount > 0) //Scroll up
//             {
//                 SCREEN[ix+iy*SCREEN_WIDTH]= SCREEN[ix+(iy+amount)*SCREEN_WIDTH];
//             }
//         }
//     }
// }

void playfield_map_vscroll(char amount)
{
    if (amount == 0) return;

    for (int iy=0; iy<PLAYFIELD_HEIGHT-amount+1; iy++)
    {
        for (int ix=0; ix<PLAYFIELD_WIDTH; ix++)
        {
            if (amount > 0) //Scroll up
            {
                playfield[iy][ix]= playfield[iy+amount][ix];
            }
        }
    }
}

void update_sel();
void playfield_newline()
{
    //Generate new cells
    for (int col=0; col<PLAYFIELD_WIDTH; col++)
    {
        playfield[PLAYFIELD_HEIGHT][col]= rand()%5;
    }

    draw_playfield(1);
    vic_waitFrames(2);
    draw_playfield(2);
    playfield_map_vscroll(1);

    update_sel();
}

void update_sel()
{
    //Cleanup old selection
    draw_cell(PLAYFIELD_X+1+old_sel_x*CELL_WIDTH, PLAYFIELD_Y+1+old_sel_y*CELL_HEIGHT, playfield[old_sel_y][old_sel_x], 0x00);
    draw_cell(PLAYFIELD_X+1+old_sel_x*CELL_WIDTH, PLAYFIELD_Y+1+(old_sel_y+1)*CELL_HEIGHT, playfield[old_sel_y+1][old_sel_x], 0x00);
    //Highlight new selection
    draw_cell(PLAYFIELD_X+1+sel_x*CELL_WIDTH, PLAYFIELD_Y+1+sel_y*CELL_HEIGHT, playfield[sel_y][sel_x]|0x80, 0x00);
    draw_cell(PLAYFIELD_X+1+sel_x*CELL_WIDTH, PLAYFIELD_Y+1+(sel_y+1)*CELL_HEIGHT, playfield[sel_y+1][sel_x]|0x80, 0x00);

    old_sel_x= sel_x;
    old_sel_y= sel_y;
}

int main()
{
    mmap_trampoline();

    //Load charset
    mmap_set(MMAP_RAM);
    memcpy(FONT, charset, 2048);
    mmap_set(MMAP_NO_BASIC);

    vic_setmode(VICM_TEXT, SCREEN, FONT);
    memset(SCREEN, 0x40, 1000);

    sel_y= PLAYFIELD_HEIGHT-2;

    draw_playfield(0);
    update_sel();

    //Main game loop
    while (1)
    {
        keyb_poll();

        if (key_pressed(KSCAN_W))
        {
            if (sel_y > 0)
                sel_y--;
            update_sel();
            while (key_pressed(KSCAN_W)) keyb_poll();
        }
        if (key_pressed(KSCAN_A))
        {
            if (sel_x > 0)
                sel_x--;
            update_sel();
            while (key_pressed(KSCAN_A)) keyb_poll();
        }
        if (key_pressed(KSCAN_S))
        {
            if (sel_y < PLAYFIELD_HEIGHT-2)
                sel_y++;
            update_sel();
            while (key_pressed(KSCAN_S)) keyb_poll();
        }
        if (key_pressed(KSCAN_D))
        {
            if (sel_x < PLAYFIELD_WIDTH-1)
                sel_x++;
            update_sel();
            while (key_pressed(KSCAN_D)) keyb_poll();
        }

        if (key_pressed(KSCAN_G))
        {
            playfield_newline();
            while (key_pressed(KSCAN_G)) keyb_poll();
        }

        if (key_pressed(KSCAN_R))
        {
            draw_playfield(0);
            update_sel();
        }

        if (key_pressed(KSCAN_STOP))
            break;

        vic_waitFrames(1);
    }
}
