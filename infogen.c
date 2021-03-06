#define VERSION "1.0"

/*
=============================================================================

							DOOM INFOGEN

							by John Carmack

=============================================================================
*/


#include "cmdlib.h"
#include "scriplib.h"

#define		MAXTYPES	128
#define	MAXINFO		32


typedef struct
{
	char	*name;
	char	*base;
} info_t;

info_t	baseinfo[MAXINFO];

char	typename[MAXTYPES][32];
char	*info[MAXTYPES][MAXINFO];
int		numtypes;
int		numinfo;
int		nummisc;

/*
===================
=
= main
=
===================
*/

int main (void)
{
	FILE	*out;
	int		i,j;

	printf ("\nINFOGEN "VERSION" by John Carmack, copyright (c) 1993 Id Software\n");
	printf ("processing infogen.in\n");
	LoadScriptFile ("infogen.in");

//
// parse defaults
//
	GetToken (true);
	if (token[0] != '$')
		Error ("first command must be $ DEFAULT");
	GetToken (false);
	if (strcmp (token,"DEFAULT"))
		Error ("first command must be $ DEFAULT");
	numinfo = 0;
	do
	{
		GetToken (true);
		if (endofscript)
			Error ("end of script in defaults");
		if (token[0] == '$')
		{
			UnGetToken ();
			break;
		}
		baseinfo[numinfo].name = malloc (strlen(token)+1);
		strcpy (baseinfo[numinfo].name, token);
		GetToken (false);
		baseinfo[numinfo].base = malloc (strlen(token)+1);
		strcpy (baseinfo[numinfo].base, token);
		numinfo++;
	} while (1);
		
//
// parse commands
//
	numtypes = 0;

	do
	{
		GetToken (true);
		if (endofscript)
			break;
		if (token[0] == '$')
		{
			GetToken (false);
			if (token[0] == '+')
			{
				sprintf (typename[numtypes],"MT_MISC%i",nummisc);
				nummisc++;
			}
			else
				strcpy (typename[numtypes], token);
			numtypes++;
			continue;
		}
		if (!numtypes)
			Error ("A type mnust be defined before declaring info");
		// find which field name
		for (i=0 ; i<numinfo ; i++)
			if (!strcmp (token, baseinfo[i].name))
				break;
		if (i==numinfo)
			Error ("Unknown info type %s",token);
		GetToken (false);
		info[numtypes-1][i] = malloc(strlen(token)+1);
		strcpy (info[numtypes-1][i], token);
	} while (1);


//===========================================
//
// write mobjinfo.h file
//
//===========================================
	out = fopen ("mobjinfo.h","w");
	fprintf (out, "// generated by makeinfo\n\n");
	fprintf (out, "typedef enum {\n");
	for (i=0 ; i<numtypes ; i++)
		fprintf (out,"%s,\n",typename[i]);
	fprintf (out, "NUMMOBJTYPES} mobjtype_t;\n\n");
	fprintf (out, "typedef struct {\n");
	for (j=0 ; j<numinfo ; j++)
		fprintf (out, "	int	%s;\n", baseinfo[j].name );
	fprintf (out, "} mobjinfo_t;\n\n");
	fprintf (out, "extern mobjinfo_t mobjinfo[NUMMOBJTYPES];\n\n");
	fclose (out);
	
//===========================================
//
// write mobjinfo.c file
//
//===========================================
	out = fopen ("mobjinfo.c","w");
	fprintf (out, "// generated by makeinfo\n\n");
	fprintf (out, "#include \"DoomDef.h\"\n\n");
	fprintf (out, "mobjinfo_t mobjinfo[NUMMOBJTYPES] = {\n");
	for (i=0 ; i<numtypes ; i++)
	{
		fprintf (out,"\n{		// %s\n", typename[i]);
		for (j=0 ; j<numinfo ; j++)
		{
			if (info[i][j])
				fprintf (out, "%s", info[i][j] );
			else
				fprintf (out, "%s", baseinfo[j].base );
			
			if (j != numinfo-1)
				fprintf (out, ",");
			fprintf (out,"		// %s\n", baseinfo[j].name);
		}
		fprintf (out, " }");
		if (i != numtypes-1)
			fprintf (out, ",");
		fprintf (out, "\n");
	}
	fprintf (out, "};\n\n");
	fclose (out);
	
	printf ("%i types parsed\n", numtypes);
	return 0;
}

