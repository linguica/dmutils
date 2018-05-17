#define VERSION "1.0"

/*
=============================================================================

								MAPCOMP

							by John Carmack

=============================================================================
*/

#include "cmdlib.h"
#include "doomdata.h"

#define MAXPATH 1025

//================
//
// wad file types
//
//================

typedef struct
{
	long            filepos;
	long            size;
	char            name[8];
} lumpinfo_t;


typedef struct
{
	char    identification[4];
	long    numlumps;
	long    infotableofs;
} wadinfo_t;

//==========================================================================

wadinfo_t       newwad;                  // the file being written out
lumpinfo_t      *newlumps;
int             newhandle;
lumpinfo_t      *lump_p;                 // wadlumps[lumpon]
int             lumpon;                  // current lump

char            loadwadpath[MAXPATH];
char            lumppath[MAXPATH];
int             datahandle;              // can be an individual file or a WAD
wadinfo_t       loadwad;                 // a WAD file to look for lumps in
lumpinfo_t      *wadlumps;
lumpinfo_t      *wadlump_p;              // info on lump to copy




//============================================================================

int		numtextures = 0;
char	texnames[2048][8];

/*
==================
=
= AddTextureFile
=
==================
*/

void AddTextureFile (char *name)
{
	maptexture_t	*mtexture;
	int			i;
	int			*maptex;
	int			offset, maxoff;
	int			numtextures1;
	int			*directory;
	
//
// load the map texture definitions from
//
	maxoff = LoadFile (name, (void **)&maptex);
	numtextures1 = LittleLong(*maptex);
	directory = maptex+1;
		
	for (i=0 ; i<numtextures1 ; i++, directory++)
	{		
		offset = LittleLong(*directory);
		if (offset > maxoff)
			Error ("InitTextures: bad texture directory");
			
		mtexture = (maptexture_t *) ( (byte *)maptex + offset);
		memcpy (texnames[numtextures], mtexture->name,8);
		numtextures++;
	}

	free (maptex);
}

/*
================
=
= TextureNumForName
=
================
*/

int	TextureNumForName (char *name)
{
	int		i;
	char	namet[9];
	
	if (name[0] == '-')		// no texture marker
		return 0;
		
	for (i=0 ; i<numtextures ; i++)
		if (!strncasecmp (texnames[i], name, 8) )
			return i;
	
	namet[8] = 0;
	memcpy (namet, name,8);
	Error ("TextureNumForName: %s not found",namet);
	
	return -1;
}



//============================================================================



/*
=================
=
= CopyLump
=
=================
*/

void CopyLump (void)
{
	long    size;
	byte            *buffer;

	lseek (datahandle, LittleLong (wadlump_p->filepos) ,SEEK_SET);
	size = LittleLong (wadlump_p->size);

	strncpy (lump_p->name , wadlump_p->name , 8);
	lump_p->filepos = LittleLong (tell(newhandle));
	lump_p->size = LittleLong (size);

	buffer = malloc (size);
	SafeRead (datahandle, buffer, size);    
	SafeWrite (newhandle, buffer, size);
	free (buffer);
	
	lumpon++;
	lump_p++;
	wadlump_p++;
}


/*
=================
=
= OpenWad
=
=================
*/

void OpenWad (char *wadname)
{
//
// open it and read in header / lump directory
//
	datahandle = SafeOpenRead (wadname);
	SafeRead (datahandle,&loadwad,sizeof(loadwad));
	loadwad.numlumps = LittleLong (loadwad.numlumps);
	loadwad.infotableofs = LittleLong (loadwad.infotableofs);

	if (strncmp(loadwad.identification,"IWAD",4))
		Error ("Wad file %s doesn't have IWAD id\n",lumppath);

	lseek (datahandle , loadwad.infotableofs , SEEK_SET);

	wadlumps = wadlump_p = malloc ( loadwad.numlumps*sizeof(lumpinfo_t) );
	SafeRead (datahandle, wadlumps , loadwad.numlumps*sizeof(lumpinfo_t));
}


/*
=====================
=
= OpenDest
=
=====================
*/

