#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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

static void *
smash_parse_delay_cfg(char *data, size_t data_size)
{
	int ret, line;
	regex_t r;
	regmatch_t rm[5];
	char err_buf[100];
	char *pt;

	const char *rs = "([0-9]+);([0-9]+);([0-9]+);(-?[0-9]+)";

	if ((ret = regcomp(&r, rs, REG_EXTENDED)) != 0) {
		regerror(ret, &r, err_buf, 100);
		fprintf(stderr, "failed to compile regex <%s>:%s\n", rs, err_buf);
		goto err;
	}

	pt = data;
	line = 1;
	while(!(ret = regexec(&r, pt, 3, rm, 0))) {
		pt += rm[0].rm_eo + 1;
		line++;
		// TODO: populate structure for delays
	}
	if (pt) {
		regerror(ret, &r, err_buf, 100);
		fprintf(stderr, "line %d: %s\n", line, err_buf);
		goto err;
	}

err:
	regfree(&r);
	munmap(data, data_size);
	return NULL;
}

static void *
smash_parse_failure_cfg(char *data, size_t data_size)
{
	int ret, line;
	regex_t r;
	regmatch_t rm[3];
	char err_buf[100];
	char *pt;

	const char *rs = "([0-9]+);([0-9]+)";

	if ((ret = regcomp(&r, rs, REG_EXTENDED)) != 0) {
		regerror(ret, &r, err_buf, 100);
		fprintf(stderr, "failed to compile regex <%s>:%s\n", rs, err_buf);
		goto err;
	}

	pt = data;
	line = 1;
	while(!(ret = regexec(&r, pt, 3, rm, 0))) {
		pt += rm[0].rm_eo + 1;
		line++;
		// TODO: populate structure for failures
	}
	if (pt) {
		regerror(ret, &r, err_buf, 100);
		fprintf(stderr, "line %d: %s\n", line, err_buf);
		goto err;
	}

err:
	regfree(&r);
	munmap(data, data_size);
	return NULL;
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
	size_t data_size;
	char *config_path;

	if (!(config_path = smash_get_config_path(ctype)))
		goto err;

	if (!(data = smash_load_file_in_memory(config_path, &data_size)))
		goto err;

	switch (ctype) {
	case CFG_DELAY:
		return smash_parse_delay_cfg(data, data_size);
	case CFG_FAILURE:
		return smash_parse_failure_cfg(data, data_size);
	default:
		/* never reached */
		return NULL;
	}
err:
	return NULL;
}
