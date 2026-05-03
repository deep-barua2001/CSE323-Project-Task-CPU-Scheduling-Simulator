#include "parse.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void set_error(SchedInput *in, const char *msg) {
    strncpy(in->error, msg, sizeof(in->error) - 1);
    in->error[sizeof(in->error) - 1] = '\0';
}

static int is_comment_or_empty(const char *s) {
    while (*s && isspace((unsigned char)*s))
        s++;
    return *s == '\0' || *s == '#';
}

static int parse_algo(const char *val, SchedAlgo *out) {
    if (strcmp(val, "FCFS") == 0) {
        *out = SCHED_ALGO_FCFS;
        return 0;
    }
    if (strcmp(val, "RR") == 0) {
        *out = SCHED_ALGO_RR;
        return 0;
    }
    if (strcmp(val, "PRIORITY") == 0) {
        *out = SCHED_ALGO_PRIORITY;
        return 0;
    }
    if (strcmp(val, "SJF") == 0) {
        *out = SCHED_ALGO_SJF;
        return 0;
    }
    if (strcmp(val, "SRTF") == 0) {
        *out = SCHED_ALGO_SRTF;
        return 0;
    }
    if (strcmp(val, "PSJF") == 0) {
        *out = SCHED_ALGO_PSJF;
        return 0;
    }
    return -1;
}

static int parse_int(const char *tok, int32_t *out) {
    char *end = NULL;
    long v = strtol(tok, &end, 10);
    if (end == tok || *end != '\0' || v < 0 || v > 0x7fffffffL)
        return -1;
    *out = (int32_t)v;
    return 0;
}

static int parse_double_tok(const char *tok, double *out) {
    char *end = NULL;
    double v = strtod(tok, &end);
    if (end == tok || *end != '\0' || v < 0.0 || v > 1.0)
        return -1;
    *out = v;
    return 0;
}

static int parse_double_pos(const char *tok, double *out) {
    char *end = NULL;
    double v = strtod(tok, &end);
    if (end == tok || *end != '\0' || v < 0.0)
        return -1;
    *out = v;
    return 0;
}

/* Strip inline # comments */
static void strip_inline_comment(char *line) {
    char *p = line;
    while (*p) {
        if (*p == '#') {
            *p = '\0';
            return;
        }
        p++;
    }
}

int sched_parse_file(FILE *f, SchedInput *in) {
    char line[SCHED_MAX_LINE];
    memset(in, 0, sizeof(*in));
    in->params.quantum = 1;
    in->params.alpha = 0.5;
    in->params.tau0 = 5.0;
    in->params.algo = SCHED_ALGO_FCFS;

    while (fgets(line, sizeof(line), f)) {
        strip_inline_comment(line);
        if (is_comment_or_empty(line))
            continue;

        char cmd[64];
        char rest[SCHED_MAX_LINE];
        rest[0] = '\0';
        if (sscanf(line, "%63s %[^\n]", cmd, rest) < 1)
            continue;

        if (strcmp(cmd, "ALGO") == 0) {
            char val[64];
            if (sscanf(rest, "%63s", val) != 1) {
                set_error(in, "ALGO requires a value");
                return -1;
            }
            SchedAlgo a;
            if (parse_algo(val, &a) != 0) {
                set_error(in, "Unknown ALGO");
                return -1;
            }
            in->params.algo = a;
        } else if (strcmp(cmd, "QUANTUM") == 0) {
            int32_t q;
            if (sscanf(rest, "%d", &q) != 1 || q <= 0) {
                set_error(in, "QUANTUM must be a positive integer");
                return -1;
            }
            in->params.quantum = q;
        } else if (strcmp(cmd, "ALPHA") == 0) {
            char tok[64];
            if (sscanf(rest, "%63s", tok) != 1) {
                set_error(in, "ALPHA requires a value in [0,1]");
                return -1;
            }
            double a;
            if (parse_double_tok(tok, &a) != 0) {
                set_error(in, "ALPHA must be in [0,1]");
                return -1;
            }
            in->params.alpha = a;
        } else if (strcmp(cmd, "TAU0") == 0) {
            char tok[64];
            if (sscanf(rest, "%63s", tok) != 1) {
                set_error(in, "TAU0 requires a non-negative number");
                return -1;
            }
            double t;
            if (parse_double_pos(tok, &t) != 0) {
                set_error(in, "TAU0 invalid");
                return -1;
            }
            in->params.tau0 = t;
        } else if (strcmp(cmd, "P") == 0) {
            if (in->nproc >= SCHED_MAX_PROCESSES) {
                set_error(in, "Too many processes");
                return -1;
            }
            /* Tokenize rest: id arrival ... bursts ... priority (last) */
            char buf[SCHED_MAX_LINE];
            strncpy(buf, rest, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            char *tok = strtok(buf, " \t\r\n");
            int32_t fields[2 + SCHED_MAX_BURSTS + 1];
            int nf = 0;
            while (tok && nf < (int)(sizeof(fields) / sizeof(fields[0]))) {
                int32_t v;
                if (parse_int(tok, &v) != 0) {
                    set_error(in, "P line: invalid integer");
                    return -1;
                }
                fields[nf++] = v;
                tok = strtok(NULL, " \t\r\n");
            }
            if (nf < 4) {
                set_error(in, "P requires: id arrival burst(s)... priority");
                return -1;
            }
            int32_t pid = fields[0];
            int32_t arrival = fields[1];
            int32_t priority = fields[nf - 1];
            int nb = nf - 3; /* bursts count */
            if (nb < 1 || nb > SCHED_MAX_BURSTS) {
                set_error(in, "P: invalid burst list");
                return -1;
            }
            ProcessSpec *p = &in->procs[in->nproc];
            p->pid = pid;
            p->arrival = arrival;
            p->priority = priority;
            p->burst_count = nb;
            p->total_cpu = 0;
            for (int i = 0; i < nb; i++) {
                int32_t b = fields[2 + i];
                if (b <= 0) {
                    set_error(in, "P: burst lengths must be positive");
                    return -1;
                }
                p->bursts[i] = b;
                p->total_cpu += b;
            }
            in->nproc++;
        } else {
            set_error(in, "Unknown directive");
            return -1;
        }
    }

    if (in->nproc == 0) {
        set_error(in, "No processes defined");
        return -1;
    }
    if (in->params.algo == SCHED_ALGO_RR && in->params.quantum <= 0) {
        set_error(in, "RR requires positive QUANTUM");
        return -1;
    }
    return 0;
}
