#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <c64/vic.h>
#include <c64/memmap.h>

#define PLAYFIELD_WIDTH     5
#define PLAYFIELD_HEIGHT    10
#define SCREEN_WIDTH        40
#define CELL_WIDTH          4
#define CELL_HEIGHT         2

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

char playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH];

void draw_cell(char x, char y, char type)
{
    //Set MSB of type to draw inverted

    for (char ix=0; ix<CELL_WIDTH; ix++)
    {
        SCREEN[x+ix+y*SCREEN_WIDTH]= cell_gfx[type][ix]+128*(type<<7);
        SCREEN[x+ix+(y+1)*SCREEN_WIDTH]= cell_gfx[type][ix+CELL_WIDTH]+128*(type<<7);
    }
}

void draw_playfield_row(char x, char y, char row_n)
{
    for (char ix=0; ix<PLAYFIELD_WIDTH; ix++)
        draw_cell(x+(ix*CELL_WIDTH), y, (ix+row_n)%5);
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

            //Walls
            if (iy==0)
                *ch= 0x4F;
            else if (iy==h+1)
                *ch= 0x4B;
            else if (ix==0)
                *ch= 0x49;
            else if (ix==w+1)
                *ch= 0x4D;

            //Corners
            if (ix==0 && iy==0)
                *ch= 0x4E;
            else if (ix==w+1 && iy==0)
                *ch= 0x50;
            else if (ix==0 && iy==h+1)
                *ch= 0x4A;
            else if (ix==w+1 && iy==h+1)
                *ch= 0x4C;
        }
    }
}

void draw_playfield(char x, char y)
{
    draw_playfield_frame(x, y);

    for (char row=0; row<PLAYFIELD_HEIGHT; row++)
        draw_playfield_row(x+1, y+1+row*CELL_HEIGHT, row);
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

    draw_playfield(1,1);

    while (1) ;
}
