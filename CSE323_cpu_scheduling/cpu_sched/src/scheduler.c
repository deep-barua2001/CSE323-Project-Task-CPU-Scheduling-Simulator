/*
 * Simulation assumptions:
 * - Single CPU. Time is discrete integers.
 * - FCFS / SJF / PRIORITY / PSJF: non-preemptive per CPU burst segment.
 * - After each CPU burst except the last, a job re-enters the ready queue at
 *   the completion time (zero I/O), so multi-burst traces exercise scheduling.
 * - Round Robin preempts only on quantum expiry (new arrivals do not preempt).
 * - SRTF is preemptive on the smallest remaining time of the current segment.
 * - Tie-break: lower process id wins when ties on stamp, remaining, priority, or tau.
 */
#include "scheduler.h"
#include "metrics.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ProcessState *ps;
    int n;
    int in_ready[SCHED_MAX_PROCESSES];
    int32_t ready_stamp[SCHED_MAX_PROCESSES];
    int running;
    GanttChart *gantt;
    SchedParams params;
    PredictorState *pred;
    FILE *plog;
    int32_t seg_start_len[SCHED_MAX_PROCESSES]; /* remaining at segment dispatch */
} Sim;

static int all_done(const ProcessState *ps, int n) {
    for (int i = 0; i < n; i++) {
        if (ps[i].completion < 0)
            return 0;
    }
    return 1;
}

static int next_arrival_after(const ProcessState *ps, int n, int32_t t) {
    int32_t best = INT32_MAX;
    for (int i = 0; i < n; i++) {
        if (ps[i].completion >= 0)
            continue;
        if (ps[i].arrival > t && ps[i].arrival < best)
            best = ps[i].arrival;
    }
    return best == INT32_MAX ? -1 : (int)best;
}

static void sim_add_arrivals(Sim *s, int32_t t) {
    for (int i = 0; i < s->n; i++) {
        if (s->ps[i].completion >= 0)
            continue;
        if (s->in_ready[i])
            continue;
        if (i == s->running)
            continue;
        if (s->ps[i].remaining <= 0)
            continue;
        if (s->ps[i].arrival <= t) {
            s->in_ready[i] = 1;
            s->ready_stamp[i] = s->ps[i].arrival;
        }
    }
}

static int pick_fcfs_tuple(const Sim *s) {
    int best = -1;
    for (int i = 0; i < s->n; i++) {
        if (!s->in_ready[i])
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        if (s->ready_stamp[i] < s->ready_stamp[best] ||
            (s->ready_stamp[i] == s->ready_stamp[best] && s->ps[i].pid < s->ps[best].pid))
            best = i;
    }
    return best;
}

static int pick_priority(const Sim *s) {
    int best = -1;
    for (int i = 0; i < s->n; i++) {
        if (!s->in_ready[i])
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        if (s->ps[i].priority < s->ps[best].priority ||
            (s->ps[i].priority == s->ps[best].priority && s->ps[i].pid < s->ps[best].pid))
            best = i;
    }
    return best;
}

static int pick_sjf_oracle(const Sim *s) {
    int best = -1;
    for (int i = 0; i < s->n; i++) {
        if (!s->in_ready[i])
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        if (s->ps[i].remaining < s->ps[best].remaining ||
            (s->ps[i].remaining == s->ps[best].remaining && s->ps[i].pid < s->ps[best].pid))
            best = i;
    }
    return best;
}

static int pick_psjf_pred(const Sim *s) {
    int best = -1;
    for (int i = 0; i < s->n; i++) {
        if (!s->in_ready[i])
            continue;
        double ti = predictor_key_tau(s->pred, i);
        if (best < 0) {
            best = i;
            continue;
        }
        double tb = predictor_key_tau(s->pred, best);
        if (ti < tb || (ti == tb && s->ps[i].pid < s->ps[best].pid))
            best = i;
    }
    return best;
}

static int pick_srtf(const Sim *s) {
    int best = -1;
    int32_t best_rem = INT32_MAX;
    int32_t best_pid = INT32_MAX;
    for (int i = 0; i < s->n; i++) {
        if (s->ps[i].completion >= 0)
            continue;
        if (s->ps[i].remaining <= 0)
            continue;
        int active = (i == s->running) || s->in_ready[i];
        if (!active)
            continue;
        int32_t r = s->ps[i].remaining;
        int32_t pid = s->ps[i].pid;
        if (r < best_rem || (r == best_rem && pid < best_pid)) {
            best_rem = r;
            best_pid = pid;
            best = i;
        }
    }
    return best;
}

