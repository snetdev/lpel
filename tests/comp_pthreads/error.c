#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void error(int exitcode, int val, const char *msg)
{
    fprintf(stderr, "%s (%s, %d)\n", msg, strerror(errno), val);
    exit(exitcode);
}
