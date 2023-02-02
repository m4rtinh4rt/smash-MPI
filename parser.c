#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

#include "parser.h"

static void *
smash_load_file_in_memory(const char *filename, size_t *data_size)
{
	int fd;
	char *data;
	struct stat finfo;

	if ((fd = open(filename, O_RDONLY, S_IRUSR | S_IWUSR)) < 0)
		goto err;

	if (fstat(fd, &finfo) < 0)
		goto err;

	if ((data = mmap(NULL, finfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0))
			== MAP_FAILED)
		goto err;

	*data_size = finfo.st_size;
	close(fd);
	return data;
err:
	close(fd);
	return NULL;
}

static void
smash_populate_delay(/*structure pointer to populate, regmatch_t *rm*/)
{
	// TODO
}

static void
smash_populate_failure(/*structure pointer to populate, regmatch_t *rm*/)
{
	// TODO
}

static char *
smash_get_config_path(enum CFG ctype)
{
	return getenv((ctype == CFG_DELAY ? CFG_DELAY_PATH : CFG_FAILURE_PATH));
}

void *
smash_parse_cfg(enum CFG ctype)
{
	void *data;
	int ret;
	size_t data_size, nline, n_cfg;
	char *config_path, *line, err_buf[100];
	const char *rs;
	regex_t r;
	regmatch_t rm[5];
	void (*f)(/*struct pointer, regmatch_t*/);

	if (!(config_path = smash_get_config_path(ctype)))
		return NULL;

	if (!(data = smash_load_file_in_memory(config_path, &data_size)))
		return NULL;

	if (ctype == CFG_DELAY) {
		rs = "([0-9]+);([0-9]+);([0-9]+);(-?[0-9]+)";
		n_cfg = 5;
		f = smash_populate_delay;
	} else {
		rs = "([0-9]+);([0-9]+)";
		n_cfg = 3;
		f = smash_populate_failure;
	}

	if ((ret = regcomp(&r, rs, REG_EXTENDED)) != 0) {
		regerror(ret, &r, err_buf, 100);
		fprintf(stderr, "failed to compile regex <%s>:%s\n", rs,
			err_buf);
		goto err;
	}

	nline = 1;
	line = strtok(data, "\n");
	while (line) {
		// if line is a comment or snaphot, do smth
		if ((ret = regexec(&r, line, n_cfg, rm, 0)) != 0) {
			regerror(ret, &r, err_buf, 100);
			fprintf(stderr, "line %ld: %s\n", nline, err_buf);
			goto err;
		}
		// TODO: call f(&struct to populate)
		nline++;
		line = strtok(NULL, "\n");
	}

	regfree(&r);
	munmap(data, data_size);
	// TODO: return structure
err:
	regfree(&r);
	munmap(data, data_size);
	return NULL;
}
