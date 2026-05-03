#ifndef SCHED_SCHEDULER_H
#define SCHED_SCHEDULER_H

#include "common.h"
#include "predictor.h"

#include <stdio.h>

/* Runs the selected algorithm on a fresh copy of specs. Writes gantt and metrics to out. */
int sched_run(const ProcessSpec *specs, int n, const SchedParams *params, GanttChart *gantt, ScheduleResult *out, FILE *predict_log);

#endif
