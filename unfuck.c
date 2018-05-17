#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/types.h>

int argc;
char **argv;
int recursive;

int	chmod(char *, int);

void usage(void)
{
	fprintf(stderr, "\n"
					"Usage: unfuck [-r] <file or directory names...>\n"
					"\n"
					"       Unfucks file permissions\n"
					"       (-r : recursively descends subdirectories)\n"
					"\n"
			);
	exit(-1);
}


int parmloc(char *parm)
{
	int argnum;
	for (argnum=argc - 1 ; argnum ; argnum--)
		if (!strcmp(argv[argnum], parm))
		{
			argv[argnum] = 0;
			break;
		}
	return argnum;
}

int argloc(void)
{
	int argloc = argc - 1;
	while (argv[argloc] && argloc)
		argloc--;
	return argloc+1;
}

void derror(char *str, ...)
{
	va_list args;
	va_start(args, str);
	fprintf(stderr, "error: ");
	vfprintf(stderr, str, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(-1);
}

void unfuckfilename(char *filename, int treatasfile)
{

	struct stat st;

	lstat(filename, &st);

	if ((st.st_mode & S_IFMT) == S_IFDIR && !treatasfile)
	{
		DIR *dir;
		struct direct *ent;
		char *subname;

		dir = opendir(filename);
		if (!dir)
			derror("Could not open directory %s\n", filename);
		while ((ent = readdir(dir)))
		{
			subname = alloca(strlen(ent->d_name) + strlen(filename) + 2);
			sprintf(subname, "%s/%s", filename, ent->d_name);
			if (!strcmp(ent->d_name, "."))
				unfuckfilename(subname, 1);
			else if (strcmp(ent->d_name, ".."))
				unfuckfilename(subname, 0);
		}
		closedir(dir);
	}
	else
	{
		if (st.st_mode & 0100)
			st.st_mode |= 0111;
		st.st_mode |= 0666;
		chmod(filename, st.st_mode);
	}

}

int main(int c, char **v)
{

	int arglist;

	argc = c;
	argv = v;

	recursive = parmloc("-r");

	arglist = argloc();
	if (arglist == argc)
		usage();

	while (arglist < argc)
	{
		unfuckfilename(argv[arglist], !recursive);
		arglist++;
	}

	return 0;

}
