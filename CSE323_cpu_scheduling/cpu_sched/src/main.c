#include "common.h"
#include "metrics.h"
#include "parse.h"
#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_int_prompt(const char *label, int *out) {
    char buf[128];
    long v;
    char *end = NULL;
    for (;;) {
        fprintf(stdout, "%s", label);
        if (!fgets(buf, sizeof(buf), stdin))
            return -1;
        v = strtol(buf, &end, 10);
        if (end != buf) {
            *out = (int)v;
            return 0;
        }
        fprintf(stdout, "Invalid number. Try again.\n");
    }
}

static int read_double_prompt(const char *label, double *out) {
    char buf[128];
    char *end = NULL;
    double v;
    for (;;) {
        fprintf(stdout, "%s", label);
        if (!fgets(buf, sizeof(buf), stdin))
            return -1;
        v = strtod(buf, &end);
        if (end != buf) {
            *out = v;
            return 0;
        }
        fprintf(stdout, "Invalid number. Try again.\n");
    }
}

static int parse_nonneg_int_token(const char *tok, int *out) {
    long v;
    char *end = NULL;
    if (!tok)
        return -1;
    v = strtol(tok, &end, 10);
    if (end == tok || (*end != '\0' && *end != '\n' && *end != '\r') || v < 0 || v > 2147483647L)
        return -1;
    *out = (int)v;
    return 0;
}

static int read_process_line(ProcessSpec *p, int index, SchedAlgo algo, int multi_burst) {
    char line[1024];
    char *tok;
    int arrival, nb = 1, priority = 0, j;

    p->pid = index + 1; // Auto-assign PID (1, 2, 3...)

    for (;;) {
        fprintf(stdout, "\nProcess P%d (PID: %d)\n", (int)p->pid, (int)p->pid);
        fprintf(stdout, "Provide values in this exact order:\n");
        
        /* Dynamic Prompt Construction */
        fprintf(stdout, "  [arrival_time]");
        if (multi_burst) {
            fprintf(stdout, " [burst_count] [burst1 ... burstN]");
        } else {
            fprintf(stdout, " [burst_length]");
        }
        if (algo == SCHED_ALGO_PRIORITY) {
            fprintf(stdout, " [priority]");
        }
        fprintf(stdout, "\n");

        fprintf(stdout, "Input line: ");
        if (!fgets(line, sizeof(line), stdin))
            return -1;

        tok = strtok(line, " \t\r\n");
        if (parse_nonneg_int_token(tok, &arrival) != 0) {
            fprintf(stdout, "Invalid input: [arrival_time] must be a non-negative integer.\n");
            continue;
        }

        p->arrival = arrival;
        p->total_cpu = 0;

        if (multi_burst) {
            tok = strtok(NULL, " \t\r\n");
            if (parse_nonneg_int_token(tok, &nb) != 0 || nb < 1 || nb > SCHED_MAX_BURSTS) {
                fprintf(stdout, "Invalid input: [burst_count] must be 1..%d.\n", SCHED_MAX_BURSTS);
                continue;
            }
        } else {
            nb = 1;
        }
        p->burst_count = nb;

        for (j = 0; j < nb; j++) {
            int b;
            tok = strtok(NULL, " \t\r\n");
            if (parse_nonneg_int_token(tok, &b) != 0 || b <= 0) {
                fprintf(stdout, "Invalid input: missing/invalid burst #%d. Must be > 0.\n", j + 1);
                break;
            }
            p->bursts[j] = b;
            p->total_cpu += b;
        }
        if (j != nb) continue;

        if (algo == SCHED_ALGO_PRIORITY) {
            tok = strtok(NULL, " \t\r\n");
            if (parse_nonneg_int_token(tok, &priority) != 0) {
                fprintf(stdout, "Invalid input: [priority] must be a non-negative integer.\n");
                continue;
            }
        } else {
            priority = 0;
        }
        p->priority = priority;

        tok = strtok(NULL, " \t\r\n");
        if (tok != NULL) {
            fprintf(stdout, "Invalid input: extra values detected at the end.\n");
            continue;
        }
        return 0;
    }
}

