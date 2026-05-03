#ifndef SCHED_METRICS_H
#define SCHED_METRICS_H

#include "common.h"

#include <stdio.h>

/* Fill result from completed process states and chart; maps metrics by array order (sorted by pid). */
void metrics_compute(ProcessState *ps, int n, GanttChart *g, ScheduleResult *out);

void metrics_print(FILE *fp, const ScheduleResult *r, const ProcessState *ps, int n);
void gantt_print_ascii(FILE *fp, const GanttChart *g);

#endif
