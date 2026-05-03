#include "predictor.h"

#include <string.h>

void predictor_init(PredictorState *p, int nproc, double tau0) {
    for (int i = 0; i < nproc; i++) {
        p[i].tau = tau0;
        p[i].has_observed = 0;
    }
}

void predictor_on_burst_done(PredictorState *p, int proc_index, int32_t actual_burst_len, double alpha) {
    PredictorState *s = &p[proc_index];
    if (!s->has_observed) {
        /* First sample blends configured TAU0 with the observed burst. */
        s->tau = alpha * (double)actual_burst_len + (1.0 - alpha) * s->tau;
        s->has_observed = 1;
    } else {
        s->tau = alpha * (double)actual_burst_len + (1.0 - alpha) * s->tau;
    }
}

double predictor_key_tau(const PredictorState *p, int proc_index) {
    return p[proc_index].tau;
}
