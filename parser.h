#ifndef PARSER_H
#define PARSER_H

#define CFG_DELAY_PATH "SMASH_MPI_DELAY_CONFIG_PATH"
#define CFG_FAILURE_PATH "SMASH_MPI_FAILURE_CONFIG_PATH"

enum CFG { CFG_DELAY, CFG_FAILURE };

void *smash_parse_cfg(enum CFG ctype);

struct cfg_delay {
	unsigned long int delay;
	unsigned int src, dst;
	int msg;
};

struct cfg_failure {
	unsigned long int time;
	unsigned int node;
};

/*
 * Structure tail padding optimization 
 * with flexible array member, valid in C99.
 *
 * To allocate do: 
 *
 *  struct cfg_delays *delays;
 *
 *  delays = malloc(sizeof(struct cfg_delays) + VECTOR_SIZE * sizeof(config_delay));
 *  delays->size = VECTOR_SIZE;
 *
 *  and to free everything do:
 *  free(delays)
 *
 *  Where VECTOR_SIZE is the number of lines in one config file.
 */

struct cfg_delays {
	unsigned int size;
	struct cfg_delay delays[];
};

struct cfg_failures {
	unsigned int size;
	struct cfg_failure failures[];
};

#endif /* PARSER_H */
