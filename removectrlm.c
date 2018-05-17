#include <stdio.h>
#include <string.h>

void convertfile(char *filename)
{

	FILE *f;
	int len, rc;
	char *file, *end;

	f = fopen(filename, "r+");
	if (!f)
	{
		fprintf(stderr, "Could not open [%s]\n", filename);
		exit(-1);
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);

	file = (char *) malloc(len);
	rc = fread(file, len, 1, f);
	if (rc != 1) 
	{
		fprintf(stderr, "Could not read [%s]\n", filename);
		exit(-1);
	}

	end = file + len;
	while (file != end)
	{
		if (*file == 13) *file = ' ';
		file++;
	}
	file -= len;

	rewind(f);

	if (fwrite(file, len, 1, f) != 1)
	{
		fprintf(stderr, "Could not write back [%s]\n", filename);
		return;
	}

	if (fclose(f))
	{
		fprintf(stderr, "Be worried.  I couldn't fclose() [%s]\n", filename);
	}

}

int main(int c, char **v)
{

	int i, len;

	for (i=1 ; i<c ; i++)
	{
		len = strlen(v[i]);
		if (v[i][len-2] == '.')
		{
			if (v[i][len-1] == 'h' || v[i][len-1] == 'c')
			convertfile(v[i]);
		}
	}

	return 0;

}

