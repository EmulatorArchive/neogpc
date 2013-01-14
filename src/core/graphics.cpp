#include "graphics.h"
#include "memory.h"

#include <cstring>

//unsigned short * drawBuffer;
int displayDirty; // ??

int zoom = 0,zoomy = 0;
int turbo = 0;

unsigned short drawBuffer[NGPC_SIZEX*NGPC_SIZEY];
unsigned short palettes[16*4+16*4+16*4];
int    totalpalette[32*32*32];

// NGP Specific
// Precalculated pattern structures (RACE)
const unsigned char mypatterns[256*4] =
{
   0,0,0,0, 0,0,0,1, 0,0,0,2, 0,0,0,3, 0,0,1,0, 0,0,1,1, 0,0,1,2, 0,0,1,3,
   0,0,2,0, 0,0,2,1, 0,0,2,2, 0,0,2,3, 0,0,3,0, 0,0,3,1, 0,0,3,2, 0,0,3,3,
   0,1,0,0, 0,1,0,1, 0,1,0,2, 0,1,0,3, 0,1,1,0, 0,1,1,1, 0,1,1,2, 0,1,1,3,
   0,1,2,0, 0,1,2,1, 0,1,2,2, 0,1,2,3, 0,1,3,0, 0,1,3,1, 0,1,3,2, 0,1,3,3,
   0,2,0,0, 0,2,0,1, 0,2,0,2, 0,2,0,3, 0,2,1,0, 0,2,1,1, 0,2,1,2, 0,2,1,3,
   0,2,2,0, 0,2,2,1, 0,2,2,2, 0,2,2,3, 0,2,3,0, 0,2,3,1, 0,2,3,2, 0,2,3,3,
   0,3,0,0, 0,3,0,1, 0,3,0,2, 0,3,0,3, 0,3,1,0, 0,3,1,1, 0,3,1,2, 0,3,1,3,
   0,3,2,0, 0,3,2,1, 0,3,2,2, 0,3,2,3, 0,3,3,0, 0,3,3,1, 0,3,3,2, 0,3,3,3,
   1,0,0,0, 1,0,0,1, 1,0,0,2, 1,0,0,3, 1,0,1,0, 1,0,1,1, 1,0,1,2, 1,0,1,3,
   1,0,2,0, 1,0,2,1, 1,0,2,2, 1,0,2,3, 1,0,3,0, 1,0,3,1, 1,0,3,2, 1,0,3,3,
   1,1,0,0, 1,1,0,1, 1,1,0,2, 1,1,0,3, 1,1,1,0, 1,1,1,1, 1,1,1,2, 1,1,1,3,
   1,1,2,0, 1,1,2,1, 1,1,2,2, 1,1,2,3, 1,1,3,0, 1,1,3,1, 1,1,3,2, 1,1,3,3,
   1,2,0,0, 1,2,0,1, 1,2,0,2, 1,2,0,3, 1,2,1,0, 1,2,1,1, 1,2,1,2, 1,2,1,3,
   1,2,2,0, 1,2,2,1, 1,2,2,2, 1,2,2,3, 1,2,3,0, 1,2,3,1, 1,2,3,2, 1,2,3,3,
   1,3,0,0, 1,3,0,1, 1,3,0,2, 1,3,0,3, 1,3,1,0, 1,3,1,1, 1,3,1,2, 1,3,1,3,
   1,3,2,0, 1,3,2,1, 1,3,2,2, 1,3,2,3, 1,3,3,0, 1,3,3,1, 1,3,3,2, 1,3,3,3,
   2,0,0,0, 2,0,0,1, 2,0,0,2, 2,0,0,3, 2,0,1,0, 2,0,1,1, 2,0,1,2, 2,0,1,3,
   2,0,2,0, 2,0,2,1, 2,0,2,2, 2,0,2,3, 2,0,3,0, 2,0,3,1, 2,0,3,2, 2,0,3,3,
   2,1,0,0, 2,1,0,1, 2,1,0,2, 2,1,0,3, 2,1,1,0, 2,1,1,1, 2,1,1,2, 2,1,1,3,
   2,1,2,0, 2,1,2,1, 2,1,2,2, 2,1,2,3, 2,1,3,0, 2,1,3,1, 2,1,3,2, 2,1,3,3,
   2,2,0,0, 2,2,0,1, 2,2,0,2, 2,2,0,3, 2,2,1,0, 2,2,1,1, 2,2,1,2, 2,2,1,3,
   2,2,2,0, 2,2,2,1, 2,2,2,2, 2,2,2,3, 2,2,3,0, 2,2,3,1, 2,2,3,2, 2,2,3,3,
   2,3,0,0, 2,3,0,1, 2,3,0,2, 2,3,0,3, 2,3,1,0, 2,3,1,1, 2,3,1,2, 2,3,1,3,
   2,3,2,0, 2,3,2,1, 2,3,2,2, 2,3,2,3, 2,3,3,0, 2,3,3,1, 2,3,3,2, 2,3,3,3,
   3,0,0,0, 3,0,0,1, 3,0,0,2, 3,0,0,3, 3,0,1,0, 3,0,1,1, 3,0,1,2, 3,0,1,3,
   3,0,2,0, 3,0,2,1, 3,0,2,2, 3,0,2,3, 3,0,3,0, 3,0,3,1, 3,0,3,2, 3,0,3,3,
   3,1,0,0, 3,1,0,1, 3,1,0,2, 3,1,0,3, 3,1,1,0, 3,1,1,1, 3,1,1,2, 3,1,1,3,
   3,1,2,0, 3,1,2,1, 3,1,2,2, 3,1,2,3, 3,1,3,0, 3,1,3,1, 3,1,3,2, 3,1,3,3,
   3,2,0,0, 3,2,0,1, 3,2,0,2, 3,2,0,3, 3,2,1,0, 3,2,1,1, 3,2,1,2, 3,2,1,3,
   3,2,2,0, 3,2,2,1, 3,2,2,2, 3,2,2,3, 3,2,3,0, 3,2,3,1, 3,2,3,2, 3,2,3,3,
   3,3,0,0, 3,3,0,1, 3,3,0,2, 3,3,0,3, 3,3,1,0, 3,3,1,1, 3,3,1,2, 3,3,1,3,
   3,3,2,0, 3,3,2,1, 3,3,2,2, 3,3,2,3, 3,3,3,0, 3,3,3,1, 3,3,3,2, 3,3,3,3,
};

