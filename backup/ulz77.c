#include "cmdlib.h"
#include "lzlib.h"

main(int c, char **v)
{

	char *input, *output;
	int ilen, olen;

	ilen = LoadFile("o", &input);
	olen = atoi(v[2]);
	output = malloc(olen);
	decode(input, output);
	SaveFile (v[1], output, olen);

}

