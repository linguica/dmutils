// gengrab.c

#include "lumpy.h"

#define SCRN(x,y)       (*(byteimage+(y)*byteimagewidth+x))

typedef struct
{
	short	width;		// in pixels
	short	height;
	short	depth;		// 1-5
	short	index;		// location in palette of color 0
	short	pad1,pad2,pad3,pad4;	// future expansion
	byte	data[8];	// as big as needed
} jagobj_t;

/*
==============
=
= GrabJagObj
=
= name depth index x y width height [orgx orgy]
=
==============
*/

void GrabJagObj (void)
{
	int             x,y,xl,yl,xh,yh,pix;
	int             width, height;
	jagobj_t		*jago;
	int				depth, index;
	int				packcount, pack, mask, shift;
	int				pixels;
	
	GetToken (false);
	depth = atoi (token);
	GetToken (false);
	index = atoi (token);
	
	GetToken (false);
	xl = atoi (token);
	GetToken (false);
	yl = atoi (token);
	GetToken (false);
	width = atoi (token);
	GetToken (false);
	height = atoi (token);

	if (depth == 3)
	{ pack = 1; shift = 8; mask = 255; }
	else if (depth == 2)
	{ pack = 2; shift = 4; mask = 15; }
	else if (depth == 1)
	{ pack = 4; shift = 2; mask = 3; }
	else if (depth == 0)
	{ pack = 8; shift = 1; mask = 1; }
	else
		Error ("Pack value in a jagobj must be 0,1,2, or 3");
		
	jago = (jagobj_t *)lump_p;
	memset (jago,0,sizeof(*jago));
	jago->depth = BigShort(depth);
	jago->index = BigShort(index);
	jago->width = BigShort(width);
	jago->height = BigShort(height);
	
	lump_p = (byte *)&jago->data;

	xh = xl+width;
	yh = yl+height;
	
	packcount = 0;
	for (y=yl ; y<yh ; y++)
	{
		for (x=xl ; x <xh ; x++)
		{
			pix = SCRN(x,y);
			SCRN(x,y) = 255;
			
			pix = (pix-index) & mask;
			
			pixels = (pixels << shift) | pix;
			if (++packcount == pack)
			{
				packcount = 0;
				*lump_p++ = pixels;
			}
		}
	}
}

//=============================================================================

#include <math.h>

#define round(x) (floor((x)+0.5))

int CRYtable[3][256];
boolean	tablebuilt = false;


//
//  Builds a table for Red, Green, Blue parts of CRY pixels
//

void CRYtoRGBnormalized(int CRYx, int CRYy, double *Rnorm,
  double *Gnorm, double *Bnorm)
{

  int x, y;

  x = CRYx;
  y = CRYy;

  // lower left CRY quadrant
  if (x < 8 && y <= 15-x)
  {
    *Rnorm = x / 8.0;
    *Gnorm = y / (15.0-x);
    *Bnorm = 1.0;
  }
  // lower right CRY quadrant
  else if (x >= 8 && y < x)
  {
    *Rnorm = 1.0;
    *Gnorm = (double) y / x;
    *Bnorm = (15.0-x) / 8;
  }
  else
  {
    *Gnorm = 1.0;
    // upper left CRY quadrant
    if (x < 8)
    {
      *Rnorm = (15.0 - y) / 8;
      *Bnorm = 1 - abs(15 - (y+x)) / 8.0;
    // upper right CRY quadrant
    } else {
      *Rnorm = 1 - abs(y - x) / 8.0;
      *Bnorm = (15.0 - y) / 8;
    }
  }
  
}

void buildCRYtable(int table[][256])
{

  int x, y;
  double rn, gn, bn;

  for (y=0 ; y<16 ; y++)
  {
    for(x=0 ; x<16 ; x++)
    {
      CRYtoRGBnormalized(x, y, &rn, &gn, &bn);
      table[0][y*16+x] = round(rn*255);
      table[1][y*16+x] = round(gn*255);
      table[2][y*16+x] = round(bn*255);
    }
  }

}

void findClosestCRY(int CRYtable[][256], int r, int g, int b,
  int *cryx, int *cryy)
{

  int i, dr, dg, db;
  int best, bestnum;
  int dist;

  best = 256;
  bestnum = -1;
  for (i=0 ; i<256 ; i++)
  {
    dr = abs(CRYtable[0][i] - r);
    dg = abs(CRYtable[1][i] - g);
    db = abs(CRYtable[2][i] - b);
    dist = sqrt((double)dr*dr+dg*dg+db*db);
    if (dist < best)
    {
      best = dist;
      bestnum = i;
    }
  }

  if (bestnum == -1)
  {
    fprintf(stderr, "Couldn't make a decent match for %d,%d,%d!\n", r, g, b);
    exit(-1);
  } else {
    *cryy = bestnum/16;
    *cryx = bestnum%16;
  }

}