// Standard VRAM Table Addresses
unsigned char * sprite_table = get_address(0x00008800);
unsigned char * pattern_table = get_address(0x0000A000);
unsigned short * patterns = (unsigned short*)pattern_table;
unsigned short * tile_table_front = (unsigned short*)get_address(0x00009000);
unsigned short * tile_table_back = (unsigned short*)get_address(0x00009800);
unsigned short * palette_table = (unsigned short*)get_address(0x00008200);
unsigned char * bw_palette_table = get_address(0x00008100);
unsigned char * sprite_palette_numbers = get_address(0x00008C00);

// VDP Registers
unsigned char * scanlineY = get_address(0x00008009);

// Frame 0/1 priority registers
unsigned char * frame0Pri = get_address(0x00008000);
unsigned char * frame1Pri = get_address(0x00008030);

// Windowing Registers
unsigned char * wndTopLeftX = get_address(0x00008002);
unsigned char * wndTopLeftY = get_address(0x00008003);
unsigned char * wndSizeX = get_address(0x000008004);
unsigned char * wndSizeY = get_address(0x000008005);

// Scrolling registers
unsigned char * scrollSpriteX = get_address(0x00008020);
unsigned char * scrollSpriteY = get_address(0x00008021);
unsigned char * scrollFrontX = get_address(0x00008032);
unsigned char * scrollFrontY = get_address(0x00008033);
unsigned char * scrollBackX = get_address(0x00008034);
unsigned char * scrollBackY = get_address(0x00008035);

// Background color selection table
unsigned char * bgSelect = get_address(0x00008118);
unsigned short * bgTable = (unsigned short*)get_address(0x000083E0);
unsigned char * oowSelect = get_address(0x00008012);
unsigned short * oowTable = (unsigned short*)get_address(0x000083F0);