static int collect_interactive_input(SchedInput *in) {
    int mode, nproc, i, multi_burst;
    memset(in, 0, sizeof(*in));
    in->params.algo = SCHED_ALGO_FCFS;
    in->params.quantum = 1;
    in->params.alpha = 0.5;
    in->params.tau0 = 5.0;

    fprintf(stdout, "Interactive CPU scheduling input\n");
    fprintf(stdout, "Algorithms: 1=FCFS 2=RR 3=PRIORITY 4=SJF 5=SRTF 6=PSJF\n");
    if (read_int_prompt("Select algorithm number: ", &mode) != 0)
        return -1;

    switch (mode) {
    case 1: in->params.algo = SCHED_ALGO_FCFS; break;
    case 2: in->params.algo = SCHED_ALGO_RR; break;
    case 3: in->params.algo = SCHED_ALGO_PRIORITY; break;
    case 4: in->params.algo = SCHED_ALGO_SJF; break;
    case 5: in->params.algo = SCHED_ALGO_SRTF; break;
    case 6: in->params.algo = SCHED_ALGO_PSJF; break;
    default: fprintf(stderr, "Unknown mode.\n"); return -1;
    }

    if (read_int_prompt("Enable multi-burst mode? (1=Yes, 0=No): ", &multi_burst) != 0)
        return -1;

    if (in->params.algo == SCHED_ALGO_RR) {
        if (read_int_prompt("Quantum: ", &mode) != 0 || mode <= 0)
            return -1;
        in->params.quantum = mode;
    }
    if (in->params.algo == SCHED_ALGO_PSJF) {
        if (read_double_prompt("Alpha [0..1]: ", &in->params.alpha) != 0 || in->params.alpha < 0.0 ||
            in->params.alpha > 1.0)
            return -1;
        if (read_double_prompt("Initial tau (TAU0): ", &in->params.tau0) != 0 || in->params.tau0 < 0.0)
            return -1;
    }

    if (read_int_prompt("Number of processes: ", &nproc) != 0 || nproc <= 0 || nproc > SCHED_MAX_PROCESSES)
        return -1;
    in->nproc = nproc;

    for (i = 0; i < nproc; i++) {
        if (read_process_line(&in->procs[i], i, in->params.algo, multi_burst) != 0)
            return -1;
    }

    return 0;
}


int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    SchedInput in;
    if (collect_interactive_input(&in) != 0) {
        fprintf(stderr, "Interactive input failed.\n");
        return 1;
    }

    GanttChart g;
    ScheduleResult res;
    memset(&g, 0, sizeof(g));
    memset(&res, 0, sizeof(res));

    fprintf(stdout, "Algorithm: ");
    switch (in.params.algo) {
    case SCHED_ALGO_FCFS:
        fprintf(stdout, "FCFS\n");
        break;
    case SCHED_ALGO_RR:
        fprintf(stdout, "RR (quantum=%d)\n", (int)in.params.quantum);
        break;
    case SCHED_ALGO_PRIORITY:
        fprintf(stdout, "PRIORITY (non-preemptive, smaller number = higher priority)\n");
        break;
    case SCHED_ALGO_SJF:
        fprintf(stdout, "SJF (non-preemptive, oracle burst)\n");
        break;
    case SCHED_ALGO_SRTF:
        fprintf(stdout, "SRTF (preemptive, oracle remaining)\n");
        break;
    case SCHED_ALGO_PSJF:
        fprintf(stdout, "PSJF (non-preemptive, predicted next burst via exponential averaging)\n");
        fprintf(stdout, "ALPHA=%.4f TAU0=%.4f\n", in.params.alpha, in.params.tau0);
        break;
    default:
        fprintf(stdout, "(unknown)\n");
        break;
    }

    FILE *plog = NULL;
    if (in.params.algo == SCHED_ALGO_PSJF)
        plog = stdout;

    fprintf(stdout, "\n[DEBUG] Starting simulation engine...\n");
    if (sched_run(in.procs, in.nproc, &in.params, &g, &res, plog) != 0) {
        fprintf(stderr, "[ERROR] Simulation engine failed!\n");
        gantt_free(&g);
        result_free(&res);
        fprintf(stdout, "\nPress any key to exit...");
        getchar();
        return 1;
    }

    fprintf(stdout, "[DEBUG] Simulation finished. Printing results...\n\n");
    gantt_print_ascii(stdout, &g);
    fprintf(stdout, "\n");
    metrics_print(stdout, &res, NULL, 0);

    gantt_free(&g);
    result_free(&res);

    fprintf(stdout, "\n------------------------------------------------\n");
    fprintf(stdout, "DONE. Press ENTER to close this window...");
    fflush(stdout);
    
    /* Robust wait: clear buffer and wait for any character */
    int c;
    while ((c = getchar()) != '\n' && c != EOF); 
    getchar(); 
    
    return 0;
}
