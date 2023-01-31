#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

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
	munmap(data, data_size);
	return NULL;
}

static void *
smash_parse_failure_cfg(char *data, size_t data_size)
{
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
