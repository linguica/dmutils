// gengrab.c

#include "lumpy.h"

typedef struct
{
	byte            width,height;
	byte            data;
} pic_t;

typedef struct
{
	short   width,height;
	short   orgx,orgy;
	byte            data;
} lpic_t;

typedef struct
{
	short   height;
	char    width[256];
	short   charofs[256];
	byte    data;                   // as much as required
} font_t;


typedef struct
{
	byte    topdelta; // pixels from top,a -1 is the last post in the collumn
	byte    length;
// length data bytes follows
} post_t;

typedef struct
{
	short           width;                  // bounding box size
	short           height;
	short           leftoffset;                     // pixels to the left of origin
	short           topoffset;                      // pixels above the origin
	int             collumnofs[256];        // only [width] used, the [0] is &collumnofs[width]
} patch_t;


#define SCRN(x,y)       (*(byteimage+(y)*byteimagewidth+x))


/*
==============
=
= GrabRaw
=
= filename RAW x y width height
=
==============
*/

void GrabRaw (void)
{
	int             x,y,xl,yl,xh,yh,w,h;
	byte            *screen_p;
	int             linedelta;

	GetToken (false);
	xl = atoi (token);
	GetToken (false);
	yl = atoi (token);
	GetToken (false);
	w = atoi (token);
	GetToken (false);
	h = atoi (token);

	xh = xl+w;
	yh = yl+h;

	screen_p = byteimage + yl*byteimagewidth + xl;
	linedelta = byteimagewidth - w;

	for (y=yl ; y<yh ; y++)
	{
		for (x=xl ; x<xh ; x++)
		{
			*lump_p++ = *screen_p;
			*screen_p++ = 0;
		}
		screen_p += linedelta;
	}
}


/*
==============
=
= GrabVRaw
=
= filename VRAW x y width height
=
==============
*/

void GrabVRaw (void)
{
	int             x,y,xl,yl,xh,yh,w,h;
	byte            *screen_p;
	int             linedelta;

	GetToken (false);
	xl = atoi (token);
	GetToken (false);
	yl = atoi (token);
	GetToken (false);
	w = atoi (token);
	GetToken (false);
	h = atoi (token);

	xh = xl+w;
	yh = yl+h;

	screen_p = byteimage + yl*byteimagewidth + xl;
	linedelta = byteimagewidth - w;

	for (x=xl ; x<xh ; x++)
	{
		for (y=yl ; y<yh ; y++)
		{
			*lump_p++ = SCRN(x,y);
			SCRN(x,y) = 0;
		}
	}
}


/*
==============
=
= GrabPic
=
= filename PIC x y width height [orgx orgy [hitxl hityl hitxh hityh]]
=
==============
*/

void GrabPic (void)
{
	int             x,y,xl,yl,w,h,p;
	int             clipplane;
	pic_t   *header;
	byte            *screen_p;

	GetToken (false);
	xl = atoi (token);
	GetToken (false);
	yl = atoi (token);
	GetToken (false);
	w = atoi (token);
	GetToken (false);
	h = atoi (token);

	clipplane = w&3;        // pad with 0 at end of x after this plane
	if (!clipplane)
		clipplane = 4;

	w = (w+3)/4;

	header = (pic_t *)lump_p;
	header->width = w;
	header->height = h;

	lump_p =&header->data;

#if 0
//
// optional origin offset
//
	if (TokenAvailable())
	{
		GetToken (false);
		header->orgx = atoi (token);
		GetToken (false);
		header->orgy = atoi (token);
		lump_p = (byte  *)&header->hitxl;
	//
	// optional hit rectangle
	//
		if (TokenAvailable())
		{
			GetToken (false);
			header->hitxl = atoi (token);
			GetToken (false);
			header->hitxh = atoi (token);
			GetToken (false);
			header->hityl = atoi (token);
			GetToken (false);
			header->hityh = atoi (token);
			lump_p = ((byte  *)&header->hityh)+1;
		}
	}
#endif

//
// grab in munged format
//
	for (p=0 ; p<4 ; p++)
	{

		for (y=0 ; y<h ; y++)
		{
			screen_p = byteimage + (y+yl)*byteimagewidth + xl + p;
			for (x=0 ; x<w ; x++)
			{
				if (x==w-1 && p>=clipplane)
					*lump_p++ = 0;          // pad with transparent
				else
				{
					*lump_p++ = *screen_p;
					*screen_p = 0;
				}
				screen_p += 4;
			}
		}
	}

}


/*
==============
=
= GrabLinearPic
=
= filename LPIC x y width height [orgx orgy]
=
==============
*/

