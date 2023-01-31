#ifndef HOOKING_H
#define HOOKING_H

#define LIBMPI "libmpi.so"
#define LIBSTD LIBM_SO

/*
 * smash_get_lib_func takes a null-terminated library name and a
 * null-terminated symbol name. On success returns the address where the symbol
 * is loaded in memory, on failure terminates the process.
 */
void *smash_get_lib_func(const char *lname, const char *fname);

#endif /* HOOKING_H */
