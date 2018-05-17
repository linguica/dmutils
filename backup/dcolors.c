#define VERSION "1.1"

/*
=============================================================================

								DCOLORS

							by John Carmack

=============================================================================
*/

#include "cmdlib.h"
#include "lbmlib.h"

#define	NUMLIGHTS		32
#define	GRAYCOLORMAP 		32

byte	*palette;

byte	lightpalette[NUMLIGHTS+2][256];
short	color12s[NUMLIGHTS+2][256];
short	color15s[NUMLIGHTS+2][256];


/*
=====================
=
= ColorShiftPalette
=
= at shift = 0, the colors are normal
= at shift = steps, the colors are all the given rgb
=====================
*/

void	ColorShiftPalette (byte *inpal, byte *outpal
, int r, int g, int b, int shift, int steps)
{
	int	i;
	int	dr, dg, db;
	byte	*in_p, *out_p;

	in_p = inpal;
	out_p = outpal;

	for (i=0 ; i<256 ; i++)
	{
		dr = r - in_p[0];
		dg = g - in_p[1];
		db = b - in_p[2];

		out_p[0] = in_p[0] + dr*shift/steps;
		out_p[1] = in_p[1] + dg*shift/steps;
		out_p[2] = in_p[2] + db*shift/steps;

		in_p += 3;
		out_p += 3;
	}
}

/*
===============
=
= BestColor
=
===============
*/

byte BestColor (int r, int g, int b, byte *palette, int rangel, int rangeh)
{
	int	i;
	long	dr, dg, db;
	long	bestdistortion, distortion;
	int	bestcolor;
	byte	*pal;

//
// let any color go to 0 as a last resort
//
	bestdistortion = ( (long)r*r + (long)g*g + (long)b*b )*2;
	bestcolor = 0;

	pal = &palette[rangel*3];
	for (i=rangel ; i<= rangeh ; i++)
	{
		dr = r - (int)pal[0];
		dg = g - (int)pal[1];
		db = b - (int)pal[2];
		pal += 3;
		distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (!distortion)
				return i;		// perfect match

			bestdistortion = distortion;
			bestcolor = i;
		}
	}

	return bestcolor;
}


/*
=====================
=
= RF_BuildLights
=
= 0 is full palette
= NUMLIGHTS	and NUMLIGHTS+1 are all black
=
=====================
*/

void	RF_BuildLights (void)
{
	int		l,c;
	int		red,green,blue, ri, gi, bi;
	byte	*palsrc;
	short	color12,color15;

	for (l=0;l<NUMLIGHTS;l++)
	{
//printf ("%i.",NUMLIGHTS-l);
		palsrc = palette;
		for (c=0;c<256;c++)
		{
			red = *palsrc++;
			green = *palsrc++;
			blue = *palsrc++;

			red = (red*(NUMLIGHTS-l)+NUMLIGHTS/2)/NUMLIGHTS;
			green = (green*(NUMLIGHTS-l)+NUMLIGHTS/2)/NUMLIGHTS;
			blue = (blue*(NUMLIGHTS-l)+NUMLIGHTS/2)/NUMLIGHTS;

			ri = (red+8)>>4;
			ri = ri > 15 ? 15 : ri;
			gi = (green+8)>>4;
			gi = gi > 15 ? 15 : gi;
			bi = (blue+8)>>4;
			bi = bi > 15 ? 15 : bi;

			color12 = (ri<<12) + (gi<<8) + (bi<<4) + 15;

			ri = (red+4)>>3;
			ri = ri > 31 ? 31 : ri;
			gi = (green+4)>>3;
			gi = gi > 31 ? 31 : gi;
			bi = (blue+4)>>3;
			bi = bi > 31 ? 31 : bi;

			color15 = (ri<<10) + (gi<<5) + bi;

			lightpalette[l][c] = BestColor(red,green,blue,palette,0,255);

			color12s[l][c] = color12;
			color15s[l][c] = color15;
		}

		memcpy (screen,lightpalette[l],256);
		screen+=320;
		memcpy (screen,lightpalette[l],256);
		screen+=320;
		memcpy (screen,lightpalette[l],256);
		screen+=320;
	}
}


/*
=====================
=
= BuildSpecials
=
= Red and gray colormaps
=
=====================
*/

void	BuildSpecials (void)
{
	int		c;
	float	red, green, blue, gray;
	byte	*palsrc;

	palsrc = palette;
	for (c=0;c<256;c++)
	{
		red = *palsrc++ / 256.0;
		green = *palsrc++ / 256.0;
		blue = *palsrc++ / 256.0;

		gray = red*0.299 + green*0.587 + blue*0.144;
		gray = 1.0 - gray;
		lightpalette[GRAYCOLORMAP][c] = BestColor(gray*255,gray*255,gray*255,palette,0,255);
	}

	memcpy (screen,lightpalette[GRAYCOLORMAP],256);
	screen+=320;
	memcpy (screen,lightpalette[GRAYCOLORMAP],256);
	screen+=320;
	memcpy (screen,lightpalette[GRAYCOLORMAP],256);
	screen+=320;
}


/*
====================
=
= main
=
====================
*/

int main (int argc, char **argv)
{
	int 		i,handle;
	byte		*pic;
	byte		outpal[768];

	printf ("\nDCOLORS "VERSION" by John Carmack, copyright (c) 1992 Id Software\n");

	if (argc != 2)
		Error ("dcolors picture.lbm");

//
// load lbm for base palette
//
	LoadLBM (argv[1],&pic, &palette);
	VGAMode ();
	SetPalette (palette);
	memcpy ( screen, pic, 64000);
	free (pic);			// don't care whats on it

//
// build palette shifts
//
	handle = SafeOpenWrite ("playpal.lmp");

	SafeWrite (handle, palette, 768);

	GetKey ();

	for (i=1 ; i<9 ; i++)
	{
		ColorShiftPalette (palette, outpal, 255, 0, 0, i, 9);
		SafeWrite (handle, outpal, 768);
		SetPalette (outpal);
		GetKey ();
	}

	SetPalette (palette);
	GetKey ();

	for (i=1 ; i<5 ; i++)
	{
		ColorShiftPalette (palette, outpal, 215, 186, 69, i, 8);
		SafeWrite (handle, outpal, 768);
		SetPalette (outpal);
		GetKey ();
	}

	ColorShiftPalette (palette, outpal, 0, 256, 0, 1, 8);
	SafeWrite (handle, outpal, 768);
	SetPalette (outpal);
	GetKey ();

	close (handle);
	SetPalette (palette);

//
// build light mappings
//
	RF_BuildLights ();

//
// build special maps
//
	BuildSpecials ();

//
// write lumps
//
	SaveFile ("colormap.lmp",&lightpalette[0][0],sizeof(lightpalette));
//	SaveFile ("colors12.lmp",&color12s[0][0],NUMLIGHTS*512);
//	SaveFile ("colors15.lmp",&color15s[0][0],NUMLIGHTS*512);

	GetKey ();
	TextMode ();

	return 0;
}


