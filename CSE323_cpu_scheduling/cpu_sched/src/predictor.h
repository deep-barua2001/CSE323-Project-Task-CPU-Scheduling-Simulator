#ifndef SCHED_PREDICTOR_H
#define SCHED_PREDICTOR_H

#include "common.h"

typedef struct {
    double tau;
    int has_observed; /* after first completed CPU burst for this process */
} PredictorState;

void predictor_init(PredictorState *p, int nproc, double tau0);
void predictor_on_burst_done(PredictorState *p, int proc_index, int32_t actual_burst_len, double alpha);

/* Key for PSJF: lower tau wins; tie-break by pid handled by caller. */
double predictor_key_tau(const PredictorState *p, int proc_index);

#endif
