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

	if ((data = mmap(NULL, finfo.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0))
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
smash_populate_delay(const char *line, size_t n, const regmatch_t *rm, struct cfg_delays *delays)
{
	delays->delays[n].src = strtol(line + rm[1].rm_so, NULL, 10);
	delays->delays[n].dst = strtol(line + rm[2].rm_so, NULL, 10);
	delays->delays[n].delay = strtol(line + rm[3].rm_so, NULL, 10);
	delays->delays[n].msg = strtol(line + rm[4].rm_so, NULL, 10);
}

static void
smash_populate_failure(const char *line, size_t n, const regmatch_t *rm, struct cfg_failures *failures)
{
	failures->failures[n].node = strtol(line + rm[1].rm_so, NULL, 10);
	failures->failures[n].time = strtol(line + rm[2].rm_so, NULL, 10);
}

static char *
smash_get_config_path(enum CFG ctype)
{
	return getenv((ctype == CFG_DELAY ? CFG_DELAY_PATH : CFG_FAILURE_PATH));
}

static int
count_lines(const char *rs)
{
	int lines = 0;
	while (*(rs)++ != '\0')
		if (*rs == '\n' || *rs == '\r')
			lines++;
	return lines;
}

void *
smash_parse_cfg(enum CFG ctype)
{
	struct cfg_delays *delays;
	struct cfg_failures *failures;
	void *data;
	int ret, lines;
	size_t data_size, nline, n_cfg;
	char *config_path, *line, err_buf[100];
	const char *rs;
	regex_t r;
	regmatch_t rm[5];
	void (*f)();
	void *cfg;

	if (!(config_path = smash_get_config_path(ctype)))
		return NULL;

	if (!(data = smash_load_file_in_memory(config_path, &data_size)))
		return NULL;

	lines = count_lines(data);
	cfg = malloc(ctype == CFG_DELAY ? sizeof(struct cfg_delays) +
					      lines * sizeof(struct cfg_delay)
					: sizeof(struct cfg_failures) +
					      lines * sizeof(struct cfg_delay));

	if (ctype == CFG_DELAY) {
		delays = cfg;
		delays->size = lines;
		rs = "([0-9]+);([0-9]+);([0-9]+);(-?[0-9]+)";
		n_cfg = 5;
		f = smash_populate_delay;
	} else {
		failures = cfg;
		failures->size = lines;
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

	nline = 0;
	line = strtok(data, "\n");
	while (line) {
		// if line is a comment or snaphot, do smth
		if ((ret = regexec(&r, line, n_cfg, rm, 0)) != 0) {
			regerror(ret, &r, err_buf, 100);
			fprintf(stderr, "line %ld: %s\n", nline + 1, err_buf);
			goto err;
		}
		f(line, nline, rm, cfg);
		nline++;
		line = strtok(NULL, "\n");
	}

	regfree(&r);
	munmap(data, data_size);
	return cfg;
err:
	regfree(&r);
	munmap(data, data_size);
	return NULL;
}