static void mark_running(Sim *s, int j) {
    s->in_ready[j] = 0;
    s->running = j;
    if (!s->ps[j].started) {
        s->ps[j].started = 1;
    }
    s->seg_start_len[j] = s->ps[j].remaining;
}

static int finish_segment(Sim *s, int j, int32_t t_end, PredictorState *pred, double alpha, FILE *plog) {
    int32_t actual = s->seg_start_len[j];
    if (pred) {
        predictor_on_burst_done(pred, j, actual, alpha);
        if (plog) {
            fprintf(plog, "pid %d finished CPU segment len=%d -> next_tau=%.4f\n", (int)s->ps[j].pid, (int)actual,
                    pred[j].tau);
        }
    }
    s->ps[j].burst_idx++;
    if (s->ps[j].burst_idx >= s->ps[j].burst_count) {
        s->ps[j].completion = t_end;
        s->running = -1;
        return 0;
    }
    s->ps[j].remaining = s->ps[j].bursts[s->ps[j].burst_idx];
    s->running = -1;
    s->in_ready[j] = 1;
    s->ready_stamp[j] = t_end;
    return 1;
}

/* --- RR queue (FIFO, small n) --- */
typedef struct {
    int q[SCHED_MAX_PROCESSES * 8];
    int n;
} RRQ;

static void rrq_init(RRQ *q) {
    q->n = 0;
}

static int rrq_empty(const RRQ *q) {
    return q->n == 0;
}

static void rrq_push(RRQ *q, int v) {
    if (q->n >= (int)(sizeof(q->q) / sizeof(q->q[0])))
        return;
    q->q[q->n++] = v;
}

static int rrq_pop(RRQ *q) {
    int v = q->q[0];
    if (q->n > 1)
        memmove(q->q, q->q + 1, (size_t)(q->n - 1) * sizeof(q->q[0]));
    q->n--;
    return v;
}

static int rrq_contains(const RRQ *q, int v) {
    for (int k = 0; k < q->n; k++) {
        if (q->q[k] == v)
            return 1;
    }
    return 0;
}

static void sim_run_fcfs_sjf_prio_psjf(Sim *s, int use_pred) {
    int32_t t = 0;
    s->running = -1;
    memset(s->in_ready, 0, sizeof(s->in_ready[0]) * (size_t)s->n);

    while (!all_done(s->ps, s->n)) {
        sim_add_arrivals(s, t);

        if (s->running < 0) {
            int pick;
            if (s->params.algo == SCHED_ALGO_FCFS)
                pick = pick_fcfs_tuple(s);
            else if (s->params.algo == SCHED_ALGO_PRIORITY)
                pick = pick_priority(s);
            else if (s->params.algo == SCHED_ALGO_SJF)
                pick = pick_sjf_oracle(s);
            else
                pick = pick_psjf_pred(s);

            if (pick < 0) {
                int na = next_arrival_after(s->ps, s->n, t);
                if (na < 0)
                    break;
                t = na;
                continue;
            }
            mark_running(s, pick);
        }

        int j = s->running;
        int32_t t0 = t;
        int32_t dt = s->ps[j].remaining;
        t += dt;
        gantt_push(s->gantt, s->ps[j].pid, t0, t);
        s->ps[j].remaining = 0;
        finish_segment(s, j, t, use_pred ? s->pred : NULL, s->params.alpha, s->plog);
    }
}

