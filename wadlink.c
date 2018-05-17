#define VERSION "1.3"

/*
=============================================================================

								WADLINK

							by John Carmack

=============================================================================
*/

#include "cmdlib.h"
#include "scriplib.h"
#include "lzlib.h"

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

char            destpath[MAXPATH];
wadinfo_t       newwad;                                 // the file being written out
lumpinfo_t      *newlumps;
int                     newhandle;
lumpinfo_t      *lump_p;                                // wadlumps[lumpon]
int                     lumpon;                                 // current lump

char                    loadwadpath[MAXPATH];
char                    lumppath[MAXPATH];
int                     datahandle;                             // can be an individual file or a WAD
wadinfo_t       loadwad;                                // a WAD file to look for lumps in
lumpinfo_t      *wadlumps;
lumpinfo_t      *wadlump_p;                             // info on lump to copy
boolean         inwadfile;

boolean outputnamed;

boolean			bigendian;

char    sourcepath[MAXPATH];
char    defaultdestpath[MAXPATH];


int             lumpscopied;                            // number of copy operations done

boolean		allwayscompress;

//============================================================================


/*
================
=
= CheckIfCompressed
=
= Checks the script file to see if the lump should be compressed, and then
= sets the high bit of the first character of the name if it should.
=
================
*/

boolean CheckIfCompressed(void)
{
	allwayscompress = false;
	
	// check for (LZSS) token
	if (TokenAvailable())
	{
		GetToken(false);
		if (!strcmpi(token, "(LZSS)"))
		{
			fprintf(stdout, "lzss");
			return true;
		} else
		if (!strcmpi(token, "(LZSSALLWAYS)"))
		{
			fprintf(stdout, "lzssallways");
			allwayscompress = true;
			return true;
		} else
		if (!strcmpi(token, "(LZSSMARS)"))
		{
			fprintf(stdout, "lzssmars");
			allwayscompress = true;
			return 3;
		} else
		if (!strcmpi(token, "(LZSSODD)"))
		{
			fprintf(stdout, "lzssodd");
			allwayscompress = true;
			return 2;
		} else {
			Error ("Unrocognized token after lump: %s\n",token);
		}
	}

	return false;

}


/*
================
=
= OpenLump
=
= If in a wad file, lseeks to the start of the lump
=
= If a separate file, sets inputhandle to the open file
=
================
*/

void OpenLump (void)
{
	int                     i;
	char            lumpname[9];

	if (inwadfile)
	{
		lumpname[8] = 0;
		strncpy (lumpname, lump_p->name, 8);

	//
	// set wadlump_p to the lumpinfo in the open wad file
	//
		wadlump_p = wadlumps;
		for (i=0 ; i<loadwad.numlumps ; i++, wadlump_p++)
			if (strncmp( lump_p->name , wadlump_p->name, 8) == 0)
				break;

		if (i == loadwad.numlumps)
			Error ("lump %s is not in wad file %s\n",lumpname,lumppath);

		strcpy (lumppath, loadwadpath);
		strcat (lumppath, " : ");
		strcat (lumppath, lumpname);
	}
	else
	{
	//
	// open the file on disk
	//
		datahandle = SafeOpenRead (lumppath);
	}

}


/*
=================
=
= CopyLump
=
=================
*/

void CopyLump (void)
{
	int    size;
	byte            *buffer;
	byte	*compressed_buffer;
	int	compressed_size;

	if (inwadfile)
	{
		lseek (datahandle, LittleLong (wadlump_p->filepos) ,SEEK_SET);
		size = LittleLong (wadlump_p->size);
	}
	else
		size = filelength (datahandle);

	printf ("%4i = %s (%i bytes)",lumpon,lumppath,size);

	if (bigendian)
	{
		lump_p->filepos = BigLong (tell(newhandle));
		lump_p->size = BigLong (size);
	}
	else
	{
		lump_p->filepos = LittleLong (tell(newhandle));
		lump_p->size = LittleLong (size);
	}
	buffer = malloc (size);
	SafeRead (datahandle, buffer, size);    

	// compress the lump if necessary
	if (lump_p->name[0] & 0x80)
	{
		compressed_buffer = encode(buffer, size, &compressed_size);
		if (compressed_size < size || allwayscompress)
		{	// allwayscompress allways uses compression, even if bad
			free(buffer);
			size = compressed_size;
			buffer = compressed_buffer;
			printf(" (%d compressed)", size);
		}
		else
		{
			free(compressed_buffer);
			lump_p->name[0] &= 0x7f;
			printf(" (doesn't compress well)");
		}
	}

	SafeWrite (newhandle, buffer, size);
	free (buffer);

	printf("\n");

	while (tell(newhandle)&3)
		SafeWrite (newhandle,buffer,1);
	
	if (!inwadfile)
		close (datahandle);

	lumpon++;
	lump_p++;
	lumpscopied++;
}

