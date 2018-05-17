#include <stdio.h>

int main(int c, char **v)
{

	FILE *f1, *f2;
	int len1, len2;
	char *file1, *file2, *end1;
	int rc;

	if (c != 3)
	{
		fprintf(stderr, "Usage: cmpfile <file1> <file2>\n");
		exit(-1);
	}

	f1 = fopen(v[1], "r");
	f2 = fopen(v[2], "r");

	if (!f1)
	{
		fprintf(stderr, "Coule not open [%s]\n", v[1]);
		exit(-1);
	}
	if (!f2)
	{
		fprintf(stderr, "Coule not open [%s]\n", v[2]);
		exit(-1);
	}

	fseek(f1, 0, SEEK_END);
	len1 = ftell(f1);
	fseek(f1, 0, SEEK_SET);

	fseek(f2, 0, SEEK_END);
	len2 = ftell(f2);
	fseek(f2, 0, SEEK_SET);

	if (len1 != len2)
	{
		fprintf(stderr, "[%s] != [%s]\n", v[1], v[2]);
		exit(0);
	}

	file1 = (char *) malloc(len1);
	file2 = (char *) malloc(len2);

	rc = fread(file1, len1, 1, f1);
	if (rc != 1)
	{
		fprintf(stderr, "Could not read [%s]\n", v[1]);
		exit(-1);
	}
	rc = fread(file2, len2, 1, f2);
	if (rc != 1)
	{
		fprintf(stderr, "Could not read [%s]\n", v[2]);
		exit(-1);
	}

	end1 = file1 + len1;
	while (file1 != end1)
	{
		if (*file1 == 13) *file1 = ' ';
		if (*file2 == 13) *file2 = ' ';
		if (*file1++ != *file2++)
		{
			fprintf(stderr, "[%s] != [%s]\n", v[1], v[2]);
			exit(0);
		}
	}

	return 0;

}