static void sim_run_rr(Sim *s) {
    int32_t t = 0;
    s->running = -1;
    RRQ qq;
    rrq_init(&qq);

    while (!all_done(s->ps, s->n)) {
        for (int i = 0; i < s->n; i++) {
            if (s->ps[i].completion >= 0)
                continue;
            if (i == s->running)
                continue;
            if (s->ps[i].remaining <= 0)
                continue;
            if (s->ps[i].arrival <= t && !rrq_contains(&qq, i))
                rrq_push(&qq, i);
        }

        if (s->running < 0) {
            if (rrq_empty(&qq)) {
                int na = next_arrival_after(s->ps, s->n, t);
                if (na < 0)
                    break;
                t = na;
                continue;
            }
            int j = rrq_pop(&qq);
            mark_running(s, j);
        }

        int j = s->running;
        int32_t qn = s->params.quantum;
        int32_t slice = qn < s->ps[j].remaining ? qn : s->ps[j].remaining;
        int32_t t0 = t;
        t += slice;
        gantt_push(s->gantt, s->ps[j].pid, t0, t);
        s->ps[j].remaining -= slice;

        if (s->ps[j].remaining == 0) {
            finish_segment(s, j, t, NULL, s->params.alpha, s->plog);
            if (s->ps[j].completion < 0 && !rrq_contains(&qq, j))
                rrq_push(&qq, j);
        } else {
            s->running = -1;
            if (!rrq_contains(&qq, j))
                rrq_push(&qq, j);
        }
    }
}

static void sim_run_srtf(Sim *s) {
    int32_t t = 0;
    s->running = -1;
    memset(s->in_ready, 0, sizeof(s->in_ready[0]) * (size_t)s->n);

    while (!all_done(s->ps, s->n)) {
        sim_add_arrivals(s, t);

        int best = pick_srtf(s);
        if (best < 0) {
            int na = next_arrival_after(s->ps, s->n, t);
            if (na < 0)
                break;
            t = na;
            continue;
        }

        if (s->running >= 0 && best != s->running) {
            /* preempt: should not happen at same t if we pick consistently */
            int old = s->running;
            s->in_ready[old] = 1;
            s->ready_stamp[old] = t;
            s->running = -1;
        }

        if (s->running < 0) {
            mark_running(s, best);
        }

        int j = s->running;
        int32_t rem = s->ps[j].remaining;
        if (rem <= 0) {
            finish_segment(s, j, t, NULL, s->params.alpha, s->plog);
            continue;
        }
        int32_t na = next_arrival_after(s->ps, s->n, t);
        int32_t run_until = t + rem;
        if (na >= 0 && na < run_until)
            run_until = na;

        /* Before running [t, run_until), check if another ready job beats us at t (tie already handled) */
        /* Run in one slice to run_until */
        int32_t dt = run_until - t;
        if (dt <= 0)
            continue;

        int32_t t0 = t;
        t += dt;
        gantt_push(s->gantt, s->ps[j].pid, t0, t);
        s->ps[j].remaining -= dt;

        if (s->ps[j].remaining == 0) {
            finish_segment(s, j, t, NULL, s->params.alpha, s->plog);
        } else {
            s->in_ready[j] = 1;
            s->ready_stamp[j] = t;
            s->running = -1;
        }
    }
}

int sched_run(const ProcessSpec *specs, int n, const SchedParams *params, GanttChart *gantt, ScheduleResult *out,
              FILE *predict_log) {
    ProcessState *ps = NULL;
    if (process_states_from_specs(specs, n, &ps) != 0)
        return -1;

    PredictorState *pred = NULL;
    if (params->algo == SCHED_ALGO_PSJF) {
        pred = (PredictorState *)calloc((size_t)n, sizeof(PredictorState));
        if (!pred) {
            free(ps);
            return -1;
        }
        predictor_init(pred, n, params->tau0);
    }

    Sim s;
    memset(&s, 0, sizeof(s));
    s.ps = ps;
    s.n = n;
    s.gantt = gantt;
    s.params = *params;
    s.pred = pred;
    s.plog = predict_log;
    s.running = -1;

    gantt_init(gantt);

    if (params->algo == SCHED_ALGO_RR)
        sim_run_rr(&s);
    else if (params->algo == SCHED_ALGO_SRTF)
        sim_run_srtf(&s);
    else if (params->algo == SCHED_ALGO_FCFS)
        sim_run_fcfs_sjf_prio_psjf(&s, 0);
    else if (params->algo == SCHED_ALGO_PRIORITY)
        sim_run_fcfs_sjf_prio_psjf(&s, 0);
    else if (params->algo == SCHED_ALGO_SJF)
        sim_run_fcfs_sjf_prio_psjf(&s, 0);
    else if (params->algo == SCHED_ALGO_PSJF)
        sim_run_fcfs_sjf_prio_psjf(&s, 1);
    else {
        gantt_free(gantt);
        free(ps);
        free(pred);
        return -1;
    }

    metrics_compute(ps, n, gantt, out);
    free(ps);
    free(pred);
    return 0;
}