void GrabLinearPic (void)
{
	int             x,y,xl,yl,w,h;
	lpic_t  *header;
	byte            *screen_p;

	GetToken (false);
	xl = atoi (token);
	GetToken (false);
	yl = atoi (token);
	GetToken (false);
	w = atoi (token);
	GetToken (false);
	h = atoi (token);

	header = (lpic_t  *)lump_p;
	header->width = LittleShort(w);
	header->height = LittleShort(h);


//
// optional origin offset
//
	if (TokenAvailable())
	{
		GetToken (false);
		header->orgx = LittleShort (atoi (token));
		GetToken (false);
		header->orgy = LittleShort (atoi (token));
	}
	else
		header->orgx = header->orgy = 0;

	lump_p = (byte  *)&header->data;


//
// grab it
//
	for (y=0 ; y<h ; y++)
	{
		screen_p = byteimage + (y+yl)*byteimagewidth + xl;
		for (x=0 ; x<w ; x++)
		{
			*lump_p++ = *screen_p;
			*screen_p++ = 0xff;
		}
	}

}



/*
==============
=
= GrabPalette
=
= filename PALETTE [startcolor endcolor]
=
==============
*/

void GrabPalette (void)
{
	int             start,end,length;

	if (TokenAvailable())
	{
		GetToken (false);
		start = atoi (token);
		GetToken (false);
		end = atoi (token);
	}
	else
	{
		start = 0;
		end = 255;
	}

	length = 3*(end-start+1);
	memcpy (lump_p, lbmpalette+start*3, length);
	lump_p += length;
}


/*
=============================================================================

							FONT GRABBING

=============================================================================
*/


font_t   *font;
int             sx,sy,ch;


/*
===================
=
= GrabChar
=
= Grabs the next character after sx,sy of height font->height, and
= advances sx,sy
=
===================
*/

void GrabChar (void)
{
	int             y;
	byte            *screen_p,b;
	int             count;

//
// look for a vertical line with a source pixel
//

	do
	{
		screen_p = byteimage + sy*byteimagewidth+sx;
		for (y=0 ; y<font->height ; y++)
			if (screen_p[y*byteimagewidth])
				goto startgrabing;
		if (++sx == byteimagewidth)
		{
			sx=0;
			sy += font->height+1;
			if (sy+font->height > byteimagewidth)
				Error ("Ran out of characters at char %i\n",ch);
		}

	} while (1);

startgrabing:
//
// grab the character
//
	font->width[ch] = 0;
	font->charofs[ch] = LittleShort (lump_p - (byte  *)font );

	do
	{
		font->width[ch]++;

		screen_p = byteimage + sy*byteimagewidth+sx;
		count = 0;

		for (y=0 ; y<font->height ; y++)
		{
			b = *screen_p;
			if (b)
				count++;
			if (b==254)             // 254 is a grabbable 0
				b = 0;
			*lump_p++ = b;
			screen_p += byteimagewidth;
		}

		if (count)      // color the grabbed collumn
			for (y=0 ; y<font->height ; y++)
			{
				screen_p -= byteimagewidth;
				*screen_p = 1;
			}

		if (++sx == byteimagewidth)
		{
			sx=0;
			sy += font->height+1;
			return;
		}

		if (!count)             // hit a blank row?
		{
			lump_p -= font->height;
			font->width[ch]--;
			return;
		}

	} while (1);
}


/*
===================
=
= GrabFont
=
= filename FONT startchar endchar [startchar endchar [...]]
=
===================
*/

void GrabFont (void)
{
	int             y;
	byte            *screen_p;
	int             top,bottom;
	int             startchar,endchar;

	font = (font_t  *)lump_p;
	memset (font,0,sizeof(*font));

//
// find the height of the font by scanning for quide lines (color 255)
//
	screen_p = byteimage;

	top = -1;
	for (y=0;y<10;y++)
		if (screen_p[y*byteimagewidth] == 255)
		{
			top = y;
			break;
		}

	if (top == -1)
		Error ("No color 255 top guideline found!\n");

	bottom = -1;
	for ( y++ ; y<100 ; y++)
		if (screen_p[y*byteimagewidth] == 255)
		{
			bottom = y;
			break;
		}

	if (bottom == -1)
		Error ("No color 255 bottom guideline found!\n");

	font->height = bottom-top-1;            // LittleShort after grabbing everything
	lump_p = &font->data;

	sx = 0;
	sy = top+1;

//
// grab ranges of characters
//
	do
	{
		GetToken (false);
		startchar = atoi (token);
		GetToken (false);
		endchar = atoi (token);

		for (ch=startchar ; ch<=endchar ; ch++)
			GrabChar ();

	} while (TokenAvailable ());

	font->height = LittleShort(font->height);
}




