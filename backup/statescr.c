#define VERSION "1.1"


/*
=============================================================================

						DOOM STATESCR

						by John Carmack

=============================================================================
*/


#include "cmdlib.h"
#include "scriplib.h"


typedef struct
{
	long	sprite;
	long	frame;
	long	tics;
	long	action;
	long	nextstate;
	long	misc1, misc2;
} state_t;

#define MAXSTATES	1024
#define	MAXTHINKS	128

char	spritename[MAXSTATES][5];
int		numsprites;
char	actionname[MAXTHINKS][32];
int		numactions;


char	statename[MAXSTATES][32];
char	nextname[MAXSTATES][32];
state_t	states[MAXSTATES];
int		numstates;

/*
===================
=
= main
=
===================
*/

int main (void)
{
	FILE	*stenum;
	int		i,j;
	state_t	*st;

	printf ("\nSTATESCR "VERSION" by John Carmack, copyright (c) 1993 Id Software\n");
	printf ("processing statescr.ss\n");
	LoadScriptFile ("statescr.ss");


//
// parse state definitions
//
	strcpy (actionname[0], "NULL");
	numactions = 1;

	numstates = numsprites = 0;
	st = states;
	do
	{
		GetToken (true);
		if (endofscript)
			break;
		for (j=0 ; j<numstates ; j++)
			if (!strcmpi(token,statename[j]))
				Error ("line %i: duplicate state name (%s)",scriptline,token);
		strcpy (statename[numstates],token);

		GetToken (false);		// sprite
		for (j=0 ; j<numsprites ; j++)
			if (!strcmpi(token,spritename[j]))
				break;
		if (j==numsprites)
		{
			numsprites++;
			strcpy (spritename[j], token);
		}
		st->sprite = j;

		GetToken (false);		// frame
		st->frame = toupper(token[0])-'A';
		if (token[1] == '*')
		{
			st->frame |= 0x8000;	// full bright flag
			if (strlen (token) != 2)
				Error ("line %i: bad frame",scriptline);
		}
		else if (strlen (token) != 1)
			Error ("line %i: bad frame",scriptline);

		GetToken (false);		// tics
		st->tics = atoi (token);

		GetToken (false);		// action
		for (j=0 ; j<numactions ; j++)
			if (!strcmpi(token,actionname[j]))
				break;
		if (j==numactions)
		{
			numactions++;
			strcpy (actionname[j], token);
		}
		st->action = j;

		GetToken (false);		// next
		strcpy (nextname[numstates],token);	// resolved after reading all states

		if (TokenAvailable ())
		{
			GetToken (false);	// misc1
			st->misc1 = atoi (token);
			if (TokenAvailable ())
			{
				GetToken (false);	// misc2
				st->misc2 = atoi (token);
			}
		}

		numstates++;
		st++;
	} while (1);

//
// resolve next state numbers
//
	for (i=0 ; i<numstates ; i++)
	{
		for (j=0 ; j<numstates ; j++)
			if (!strcmpi(nextname[i],statename[j]))
				break;
		if (j==numstates)
			printf ("Unresolved nextstate: %s\n",nextname[i]);
		states[i].nextstate = j;
	}


//=============================================
//
// write states.h file
//
//=============================================

	stenum = fopen ("states.h","w");
	fprintf (stenum, "// generated by statescr\n\n");
#if 0
					 "#ifndef __P_LOCAL__\n"
					 "#include \"P_local.h\"\n"
					 "#endif\n\n");
#endif

//
// write sprite names
//
	fprintf (stenum,"typedef enum {\n");
	for (i=0 ; i<numsprites ; i++)
		fprintf (stenum,"SPR_%s,\n",spritename[i]);
	fprintf (stenum,"NUMSPRITES\n} spritenum_t;\n\n");

//
// write state names
//
	fprintf (stenum,"typedef enum {\n");
	for (i=0 ; i<numstates ; i++)
		fprintf (stenum,"%s,\n",statename[i]);
	fprintf (stenum,"NUMSTATES\n} statenum_t;\n\n");

	fprintf (stenum, "typedef struct\n"
					 "{\n"
					 "	 spritenum_t	sprite;\n"
					 "	 long			frame;\n"
					 "	 long			tics;\n"
					 "	 void			(*action) ();\n"
					 "	 statenum_t		nextstate;\n"
					 "	 long			misc1, misc2;\n"
					 "} state_t;\n\n");

	fprintf (stenum, "extern state_t	states[NUMSTATES];\n");
	fprintf (stenum, "extern char *sprnames[NUMSPRITES];\n\n");
	fclose (stenum);

//===========================================
//
// write states.c file
//
//===========================================
	stenum = fopen ("states.c","w");
	fprintf (stenum, "// generated by statescr\n\n");
	fprintf (stenum, "#include \"states.h\"\n");
	fprintf (stenum, "#include <stdlib.h>\n\n");

//
// write sprite names for initial sprite loading
//
	fprintf (stenum, "char *sprnames[NUMSPRITES] = {\n");
	for (i=0 ; i<numsprites ; i++)
	{
		fprintf (stenum, "\"%s\"", spritename[i]);
		if (i != numsprites-1)
		{
			fprintf (stenum, ",");
			if (i%10 == 9)
				fprintf (stenum, "\n");
		}
	}
	fprintf (stenum, "\n};\n\n");

//
// write action names
//
	for (i=1 ; i<numactions ; i++)	// skip NULL at 0
		fprintf (stenum,"void %s ();\n", actionname[i]);
	fprintf (stenum,"\n");

//
// write state structures
//
	fprintf (stenum, "state_t	states[NUMSTATES] = {\n");
	for (i=0, st=states ; i<numstates ; i++,st++)
	{
		fprintf (stenum, "{SPR_%s,%li,%li,%s,%s,%li,%li}",
		spritename[st->sprite], st->frame,
		st->tics, actionname[st->action], statename[st->nextstate],
		st->misc1, st->misc2);
		if (i != numstates-1)
			fprintf (stenum, ",");
		fprintf (stenum, "\t// %s\n",statename[i]);
	}
	fprintf (stenum, "};\n\n");

	fclose (stenum);



	printf ("%u states parsed\n",numstates);

	return 0;
}