// machine constants
unsigned char * color_switch = get_address(0x00006F91);


//////////////////////////////////////////////////////////////////////////////
//
// Neogeo Pocket & Neogeo Pocket color rendering
//
//////////////////////////////////////////////////////////////////////////////
static const unsigned short bwTable[8] =
{
   0x0FFF, 0x0DDD, 0x0BBB, 0x0999, 0x0777, 0x0555, 0x0333, 0x0000
};

// used for rendering the sprites

typedef struct
{
    unsigned short offset;    // offset in graphics buffer to blit, Flavor hopes 16bits is good enough
    unsigned short pattern;   // pattern code including palette number
    unsigned short *tilept;   // pointer into the tile description
    unsigned short *palette;   // palette used to render this sprite
}
SPRITE;

typedef struct
{
    unsigned short *gbp;    // (0,x) base for drawing
    unsigned char count[152];
    SPRITE   sprite[152][64];
}
SPRITEDEFS;

SPRITEDEFS spriteDefs[3];    // 4 priority levels

// definitions of types and variables that are used internally during
// rendering of a screen
typedef struct
{
    unsigned short *gbp;     // pointer into graphics buffer
    unsigned char oldScrollX;    // keep an eye on the old and previous values
    unsigned char *newScrollX;   // of the scroll values
    unsigned char oldScrollY;
    unsigned char *newScrollY;
    unsigned short *tileBase;    // start address of the tile table this structure represents
    short   tile[21];    // the tile code
    unsigned short *palettes[21];   // palettes associated with tiles
    unsigned short *tilept[21];   // tile lines to render
    unsigned short *palette;    // palette for the tiles this VSYNC
}
TILECACHE;

TILECACHE  tCBack;      // tile pointer cache for the back buffer
TILECACHE  tCFront;     // tile pointer cache for the front buffer

void palette_init(unsigned int dwRBitMask, unsigned int dwGBitMask, unsigned int dwBBitMask)
{
    char RShiftCount = 0, GShiftCount = 0, BShiftCount = 0;
    char RBitCount = 0, GBitCount = 0, BBitCount = 0;
    int  r,g,b;
    unsigned int i;

    i = dwRBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        RShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        RBitCount++;
    }
    i = dwGBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        GShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        GBitCount++;
    }
    i = dwBBitMask;
    while (!(i&1))
    {
        i = i >> 1;
        BShiftCount++;
    }
    while (i&1)
    {
        i = i >> 1;
        BBitCount++;
    }
   for (b=0; b<16; b++)
      for (g=0; g<16; g++)
            for (r=0; r<16; r++)
               totalpalette[b*256+g*16+r] =
                  (((b<<(BBitCount-4))+(b>>(4-(BBitCount-4))))<<BShiftCount) +
                  (((g<<(GBitCount-4))+(g>>(4-(GBitCount-4))))<<GShiftCount) +
                  (((r<<(RBitCount-4))+(r>>(4-(RBitCount-4))))<<RShiftCount);
}

bool graphics_init()
{
   palette_init(0xf800,0x07c0,0x003e);
    			    
   bgTable  = (unsigned short *)get_address(0x000083E0);
   oowTable  = (unsigned short *)get_address(0x000083F0);

   *scanlineY = 0;
    /*
    switch(m_emuInfo.machine)
    {
        case NGP:
            bgTable  = (unsigned short *)bwTable;
            oowTable = (unsigned short *)bwTable;
            //set_palette = set_paletteBW;
#ifndef __GP32__
            graphicsBlitInit();
#endif
            *scanlineY = 0;
            break;
        case NGPC:
			bgTable  = (unsigned short *)get_address(0x000083E0);
			oowTable  = (unsigned short *)get_address(0x000083F0);
            //set_palette = set_paletteCol;
#ifndef __GP32__
            graphicsBlitInit();
#endif
            *scanlineY = 0;
            break;
    }
    */

   return true;
}

void graphics_blit()
{
}

//
// THOR'S GRAPHIC CORE
//

typedef struct
{
	unsigned char flip;
	unsigned char x;
	unsigned char pal;
} MYSPRITE;

