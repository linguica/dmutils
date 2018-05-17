#define VERSION "1.1"

/*
=============================================================================

							  SPITWAD

						  by John Carmack

=============================================================================
*/

#include "lumpy.h"


#define MAXLUMP                 0x40000         // biggest possible lump (256k)
#define MAXLUMPS                2048            // max lumps in one WAD file

typedef struct
{
	char    *name;
	void    (*function) (void);
} command_t;


byte            *byteimage, *lbmpalette;
int                     byteimagewidth, byteimageheight;

boolean         makewad;
boolean         lumppause;

byte            *lump_p;
lumpinfo_t      *header, *header_p;
wadinfo_t       wadheader;

char            scriptpath[1025];
char            lumpname[9];            // lump names are 8 chars max

int                     handle;

int                     lumpbuffersize;
byte            *lumpbuffer;


/*
=============================================================================

							MAIN

=============================================================================
*/

void GrabRaw (void);
void GrabVRaw (void);
void GrabPic (void);
void GrabLinearPic (void);
void GrabFont (void);
void GrabPalette (void);

void GrabPatch (void);
void GrabPatch255 (void);

void GrabJagBlock (void);
void GrabJagSolid (void);
void GrabSnesBlock (void);
void GrabJagPalette (void);

void GrabJagObj (void);
void GrabJagPatch (void);
void GrabJagWall (void);

command_t       commands[] =
{
	{"raw",GrabRaw},
	{"vraw",GrabVRaw},
	{"pic",GrabPic},
	{"lpic",GrabLinearPic},
	{"font",GrabFont},
	{"palette",GrabPalette},

	{"patch",GrabPatch255},
	{"patch255",GrabPatch255},

	{"jagblock",GrabJagBlock},
	{"jagsolid",GrabJagSolid},

	{"jagobj",GrabJagObj},
	{"jagpal",GrabJagPalette},
	{"jagpatch",GrabJagPatch},
	{"jagwall",GrabJagWall},

	{"snesblock",GrabSnesBlock},

	{NULL,NULL}                     // list terminator
};


void UpdateImage (void)
{
}


void Pause (void)
{
#ifdef __NeXT__
	getchar ();
#else
	int     ret;

	ret = _bios_keybrd (_KEYBRD_READ);
	if ( ret>>8 == 1)
		Error ("Aborted");
#endif
}


/*
==============
=
= LoadScreen
=
==============
*/

void LoadScreen (char *name)
{
//
// load the lbm
//
	if (lumppause)
	{
		TextMode ();
		printf ("grabbing from screen: %s\n",name);
		Pause ();
	}
	LoadLBM (name, &byteimage, &lbmpalette);
	VGAMode ();
	SetPalette (lbmpalette);

#ifndef __NeXT__
	memcpy ((byte *)0xa0000,byteimage,64000);
	byteimage = (byte *)0xa0000;
#endif

	byteimagewidth = 320;
	byteimageheight = bmhd.h;

}


/*
===============
=
= WriteLump
=
===============
*/


void WriteLump (void)
{
	int		size;
	char    lumppath[256];
	
	grabbed++;
	//
	// save the grabbed lump to disk
	//
	// dword align
	while ((int)lump_p&3)
		*lump_p++ = 0;

	size = lump_p - lumpbuffer;
	if (size > MAXLUMP)
		Error ("Lump size exceeded %lu, memory corrupted!",MAXLUMP);

	if (!makewad)
	{
	// open a seperate file for the lump
		strcpy (lumppath, scriptpath);
		strcat (lumppath,lumpname);
		strcat (lumppath,".lmp");
		handle = SafeOpenWrite (lumppath);
	}
	else
	{
	// record directory info for the lump in the wad header

		if (grabbed == MAXLUMPS)
			Error ("Too many lumps grabbed for WAD file!");

		header_p->filepos = LittleLong(tell (handle));
		header_p->size = LittleLong(size);
		strncpy (header_p->name, lumpname,8);
		header_p++;
	}

	SafeWrite (handle, lumpbuffer, size);

	if (!makewad)
	{
		close(handle);
		handle = -1;
	}
		
}