/*
==============
=
= GrabPatch
=
= filename patch x y width height [originx [originy]]
=
==============
*/

void GrabPatch255 (void)
{
	int             x,y,xl,yl,xh,yh;
	int             orgx, orgy;
	int             width;
	byte            pixel, oldpixel, transcolor, *length_p;
	patch_t *header;

	GetToken (false);
	xl = atoi (token);
	GetToken (false);
	yl = atoi (token);
	GetToken (false);
	xh = xl-1+atoi (token);
	GetToken (false);
	yh = yl-1+atoi (token);

	if (xh<xl || yh<yl || xl < 0 || yl<0 || xh>319 || yh>199)
		Error ("Bad size: %i, %i, %i, %i",xl,yl,xh,yh);

	if (TokenAvailable ())
	{
		GetToken (false);
		orgx = xl+atoi (token);
		if (TokenAvailable ())
		{
			GetToken (false);
			orgy = yl+atoi (token);
		}
		else
			orgy = yh;
	}
	else
	{
		orgx = (xh+xl)/2;
		orgy = yh-4;
	}

//
// find exact bounding box
//
	transcolor = 255;

	while (1)
	{
		for (y=yl ; y<= yh ; y++)
			if (SCRN(xl,y) != transcolor)
				goto gotxl;
		for (y=yl ; y<= yh ; y++)
			SCRN(xl,y) = 0xff;
		xl++;
		if (xl > xh)
			Error ("GrabPatch: Nothing in block");
	}
gotxl:

	while (1)
	{
		for (y=yl ; y<= yh ; y++)
			if (SCRN(xh,y) != transcolor)
				goto gotxh;
		for (y=yl ; y<= yh ; y++)
			SCRN(xh,y) = 0xff;
		xh--;
	}
gotxh:

	while (1)
	{
		for (x=xl ; x<= xh ; x++)
			if (SCRN(x,yl) != transcolor)
				goto gotyl;
		for (x=xl ; x<= xh ; x++)
			SCRN(x,yl) = 0xff;
		yl++;
	}
gotyl:

	while (1)
	{
		for (x=xl ; x<= xh ; x++)
			if (SCRN(x,yh) != transcolor)
				goto gotyh;
		for (x=xl ; x<= xh ; x++)
			SCRN(x,yh) = 0xff;
		yh--;
	}
gotyh:


//
// fill in header
//
	header = (patch_t  *)lump_p;
	header->leftoffset = orgx- xl;
	header->topoffset = orgy-yl;
	width = xh-xl+1;
	header->width = width;
	header->height = yh-yl+1;

//
// start grabbing posts
//
	lump_p = (byte *)&header->collumnofs[header->width];

	for (x=xl ; x<= xh ; x++)
	{
		header->collumnofs[x-xl] = LittleLong(lump_p - (byte *)header);
		y = yl;
		while (1)
		{
		// grab a segment
		// an extra pixel is grabbed at the start and end of the column
		// to keep a round off error from grabbing a strange pixel

		// don't grab more than 128 high, because the texture mapper wraps
		// fake it with two contiguous columns
		
			while (SCRN(x,y) == transcolor && y<= yh)
				y++;

			if (y > yh)
			{
			// end of row
				*lump_p++ = 0xff;
				break;
			}

			*lump_p++ = y-yl;       // delta from top of shape
			length_p = lump_p++;
			*length_p=0;
			*lump_p++ = SCRN(x,y);
	// write an extra copy of first pixel
			while ( (pixel = SCRN(x,y)) != transcolor && y<= yh 
			&& *length_p != 128)
			{
				*lump_p++ = pixel;
				SCRN(x,y) = 0;
				(*length_p)++;  // inc pixel count
				y++;
				oldpixel = pixel;
			}
			*lump_p++ = oldpixel;
	// write an extra copy of last pixel
		}
	}

}

//=============================================================================

// this was used for character sprites in wolfjag, but it wasn't thought
// out well enough for general use

/*
==============
=
= GrabJagBlock
=
= name x y width height
=
==============
*/

#define MASKCOLOR 0

int             grabsolid;

void GrabJagBlock (void);

void GrabJagSolid (void)
{
	grabsolid = 1;
	GrabJagBlock ();
	grabsolid = 0;
}