typedef struct
{
	unsigned short tile;
	unsigned char id;
} MYSPRITEREF;

typedef struct
{
	unsigned char count[152];
	MYSPRITEREF refs[152][64];
} MYSPRITEDEF;

MYSPRITEDEF mySprPri40,mySprPri80,mySprPriC0;
MYSPRITE mySprites[64];
unsigned short myPalettes[192];

void sortSprites(unsigned int bw)
{
    unsigned int spriteCode;
    unsigned char x, y, prevx=0, prevy=0;
    unsigned int i, j;
    unsigned short tile;
    MYSPRITEREF *spr;

    // initialize the number of sprites in each structure
    memset(mySprPri40.count, 0, 152);
    memset(mySprPri80.count, 0, 152);
    memset(mySprPriC0.count, 0, 152);

    for (i=0;i<64;i++)
    {
        spriteCode = *((unsigned short *)(sprite_table+4*i));

        prevx = (spriteCode & 0x0400 ? prevx : 0) + *(sprite_table+4*i+2);
        x = prevx + *scrollSpriteX;

        prevy = (spriteCode & 0x0200 ? prevy : 0) + *(sprite_table+4*i+3);
        y = prevy + *scrollSpriteY;

        if ((x>167 && x<249) || (y>151 && y<249) || (spriteCode<=0xff) || ((spriteCode & 0x1800)==0))
            continue;

		mySprites[i].x = x;
		mySprites[i].pal = bw ? ((spriteCode>>11)&0x04) : ((sprite_palette_numbers[i]&0xf)<<2);
		mySprites[i].flip = spriteCode>>8;
		tile = (spriteCode & 0x01ff)<<3;

        for (j = 0; j < 8; ++j,++y)
        {
        	if (y>151)
        		continue;
            switch (spriteCode & 0x1800)
            {
                case 0x1800:
                spr = &mySprPriC0.refs[y][mySprPriC0.count[y]++];
                break;
                case 0x1000:
                spr = &mySprPri80.refs[y][mySprPri80.count[y]++];
                break;
                case 0x0800:
                spr = &mySprPri40.refs[y][mySprPri40.count[y]++];
                break;
                default: continue;
            }
            spr->id = i;
            spr->tile = tile + (spriteCode&0x4000 ? 7-j : j);
        }
    }
}


