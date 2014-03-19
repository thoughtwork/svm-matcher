#ifndef _h_MPR
#define _h_MPR

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#define MPR_LINE(s)         #s
#define MPR_LINE2(s)        MPR_LINE(s)
#define MPR_LINE3           MPR_LINE2(__LINE__)
#define MPR_LOC             __FILE__ ":" MPR_LINE3

static inline void mprAssert(const char *loc, const char *msg)
{
	printf("Assertion \"%s\", failed at \"%s\".\n", msg, loc);
    exit(EXIT_FAILURE);
}

#undef assert
#define assert(C)       if (C) ; else mprAssert(MPR_LOC, #C)

#endif /* _h_MPR */