/*
=============================================================================

					   SCRIPT COMMANDS

=============================================================================
*/


/*
=================
=
= CmdCloseWad
=
=================
*/

void CmdCloseWad (void)
{
	if (!inwadfile)
		Error ("$CLOSEWAD issued without an open wad file\n");

	close (datahandle);
	free (wadlumps);

	inwadfile = false;
}


/*
=================
=
= CmdOpenWad
=
=================
*/

void CmdOpenWad (void)
{
	if (inwadfile)
		CmdCloseWad ();

//
// get and qualify wad file name
//
	GetToken (false);
	strcpy (loadwadpath, token);
	DefaultExtension (loadwadpath, ".wad");
	DefaultPath (loadwadpath, sourcepath);

//
// open it and read in header / lump directory
//
	datahandle = SafeOpenRead (loadwadpath);
	SafeRead (datahandle,&loadwad,sizeof(loadwad));
	loadwad.numlumps = LittleLong (loadwad.numlumps);
	loadwad.infotableofs = LittleLong (loadwad.infotableofs);

	if (strncmp(loadwad.identification,"IWAD",4))
		Error ("Wad file %s doesn't have IWAD id\n",lumppath);

	lseek (datahandle , loadwad.infotableofs , SEEK_SET);

	wadlumps = malloc ( loadwad.numlumps*sizeof(lumpinfo_t) );
	SafeRead (datahandle, wadlumps , loadwad.numlumps*sizeof(lumpinfo_t));

	inwadfile = true;
}


/*
=================
=
= CmdShootWad
=
= Copies every lump in a wad file into the output
=
=================
*/

void CmdShootWad (void)
{
	int             i;
	int		compressed;
	char            lumpname[9];


	CmdOpenWad ();
	compressed = CheckIfCompressed();

	for (i=0 ; i<loadwad.numlumps ; i++)
	{
		strncpy (lump_p->name , wadlumps[i].name , 8);
		lumpname[8] = 0;
		strncpy (lumpname, lump_p->name, 8);

		wadlump_p = wadlumps+i;

		strcpy (lumppath, loadwadpath);
		strcat (lumppath, " : ");
		strcat (lumppath, lumpname);

		if (compressed == 1 || ( compressed == 2 && (i&1) ) )
			lump_p->name[0] |= 0x80;
		if (compressed == 3 &&
		 i!=0 && i!=4 && i!=7 && i!=9 && i!=9 && i != 10)
			lump_p->name[0] |= 0x80;
		 
		CopyLump ();
	}

	CmdCloseWad ();

}


/*
===================
=
= CmdLabel
=
===================
*/

void CmdLabel (void)
{
	GetToken (false);

	lump_p->filepos = 0;
	lump_p->size = 0;

	strupr (token);
	strncpy(  lump_p->name, token, 8);

	printf ("%4i is LABEL: %s\n",lumpon,token);

	lumpon++;
	lump_p++;
}

/*
=====================
=
= CmdWadName
=
=====================
*/

void CmdWadName (char *name)
{
	if (outputnamed)
		Error ("Tried to rename the wad file");

//
// get the output filename
//
	strcpy (destpath,name);
	DefaultPath (destpath, defaultdestpath);
	DefaultExtension (destpath, ".wad");

	printf ("Output wad file: %s\n",destpath);

	newhandle = SafeOpenWrite (destpath);

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

	outputnamed = true;
	inwadfile = false;
}



//==========================================================================

/*
================
=
= CheckCommands
=
================
*/

boolean CheckCommands (void)
{
	if (token[0] != '$')
		return false;

//
// link commands
//
	if (!strcmpi(token,"$WADNAME") )
	{
		GetToken (false);
		CmdWadName (token);
		return true;
	}

	if (!strcmpi(token,"$OPENWAD") )
	{
		CmdOpenWad ();
		return true;
	}

	if (!strcmpi(token,"$SHOOTWAD") )
	{
		CmdShootWad ();
		return true;
	}

	if (!strcmpi(token,"$CLOSEWAD") )
	{
		CmdCloseWad ();
		return true;
	}

	if (!strcmpi(token,"$LABEL") )
	{
		CmdLabel ();
		return true;
	}

	Error ("Unrocognized command %s\n",token);
	return false;
}