void drawSprites(unsigned short* draw,
				 MYSPRITEREF *sprites,int count,
				 int x0,int x1)
{
	unsigned short*pal;
	unsigned int pattern,pix,cnt;
	MYSPRITE *spr;
	int i,cx;

	for (i=count-1;i>=0;--i)
	{
		pattern = patterns[sprites[i].tile];
		if (pattern==0)
			continue;

		spr = &mySprites[sprites[i].id];

		if (spr->x>248)
			cx = spr->x-256;
        else
			cx = spr->x;

		if (cx+8<=x0 || cx>=x1)
			continue;

		pal = &myPalettes[spr->pal];

        if (cx<x0)
        {
			cnt = 8-(x0-cx);
			if (spr->flip&0x80)
			{
                pattern>>=((8-cnt)<<1);
                for (cx=x0;pattern;++cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                pattern &= (0xffff>>((8-cnt)<<1));
                for (cx = x0+cnt-1;pattern;--cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
        }
        else if (cx+7<x1)
        {
			if (spr->flip&0x80)
			{
                for (;pattern;++cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                for (cx+=7;pattern;--cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
        }
        else
        {
			cnt = x1-cx;
			if (spr->flip&0x80)
			{
                pattern &= (0xffff>>((8-cnt)<<1));
                for (;pattern;++cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                pattern>>=((8-cnt)<<1);
                for (cx+=cnt-1;pattern;--cx)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[cx] = pal[pix];
                    pattern>>=2;
                }
			}
        }
	}
}


void drawScrollPlane(unsigned short* draw,
					 unsigned short* tile_table,int scrpal,
					 unsigned char dx,unsigned char dy,
					 int x0,int x1,unsigned int bw)
{
	unsigned short*tiles;
	unsigned short*pal;
	unsigned int pattern;
	unsigned int j,count,pix,idy,tile;
	int i,x2;

	dx+=x0;
	tiles = tile_table+((dy>>3)<<5)+(dx>>3);

	count = 8-(dx&0x7);
	dx &= 0xf8;
	dy &= 0x7;
	idy = 7-dy;

	i = x0;

    if (count<8)
    {
		tile = *(tiles++);
		pattern = patterns[(((tile&0x1ff))<<3) + (tile&0x4000 ? idy:dy)];
		if (pattern)
		{
			pal = &myPalettes[scrpal + (bw ? (tile&0x2000 ? 4 : 0) : ((tile>>7)&0x3c))];
			if (tile&0x8000)
			{
                pattern>>=((8-count)<<1);
                for (j=i;pattern;++j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                pattern &= (0xffff>>((8-count)<<1));
                for (j=i+count-1;pattern;--j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
		}
        i+=count;
		dx+=8;
		if (dx==0)
			tiles-=32;
    }

    x2 = i+((x1-i)&0xf8);

	for (;i<x2;i+=8)
	{
		tile = *(tiles++);
		pattern = patterns[(((tile&0x1ff))<<3) + (tile&0x4000 ? idy:dy)];
		if (pattern)
		{
			pal = &myPalettes[scrpal + (bw ? (tile&0x2000 ? 4 : 0) : ((tile>>7)&0x3c))];
			if (tile&0x8000)
			{
                for (j=i;pattern;++j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
                for (j=i+7;pattern;--j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
		}
		dx+=8;
		if (dx==0)
			tiles-=32;
	}

	if (x2!=x1)
	{
        count = x1-x2;
		tile = *(tiles++);
		pattern = patterns[(((tile&0x1ff))<<3) + (tile&0x4000 ? idy:dy)];
		if (pattern)
		{
			pal = &myPalettes[scrpal + (bw ? (tile&0x2000 ? 4 : 0) : ((tile>>7)&0x3c))];
			if (tile&0x8000)
			{
                pattern &= (0xffff>>((8-count)<<1));
                for (j=i;pattern;++j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
			else
			{
			    pattern>>=((8-count)<<1);
                for (j=i+count-1;pattern;--j)
                {
                    pix = pattern & 0x3;
                    if (pix)
                        draw[j] = pal[pix];
                    pattern>>=2;
                }
			}
		}
	}
}

void graphicsBlit(unsigned char render)
{
	int i,x0,x1,k;
    if (*scanlineY < 152)
    {
        if(render)
        {
			unsigned short* draw = &drawBuffer[*scanlineY*NGPC_SIZEX];
			unsigned short bgcol;
            unsigned int bw = false; // (m_emuInfo.machine == NGP);
            unsigned short OOWCol = NGPC_TO_SDL16(oowTable[*oowSelect & 0x07]);
            unsigned short* pal;
            unsigned short* mempal;

			if (*scanlineY==0)
				sortSprites(bw);
			if (*scanlineY<*wndTopLeftY || *scanlineY>*wndTopLeftY+*wndSizeY || *wndSizeX==0 || *wndSizeY==0)
			{
				for (i=0;i<NGPC_SIZEX;i++)
					draw[i] = OOWCol;
			}
			else if (zoom==0 ||( *scanlineY>=zoomy && *scanlineY<zoomy+120))
			{
				if ((turbo==0) || (((*scanlineY)&7)==0))
				{
		            if (bw)
    		        {
        		        for(i=0;i<4;i++)
            		    {
	                	    myPalettes[i]     = NGPC_TO_SDL16(bwTable[bw_palette_table[i]    & 0x07]);
		                    myPalettes[4+i]   = NGPC_TO_SDL16(bwTable[bw_palette_table[4+i]  & 0x07]);
    		                myPalettes[64+i]  = NGPC_TO_SDL16(bwTable[bw_palette_table[8+i]  & 0x07]);
        		            myPalettes[68+i]  = NGPC_TO_SDL16(bwTable[bw_palette_table[12+i] & 0x07]);
            		        myPalettes[128+i] = NGPC_TO_SDL16(bwTable[bw_palette_table[16+i] & 0x07]);
                		    myPalettes[132+i] = NGPC_TO_SDL16(bwTable[bw_palette_table[20+i] & 0x07]);
		                }
    		        }
        		    else
					{
						pal = myPalettes;
						mempal = palette_table;
						for (i=0;i<192;i++)
							*(pal++) = NGPC_TO_SDL16(*(mempal++));
        		    }
				}

	            if(*bgSelect & 0x80)
    	            bgcol = NGPC_TO_SDL16(bgTable[*bgSelect & 0x07]);
        	    else if(bw)
	                bgcol = NGPC_TO_SDL16(bwTable[0]);
    	        else
        	        bgcol = NGPC_TO_SDL16(bgTable[0]);//maybe 0xFFF?

				x0 = *wndTopLeftX;
				x1 = x0+*wndSizeX;
				if (x1>NGPC_SIZEX)
					x1 = NGPC_SIZEX;

				for (i=x0;i<x1;i++)
					draw[i] = bgcol;

				if (mySprPri40.count[*scanlineY])
					drawSprites(draw,mySprPri40.refs[*scanlineY],mySprPri40.count[*scanlineY],x0,x1);

	            if (*frame1Pri & 0x80)
	            {
        			drawScrollPlane(draw,tile_table_front,64,*scrollFrontX,*scrollFrontY+*scanlineY,x0,x1,bw);
	            	if (mySprPri80.count[*scanlineY])
						drawSprites(draw,mySprPri80.refs[*scanlineY],mySprPri80.count[*scanlineY],x0,x1);
		        	drawScrollPlane(draw,tile_table_back,128,*scrollBackX,*scrollBackY+*scanlineY,x0,x1,bw);
	            }
	            else
	            {
		        	drawScrollPlane(draw,tile_table_back,128,*scrollBackX,*scrollBackY+*scanlineY,x0,x1,bw);
					if (mySprPri80.count[*scanlineY])
						drawSprites(draw,mySprPri80.refs[*scanlineY],mySprPri80.count[*scanlineY],x0,x1);
	    	    	drawScrollPlane(draw,tile_table_front,64,*scrollFrontX,*scrollFrontY+*scanlineY,x0,x1,bw);
	            }

				if (mySprPriC0.count[*scanlineY])
					drawSprites(draw,mySprPriC0.refs[*scanlineY],mySprPriC0.count[*scanlineY],x0,x1);

				for (i=0;i<x0;i++)
					draw[i] = OOWCol;
				for (i=x1;i<NGPC_SIZEX;i++)
					draw[i] = OOWCol;
	        }
        }
        if (*scanlineY == 151)
        {
            // start VBlank period
            tlcsMemWriteByte(0x00008010,tlcsMemReadByte(0x00008010) | 0x40);
        }
        *scanlineY+= 1;
    }
    else if (*scanlineY == 198)
    {
        // stop VBlank period
        tlcsMemWriteByte(0x00008010,tlcsMemReadByte(0x00008010) & ~0x40);
        /*
	    if(render)
	    {
	    	unsigned short *buf = getCurrentBuffer();
	    	int dx;
	    	unsigned int pix;
			if (!zoom)
			{
	    		for (i=0,k=0;i<NGPC_SIZEY;i++)
	    		{
	    			dx = OFFSETX*SIZEY+(SIZEY-OFFSETY-i);
		    		for (j=0;j<NGPC_SIZEX;j++,dx+=SIZEY)
		    			buf[dx] = drawBuffer[k++];
	    		}
			}
			else
			{
		    	for (i=0,k=zoom*NGPC_SIZEX;i<NGPC_SIZEY-32;i++)
		    	{
		    		dx = SIZEY-i*2-2;
			    	for (j=0;j<NGPC_SIZEX;j++)
			    	{
		    			pix = drawBuffer[k++];
		    			buf[dx] = buf[dx+1] = pix;
			    		dx+=SIZEY;
			    		buf[dx] = buf[dx+1] = pix;
			    		dx+=SIZEY;
			    	}
		    	}
	    	}
         graphics_paint();
			flip(0);
		}
       */
        *scanlineY = 0;
    }
    else
        *scanlineY+= 1;
}

