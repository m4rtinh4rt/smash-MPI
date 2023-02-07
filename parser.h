#ifndef PARSER_H
#define PARSER_H

#define CFG_DELAY_PATH "SMASH_MPI_DELAY_CONFIG_PATH"
#define CFG_FAILURE_PATH "SMASH_MPI_FAILURE_CONFIG_PATH"

enum CFG { CFG_DELAY, CFG_FAILURE };

void *smash_parse_cfg(enum CFG ctype);

#endif /* PARSER_H */
