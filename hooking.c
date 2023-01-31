#include <dlfcn.h>
#include <err.h>
#include <gnu/lib-names.h>
#include <stdlib.h>

void *
smash_get_lib_func(const char *lname, const char *fname)
{
	void *lib, *p;

	if (!(lib = dlopen(lname, RTLD_LAZY)))
		err(EXIT_FAILURE, "%s", dlerror());

	if (!(p = dlsym(lib, fname)))
		err(EXIT_FAILURE, "%s", dlerror());

	dlclose(lib);
	return p;
}