/*
=============================================================================

						MAIN LOOP

=============================================================================
*/


/*
===================
=
= ProcessScript
=
===================
*/

void ProcessScript (void)
{
	lumpscopied = 0;
	outputnamed = false;

	while (1)
	{
	//
	// skip ahead to next command or lump name
	//
		GetToken (true);
		if (endofscript)
			break;                          // all done

		if ( CheckCommands () )
			continue;                       // linker command, not data lump

		if (!outputnamed)
			CmdWadName ("wadlink.wad");  // use default wad name

	//
	// get the lump name and lump pathname
	// if the lump doesn't have an extension, default to .LMP
	//
		strcpy (lumppath,token);
		DefaultPath (lumppath,sourcepath);
		DefaultExtension (lumppath,".lmp");
		ExtractFileBase (lumppath,lump_p->name);

		OpenLump ();                    // Find the lump data, either in a seperate file or in the comp file
		if (CheckIfCompressed())
			lump_p->name[0] |= 0x80;
		CopyLump ();                    // copy the lump to the output wad
	}

}


/*
===================
=
= WriteDirectory
=
===================
*/

void WriteDirectory (void)
{
//
// write lumpinfo table
//
	if (bigendian)
	{
		newwad.numlumps = BigLong (lumpon);
		newwad.infotableofs = BigLong (tell(newhandle));
	}
	else
	{
		newwad.numlumps = LittleLong (lumpon);
		newwad.infotableofs = LittleLong (tell(newhandle));
	}
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
	int	l;
	int     parmnum,parmsleft;
	char    scriptfilename[MAXPATH];

	printf ("\nWADLINK "VERSION" by John Carmack, copyright (c) 1992 Id Software\n");

	parmsleft = argc;

//
// check for help
//
	if (CheckParm ("?"))
	{
		printf (
"Usage: wadlink [-b] [-source path] [-dest path] [-script scriptfile]\n\n"
"-source path	To place the source for the files in another directory\n"
"-dest path	To place the linked wad file in another directory\n"
"-script file	The script name defaults to WADLINK.WL if not specified\n"
"-bigendian		To store the header and info table in big endian format\n"
);
		exit (1);
	}


	bigendian = CheckParm ("bigendian");
	if (bigendian)
	{
		printf ("Writing numbers BIG endian\n");
		parmsleft--;
	}
	else
		printf ("Writing numbers LITTLE endian\n");
		
//
// get source directory for data files
//
	parmnum = CheckParm ("source");

	if (parmnum)
	{
		strcpy (sourcepath,argv[parmnum+1]);
		parmsleft -= 2;
	}
	else
	{
		getcwd (sourcepath,MAXPATH);
	}
	if (sourcepath[strlen(sourcepath)-1] != PATHSEPERATOR)
	{
		l = strlen(sourcepath);
		sourcepath[l] = PATHSEPERATOR;
		sourcepath[l+1] = 0;
	}

	printf ("Source directory_____: %s\n",sourcepath);

//
// get destination directory for link file
//
	parmnum = CheckParm ("dest");

	if (parmnum)
	{
		strcpy (defaultdestpath,argv[parmnum+1]);
		parmsleft -= 2;
	}
	else
	{
		getcwd (defaultdestpath,MAXPATH);
	}
	if (defaultdestpath[strlen(defaultdestpath)-1] != PATHSEPERATOR)
	{
		l = strlen(defaultdestpath);
		defaultdestpath[l] = PATHSEPERATOR;
		defaultdestpath[l+1] = 0;
	}

	printf ("Destination directory: %s\n",defaultdestpath);

//
// get script file
//
	parmnum = CheckParm ("script");

	if (parmnum)
	{
		strcpy (scriptfilename,argv[parmnum+1]);
		parmsleft -= 2;
	}
	else
	{
		getcwd (scriptfilename,MAXPATH);
		strcat (scriptfilename,"/wadlink.wl");
	}
	printf ("Script file__________: %s\n",scriptfilename);
	LoadScriptFile (scriptfilename);

	if (parmsleft != 1)
		Error ("Improper parameters.  WADLINK -? for help.\n");

//
// start doing stuff
//
	ProcessScript ();
	WriteDirectory ();

	printf ("\n%u lumps copied.\n",lumpscopied);

	return 0;
}
