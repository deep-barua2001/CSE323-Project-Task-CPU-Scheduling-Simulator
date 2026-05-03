#ifndef SCHED_PARSE_H
#define SCHED_PARSE_H

#include "common.h"

#include <stdio.h>

typedef struct {
    SchedParams params;
    ProcessSpec procs[SCHED_MAX_PROCESSES];
    int nproc;
    char error[256];
} SchedInput;

/* Read and parse entire file. Returns 0 on success, -1 on error (see in->error). */
int sched_parse_file(FILE *f, SchedInput *in);

#endif