/*
================
=
= ParseScript
=
================
*/

void ParseScript (void)
{
	int                     cmd;
	int                     size;

	lumpbuffer = malloc (MAXLUMP);
	grabbed = 0;

	do
	{
		//
		// get a command / lump name
		//
		GetToken (true);
		if (endofscript)
			break;
		if (!strcmpi (token,"$LOAD"))
		{
			GetToken (false);
			LoadScreen (token);
			continue;
		}


		//
		// new lump
		//
		memset (lumpname,0,sizeof(lumpname));
		strncpy (lumpname, token, 9);
		for (size=0 ; size<8 ; size++)
			lumpname[size] = toupper(lumpname[size]);

		//
		// get the grab command
		//
		lump_p = lumpbuffer;

		GetToken (false);

		//
		// call a routine to grab some data and put it in lumpbuffer
		// with lump_p pointing after the last byte to be saved
		//
		for (cmd=0 ; commands[cmd].name ; cmd++)
			if ( !strcmpi(token,commands[cmd].name) )
			{
				commands[cmd].function ();
				break;
			}

		if ( !commands[cmd].name )
			Error ("Unrecognized token '%s' at line %i",token,scriptline);

		if (lumppause)
			Pause ();

		WriteLump ();
		
		UpdateImage ();         // display highlighting
	} while (script_p < scriptend_p);
}

/*
=================
=
= GrabLumpyScript
=
= The LBM should allready have been loaded
= Loads a script file, then grabs everything into it
=
=================
*/

void GrabLumpyScript (char const *basename,  boolean wadfile, boolean dopause)
{
	char            script[256];
	char            picture[256];
	char            filename[256];
	int             offset, size;

	handle = -1;
	makewad = wadfile;
	lumppause = dopause;

//
// read in the script file
//
	strcpy (script, basename);
	DefaultExtension (script, ".ls");
	LoadScriptFile (script);

//
// if the first token is not a $load command, use the basename for lbm screen
//
	GetToken (true);                                        // peek at first token
	UnGetToken ();

	if ( strcmpi(token,"$LOAD") )
	{
		strcpy (picture, basename);
		strcat (picture,".lbm");
		LoadScreen (picture);
	}

//
// open the output file
//
	if (makewad)
	{
		header = header_p = malloc (MAXLUMPS*sizeof(lumpinfo_t) );

		strcpy (filename,basename);
		StripExtension (filename);
		strcat (filename,".wad");
		handle = SafeOpenWrite (filename);
		lseek (handle,sizeof(wadinfo_t),SEEK_SET); // leave space for prologue
	}
	else
	{
		strcpy (scriptpath, basename);
		StripFilename (scriptpath);
	}

//
// parse the script commands
//
	ParseScript ();


//
// if a wad grab, write the header out
//
	if (makewad)
	{
		offset = tell (handle);
		size = grabbed*sizeof(lumpinfo_t);
		SafeWrite (handle, header, size);

		lseek (handle,0,SEEK_SET);
		wadheader.numlumps = LittleLong(grabbed);
		wadheader.infotableofs = LittleLong(offset);
		strncpy (wadheader.identification, "IWAD", 4);

		SafeWrite (handle,&wadheader , sizeof(wadheader));
		close (handle);
		handle = -1;
	}


//
// all done
//
#ifndef __NeXT__
	TextMode ();
#endif
	if (makewad)
		printf ("%i lumps grabbed in a WAD file\n",grabbed);
	else
		printf ("%i lumps grabbed\n",grabbed);

}


/*
==============================
=
= main
=
==============================
*/

int main (int argc, char **argv)
{
	char            *filename;
	boolean         inwadfile, dopause;

	printf ("\nLUMPY "VERSION" by John Carmack, copyright (c) 1992 Id Software\n");

	if (argc == 1)
		Error ("LUMPY [-s] [-p] basename");
		
	inwadfile = !CheckParm ("s");
	dopause = CheckParm ("p");
	
	filename = argv[argc-1];
	
	GrabLumpyScript (filename, inwadfile, dopause);

	return 0;
}