void GrabJagBlock (void)
{
	int             x,y,xl,yl,xh,yh,pix;
	int             width, blockxh, blockyl, blockxl;

	GetToken (false);
	xl = blockxl = atoi (token);
	GetToken (false);
	yl = blockyl = atoi (token);
	GetToken (false);
	xh = blockxh = xl-1+atoi (token);
	GetToken (false);
	yh = yl-1+atoi (token);

//
// find first and last collumn
//
	if (!grabsolid)
	{

	for ( ; xl<=xh ; xl++)
		for (y=yl ; y<=yh;y++)
			if (SCRN(xl,y) != MASKCOLOR)
				goto gotfirst;
gotfirst:
	for ( ; xh>=xl ; xh--)
		for (y=yl ; y<=yh;y++)
			if (SCRN(xh,y) != MASKCOLOR)
				goto gotlast;
gotlast:


	for ( ; yl<=yh ; yl++)
		for (x=xl ; x<=xh;x++)
			if (SCRN(x,yl) != MASKCOLOR)
				goto gottop;
gottop:
	for ( ; yh>=yl ; yh--)
		for (x=xl ; x<=xh;x++)
			if (SCRN(x,yh) != MASKCOLOR)
				goto gotbottom;
gotbottom: ;
	}

	width = (xh-xl+8)&~7;
	xh = xl + width -1;

	*lump_p++ = xl - blockxl;
	*lump_p++ = yl - blockyl;
	*lump_p++ = width;
	*lump_p++ = yh-yl+1;

	*lump_p++ = 0;
	*lump_p++ = 0;
	*lump_p++ = 0;
	*lump_p++ = 0;

	for (y=yl ; y<=yh ; y++)
	{
		for (x=xl ; x <=xh ; x++)
		{
			if (x>blockxh)
				pix = MASKCOLOR;
			else
				pix = SCRN(x,y);
			if (pix==MASKCOLOR)
			{
				pix = 0;
				SCRN(x,y) = 0;
			}
			else
				SCRN(x,y) = MASKCOLOR;

			*lump_p++ = pix;
		}
	}
}

//=============================================================================


/*
=================
=
= GrabTile
=
= x/y are tile coordinates (pixels*8)
=================
*/

void GrabTile (int x, int y, int planes)
{
	int	sx,sy;
	byte	tile[8][8], b;
	byte	*dest;
	
//
// grab it
//
	for (sy = 0 ; sy< 8 ; sy++)
		for (sx = 0 ; sx< 8 ; sx++)
		{
			tile[sy][sx] = SCRN(x*8+sx,y*8+sy);
			SCRN(x*8+sx,y*8+sy) = 0xff;
		}

	if (planes == 7)
	{	// mode 7 tiles are just bytes
		memcpy (lump_p, tile, 64);
		lump_p += 64;
		return;
	}

//
// munge it
//
	memset (lump_p,0,64);

	for (sy = 0 ; sy< 8 ; sy++)
		for (sx = 0 ; sx< 8 ; sx++)
		{
			b = tile[sy][sx];

			if (planes == 1)
			{	// for jaguar
				lump_p[sy] <<=1;
				lump_p[sy] |= b&1;
				continue;
			}

			if (planes == 2)
			{	// for jaguar
				dest = lump_p + sy*2 + (sx>>2);
				*dest <<= 2;
				*dest |= b&3;
				continue;
			}

			lump_p[sy*2] <<=1;
			lump_p[sy*2] |= b&1;
			b>>=1;

			lump_p[sy*2+1] <<=1;
			lump_p[sy*2+1] |= b&1;
			b>>=1;

			if (planes == 2)
				continue;

			lump_p[1*16+sy*2] <<=1;
			lump_p[1*16+sy*2] |= b&1;
			b>>=1;

			lump_p[1*16+sy*2+1] <<=1;
			lump_p[1*16+sy*2+1] |= b&1;
			b>>=1;

			if (planes == 4)
				continue;

			lump_p[2*16+sy*2] <<=1;
			lump_p[2*16+sy*2] |= b&1;
			b>>=1;

			lump_p[2*16+sy*2+1] <<=1;
			lump_p[2*16+sy*2+1] |= b&1;
			b>>=1;

			lump_p[3*16+sy*2] <<=1;
			lump_p[3*16+sy*2] |= b&1;
			b>>=1;

			lump_p[3*16+sy*2+1] <<=1;
			lump_p[3*16+sy*2+1] |= b&1;
			b>>=1;
		}


	lump_p += 8*planes;
}


/*
==============
=
= GrabSnesBlock
=
= planes x y width height
=
= Coordinates are in tiles
= Chunks up into 8*8 tiles, and can grab different bit depths
==============
*/


void GrabSnesBlock (void)
{
	int		x,y,w,h, planes;
	int		xs,ys;

	GetToken (false);
	planes = atoi (token);
	GetToken (false);
	x = atoi (token);
	GetToken (false);
	y = atoi (token);
	GetToken (false);
	w = atoi (token);
	GetToken (false);
	h = atoi (token);

	for (ys = y ; ys<y+h ; ys++)
		for (xs = x ; xs<x+w ; xs++)
			GrabTile (xs,ys, planes);

}


