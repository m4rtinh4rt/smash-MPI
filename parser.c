#include "parser.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void
smash_parse_cfg(const char *filename, int *res)
{
	struct stat sb;
	char *file, *tok;
	int fd;
	int i;

	fd = open(filename, O_RDONLY, S_IRUSR | S_IWUSR);
	if (fstat(fd, &sb) == -1) {
		perror("can't get file size.\n");
	}

	file = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	char s[sb.st_size + 1];
	strcpy(s, file);
	close(fd);

	tok = strtok(s, ";");
	i = 0;
	while (tok) {
		res[i++] = atoi(tok);
		tok = strtok(NULL, ";");
	}
}
