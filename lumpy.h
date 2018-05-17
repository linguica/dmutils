#include "cmdlib.h"
#include "scriplib.h"
#include "lbmlib.h"

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

extern  byte            *byteimage, *lbmpalette;
extern  int                     byteimagewidth, byteimageheight;

extern  boolean lumppause;
extern  byte            *lump_p;

extern  byte            *lumpbuffer;

void GrabLumpyScript (char const *script, boolean wadfile, boolean dopause);

void WriteLump (void);

extern	char	lumpname[9];            // lump names are 8 chars max