unsigned short RGB2CRY (int r, int g, int b)
{
	int intensity, cryx, cryy;
    unsigned char cry[2];
	double	nr,ng,nb;
	
	intensity = r > g ? r : g;
	intensity = intensity > b ? intensity : b;
	intensity = intensity ? intensity : 1;
	nr = (double) r / intensity * 255;
	ng = (double) g / intensity * 255;
	nb = (double) b / intensity * 255;
	findClosestCRY(CRYtable, nr, ng, nb, &cryx, &cryy);
	cry[0] = (cryx<<4)+cryy;
	cry[1] = intensity;

	return *(unsigned short *)&cry;
}


/*
==============
=
= GrabJagPalette
=
==============
*/

void GrabJagPalette (void)
{
	int		i;
	byte	*inpal;
	unsigned	cry;
	
	if (!tablebuilt)
		buildCRYtable (CRYtable);
	
	inpal = lbmpalette;
		
	for (i=0 ; i<256 ; i++)
	{
		cry = RGB2CRY(inpal[0],inpal[1],inpal[2]);
		*(unsigned short *)lump_p = cry;
		inpal += 3;
		lump_p+=2;
	}

}


/*
==============
=
= GrabJagWall
=
= Just a vraw with duplication at the end for wrapping
=
= filename jagwall x y width height
=
==============
*/

void GrabJagWall (void)
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
	
	memcpy (lump_p, lumpbuffer, 320);
	lump_p += 320;
}


//===========================================================================


/*
==============
=
= GrabJagPatch
=
= filename patch x y width height [originx [originy]]
=
= Similar to GrapPatch255, but all data is condensed at end so it can
= be easily converted to 16 bit color
=
= all shorts are big endian!
=
= column data is:
= byte 	yofs	(ff = end of column)
= byte 	length
= short data start (offset from jagpatch+headersize)
==============
*/

typedef struct
{
	short	width;                  // bounding box size
	short   height;
	short   leftoffset;             // pixels to the left of origin
	short   topoffset;              // pixels above the origin
	short   collumnofs[320];        // only [width] used, the [0] is &collumnofs[width]
} jagpatch_t;

byte	tempbuffer[64000];

void GrabJagPatch (void)
{
	int             x,y,xl,yl,xh,yh;
	int             orgx, orgy;
	int             width;
	byte            pixel, oldpixel, transcolor, *length_p;
	jagpatch_t *header;
	byte	*temp_p;

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
	header = (jagpatch_t  *)lump_p;
	header->leftoffset = BigShort(orgx- xl);
	header->topoffset = BigShort(orgy-yl);
	width = xh-xl+1;
	header->width = BigShort(width);
	header->height = BigShort(yh-yl+1);

//
// start grabbing posts
//
	temp_p = tempbuffer;

	lump_p = (byte *)&header->collumnofs[width];

	if (width&1)
		lump_p += 2;	// int align
		
	
	for (x=xl ; x<= xh ; x++)
	{
		header->collumnofs[x-xl] = BigShort(lump_p - (byte *)header);
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
				*lump_p++ = 0xff;
				*lump_p++ = 0xff;
				*lump_p++ = 0xff;
				break;
			}

			*lump_p++ = y-yl;       // delta from top of shape
			length_p = lump_p++;	// pixel count
			*((short *)lump_p) =
				BigShort (temp_p+1 - tempbuffer); // data ofs
			lump_p+=2;
			*length_p=0;

			*temp_p++ = SCRN(x,y);	// extra copy of first pixel
			while ( (pixel = SCRN(x,y)) != transcolor && y<= yh
			&& *length_p != 128)
			{
				*temp_p++ = pixel;
				SCRN(x,y) = 0;
				(*length_p)++;  // inc pixel count
				y++;
				oldpixel = pixel;
			}
			*temp_p++ = oldpixel;	// extra copy of last pixel
		}
	}

//
// write the header as one lump
//
	WriteLump ();		// header to lump_p
	
//
// let the rest of the data be another lump
//
	sprintf (lumpname,".");
	memcpy (lumpbuffer, tempbuffer, temp_p - tempbuffer);
	lump_p = lumpbuffer + (temp_p-tempbuffer);

}

