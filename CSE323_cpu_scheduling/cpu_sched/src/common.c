#include "common.h"

#include <stdlib.h>
#include <string.h>

void gantt_init(GanttChart *g) {
    free(g->seg);
    g->seg = NULL;
    g->len = 0;
    g->cap = 0;
}

void gantt_free(GanttChart *g) {
    free(g->seg);
    g->seg = NULL;
    g->len = g->cap = 0;
}

int gantt_push(GanttChart *g, int32_t pid, int32_t start, int32_t end) {
    if (g->len >= SCHED_MAX_GANTT_SEGMENTS)
        return -1;
    if (g->len == g->cap) {
        size_t ncap = g->cap ? g->cap * 2 : 64;
        GanttSegment *p = (GanttSegment *)realloc(g->seg, ncap * sizeof(GanttSegment));
        if (!p)
            return -1;
        g->seg = p;
        g->cap = ncap;
    }
    g->seg[g->len].pid = pid;
    g->seg[g->len].start = start;
    g->seg[g->len].end = end;
    g->len++;
    return 0;
}

void result_init(ScheduleResult *r, size_t nproc) {
    r->by_proc = (ProcMetrics *)calloc(nproc, sizeof(ProcMetrics));
    r->n = nproc;
    r->avg_waiting = 0.0;
    r->avg_turnaround = 0.0;
}

void result_free(ScheduleResult *r) {
    free(r->by_proc);
    r->by_proc = NULL;
    r->n = 0;
}

int process_states_from_specs(const ProcessSpec *specs, int n, ProcessState **out) {
    ProcessState *ps = (ProcessState *)calloc((size_t)n, sizeof(ProcessState));
    if (!ps)
        return -1;
    for (int i = 0; i < n; i++) {
        ps[i].pid = specs[i].pid;
        ps[i].arrival = specs[i].arrival;
        ps[i].burst_count = specs[i].burst_count;
        memcpy(ps[i].bursts, specs[i].bursts, sizeof(int32_t) * (size_t)specs[i].burst_count);
        ps[i].burst_idx = 0;
        ps[i].remaining = specs[i].burst_count > 0 ? specs[i].bursts[0] : 0;
        ps[i].priority = specs[i].priority;
        ps[i].total_cpu = specs[i].total_cpu;
        ps[i].started = 0;
        ps[i].completion = -1;
    }
    *out = ps;
    return 0;
}
