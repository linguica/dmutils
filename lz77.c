#include "cmdlib.h"
#include "lzlib.h"

main(int c, char **v)
{

	char *input, *output;
	int ilen, olen;

	ilen = LoadFile(v[1], &input);
	output = encode(input, ilen, &olen);
	LoadFile(v[1], &input);
	SaveFile ("o", output, olen);

}

