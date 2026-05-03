#include "metrics.h"

#include <stdlib.h>

static int cmp_int32(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a;
    int32_t y = *(const int32_t *)b;
    if (x < y)
        return -1;
    if (x > y)
        return 1;
    return 0;
}

void metrics_compute(ProcessState *ps, int n, GanttChart *g, ScheduleResult *out) {
    (void)g;
    int32_t *pids = (int32_t *)malloc((size_t)n * sizeof(int32_t));
    for (int i = 0; i < n; i++)
        pids[i] = ps[i].pid;
    qsort(pids, (size_t)n, sizeof(int32_t), cmp_int32);

    result_init(out, (size_t)n);
    double sumw = 0.0, sumt = 0.0;
    for (int k = 0; k < n; k++) {
        int32_t want = pids[k];
        int idx = -1;
        for (int i = 0; i < n; i++) {
            if (ps[i].pid == want) {
                idx = i;
                break;
            }
        }
        if (idx < 0)
            continue;
        int32_t ct = ps[idx].completion;
        int32_t ta = ps[idx].arrival;
        int32_t tt = ct - ta;
        int32_t wt = tt - ps[idx].total_cpu;
        out->by_proc[k].pid = want;
        out->by_proc[k].completion = ct;
        out->by_proc[k].turnaround = tt;
        out->by_proc[k].waiting = wt;
        sumw += (double)wt;
        sumt += (double)tt;
    }
    out->avg_waiting = sumw / (double)n;
    out->avg_turnaround = sumt / (double)n;
    free(pids);
}

void metrics_print(FILE *fp, const ScheduleResult *r, const ProcessState *ps, int n) {
    (void)ps;
    (void)n;
    fprintf(fp, "Per-process metrics (sorted by pid):\n");
    for (size_t i = 0; i < r->n; i++) {
        fprintf(fp, "  pid %d: completion=%d turnaround=%d waiting=%d\n", (int)r->by_proc[i].pid,
                (int)r->by_proc[i].completion, (int)r->by_proc[i].turnaround, (int)r->by_proc[i].waiting);
    }
    fprintf(fp, "Averages: avg_waiting=%.3f avg_turnaround=%.3f\n", r->avg_waiting, r->avg_turnaround);
}

void gantt_print_ascii(FILE *fp, const GanttChart *g) {
    if (g->len == 0) {
        fprintf(fp, "Gantt: (empty)\n");
        return;
    }
    fprintf(fp, "Gantt (pid per time unit, '|' = tick):\n|");
    for (size_t i = 0; i < g->len; i++) {
        int32_t a = g->seg[i].start;
        int32_t b = g->seg[i].end;
        for (int32_t t = a; t < b; t++) {
            fprintf(fp, "%d|", (int)g->seg[i].pid);
        }
    }
    fprintf(fp, "\n");
}