void OpenDest (char *name)
{
//
// get the output filename
//
	printf ("Output wad file: %s\n", name);

	newhandle = SafeOpenWrite (name);
	newwad.numlumps = 0;
	strncpy (newwad.identification, "IWAD", 4);

//
// allocate space for the lump directory
//
	newlumps = malloc (0xfff0);
	lump_p = newlumps;
	lumpon = 0;

//
// position the file pointer to begin writing data
//
	// leave space in the data file for the header
	lseek (newhandle,sizeof(newwad),SEEK_SET);

}


/*
=================
=
= CrunchSidedefs
=
=================
*/

void CrunchSidedefs (void)
{
	long   				size, newsize;
	byte    			*buffer, *buf2;
	int					i, numsides;
	mapsidedef_t		*ms;
	compmapsidedef_t	*cms;
	
	lseek (datahandle, LittleLong (wadlump_p->filepos) ,SEEK_SET);
	size = LittleLong (wadlump_p->size);

	buffer = malloc (size);
	SafeRead (datahandle, buffer, size);    
	ms = (mapsidedef_t *)buffer; 
	numsides = size/sizeof(mapsidedef_t);
	
	newsize = numsides*sizeof(compmapsidedef_t);
	buf2 = malloc (newsize);
	cms = (compmapsidedef_t *)buf2;
	
	for (i=0 ; i<numsides ; i++)
	{
		cms->textureoffset = ms->textureoffset;
		cms->rowoffset = ms->rowoffset;
		cms->sector = ms->sector;
		cms->toptexture = LittleShort (TextureNumForName (ms->toptexture));
		cms->bottomtexture =LittleShort(TextureNumForName (ms->bottomtexture));
		cms->midtexture = LittleShort (TextureNumForName (ms->midtexture));		
		ms++;
		cms++;
	}
	
	strncpy (lump_p->name , wadlump_p->name , 8);
	lump_p->filepos = LittleLong (tell(newhandle));
	lump_p->size = LittleLong (newsize);

	SafeWrite (newhandle, buf2, newsize);
	free (buffer);
	free (buf2);
	printf ("%i sides, saved %i bytes\n", numsides, size - newsize);
	
	lumpon++;
	lump_p++;
	wadlump_p++;
}



/*
===================
=
= CrunchWad
=
===================
*/

void CrunchWad (char *name)
{
	char		dest[MAXPATH];
	
	OpenWad (name);
	if (loadwad.numlumps != 10 || wadlumps[0].name[0] != 'E' 
	|| wadlumps[0].name[2] != 'M' || wadlumps[0].name[4] != 0 )
		Error ("wad file does not seem to be an uncompressed doom map");
		
	strcpy (dest, name);
	StripExtension (dest);
	strcat (dest,"c.wad");
		
	OpenDest (dest);

	wadlump_p->name[4] = 'C';	
	wadlump_p->name[5] = 0;	
	CopyLump ();
		// copy label with a C appended

	CopyLump ();
		// copy things
	CopyLump ();
		// copy linedefs
	
	CrunchSidedefs ();	// compress sidedefs
	
	CopyLump ();
		// copy vertexes
	CopyLump ();
		// copy segs
	CopyLump ();
		// copy ssectors
	CopyLump ();
		// copy nodes
	CopyLump ();
		// copy sectors
	CopyLump ();
		// copy blockmap

	close (datahandle);


//
// write lumpinfo table
//
	newwad.numlumps = LittleLong (lumpon);
	newwad.infotableofs = LittleLong (tell(newhandle));
	write (newhandle,newlumps, lumpon*sizeof(lumpinfo_t));

//
// write wadinfo
//
	lseek (newhandle,0,SEEK_SET);
	write (newhandle,&newwad,sizeof(newwad));

	close (newhandle);
}


/*
===================
=
= main
=
===================
*/

int main (int argc, char **argv)
{
	int		i, s;

	printf ("\nMAPCOMP "VERSION" by John Carmack, copyright (c) 1993 Id Software\n");

	if (argc == 1)
	{
		printf (
		"Usage: mapcomp texturefile [texturefiles] -wads e1m1.wad ...\n"
		"generates e1m1c.wad ...\n"
		);
		exit (1);
	}

	s = CheckParm ("wads");
	if (!s)
		Error ("no -wads parm");
		
	for (i=1 ; i<s ; i++)
		AddTextureFile (argv[i]);
	printf ("numtextures : %i\n",numtextures);
	

	for (i=s+1 ; i<argc ; i++)
		CrunchWad (argv[i]);

	return 0;
}
