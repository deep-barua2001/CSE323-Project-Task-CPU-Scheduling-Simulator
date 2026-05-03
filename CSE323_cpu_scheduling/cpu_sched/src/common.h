#ifndef SCHED_COMMON_H
#define SCHED_COMMON_H

#include <stddef.h>
#include <stdint.h>

/*
 * Priority: smaller integer = higher priority (more urgent).
 * Tie-break for scheduling: lower process id wins.
 *
 * Each process may have one or more CPU bursts (back-to-back segments).
 * After completing a burst except the last, the job returns to the ready
 * queue (zero I/O time) so prediction can influence the next scheduling
 * decision.
 */

#define SCHED_MAX_PROCESSES 256
#define SCHED_MAX_BURSTS 32
#define SCHED_MAX_LINE 1024
#define SCHED_MAX_GANTT_SEGMENTS 4096

typedef enum {
    SCHED_ALGO_FCFS = 0,
    SCHED_ALGO_RR,
    SCHED_ALGO_PRIORITY,
    SCHED_ALGO_SJF,
    SCHED_ALGO_SRTF,
    SCHED_ALGO_PSJF
} SchedAlgo;

typedef struct {
    int32_t pid;
    int32_t arrival;
    int32_t bursts[SCHED_MAX_BURSTS];
    int burst_count;
    int32_t priority;
    int32_t total_cpu; /* sum of bursts, for metrics */
} ProcessSpec;

typedef struct {
    int32_t pid;
    int32_t arrival;
    int32_t bursts[SCHED_MAX_BURSTS];
    int burst_count;
    int burst_idx;     /* index of current CPU segment */
    int32_t remaining; /* remaining time in current segment */
    int32_t priority;
    int32_t total_cpu;
    int started;       /* first time CPU assigned */
    int32_t completion;
} ProcessState;

typedef struct {
    int32_t pid;
    int32_t start;
    int32_t end;
} GanttSegment;

typedef struct {
    GanttSegment *seg;
    size_t len;
    size_t cap;
} GanttChart;

typedef struct {
    SchedAlgo algo;
    int32_t quantum;
    double alpha;  /* exponential averaging */
    double tau0;   /* initial guess when no history */
} SchedParams;

typedef struct {
    int32_t pid;
    int32_t completion;
    int32_t turnaround;
    int32_t waiting;
} ProcMetrics;

typedef struct {
    ProcMetrics *by_proc;
    size_t n;
    double avg_waiting;
    double avg_turnaround;
} ScheduleResult;

void gantt_init(GanttChart *g);
void gantt_free(GanttChart *g);
int gantt_push(GanttChart *g, int32_t pid, int32_t start, int32_t end);

void result_init(ScheduleResult *r, size_t nproc);
void result_free(ScheduleResult *r);

/* Deep-copy specs to mutable states (remaining set from first burst). */
int process_states_from_specs(const ProcessSpec *specs, int n, ProcessState **out);

#endif
