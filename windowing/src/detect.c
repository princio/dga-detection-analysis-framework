#include "detect.h"

#include <stdlib.h>
#include <string.h>


#define UNUSED(x) (void)(x)

// TODO: check if is passed by refence with %p printf
const char evaluation_functions[3][50] = {
    "f1score",
    "fpr",
    "tpr"
};

const char dgadetection_names[N_DGADETECTIONs][50] = {
    "ignore",
    "merge"
};

double detect_f1score_beta(DGADetection dgadetection, Detections dtfs, double beta);
double detect_fpr(DGADetection dgadetection, Detections dtfs, double __ignore);
double detect_tpr(DGADetection dgadetection, Detections dtfs, double __ignore);

const EvaluationMethod evaluation_methods[N_EVALUATEMETHODs] = {
    { .id=DETECT_SF_F1SCORE, .beta = 1.0, .name = "F1SCORE_1.0", .func=&detect_f1score_beta, .greater=1 },
    { .id=DETECT_SF_F1SCORE, .beta = 0.5, .name = "F1SCORE_0.5", .func=&detect_f1score_beta, .greater=1 },
    { .id=DETECT_SF_F1SCORE, .beta = 0.1, .name = "F1SCORE_0.1", .func=&detect_f1score_beta, .greater=1 },
    { .id=DETECT_SF_FPR, .beta = 0., .name = "FPR", .func=&detect_fpr, .greater=0 },
    { .id=DETECT_SF_TPR, .beta = 0., .name = "TPR", .func=&detect_tpr, .greater=1 },
};

double detect_f1score_beta(DGADetection dgadetection, Detections dtfs, double beta) {
    double f1;
    switch (dgadetection)
    {
        case DGADETECTION_MERGE_1: {
            double fp = dtfs[0].windows.falses;
            double fn = dtfs[2].windows.falses + dtfs[1].windows.falses;
            double tp = dtfs[2].windows.trues + dtfs[1].windows.trues;

            double beta_2 = beta * beta;

            f1 = ((double) (1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
            break;
        }

        case DGADETECTION_IGNORE_1: {
            double fp = dtfs[0].windows.falses;
            double fn = dtfs[2].windows.falses;
            double tp = dtfs[2].windows.trues;

            double beta_2 = beta * beta;

            f1 = ((double) (1 + beta_2) * tp) / (((1 + beta_2) * tp) + beta_2 * fp + fn);
            break;
        }
    }

    return f1;
}

double detect_fpr(DGADetection dgadetection, Detections dtfs, double __ignore) {
    UNUSED(dgadetection);
    UNUSED(__ignore);

    double fp = dtfs[0].windows.falses;
    double tn = dtfs[0].windows.trues;

    return ((double) fp) / (fp + tn);
}

double detect_tpr(DGADetection dgadetection, Detections dtfs, double __ignore) {
    UNUSED(dgadetection);
    UNUSED(__ignore);

    double fn, tp;

    switch (dgadetection)
    {
        case DGADETECTION_MERGE_1: {
            fn = dtfs[2].windows.falses + dtfs[1].windows.falses;
            tp = dtfs[2].windows.trues + dtfs[1].windows.trues;
            break;
        }
        case DGADETECTION_IGNORE_1: {
            fn = dtfs[2].windows.falses;
            tp = dtfs[2].windows.trues;
            break;
        }
    }

    return ((double) fn) / (fn + tp);
}

double detect_score_func(DGADetection dgadetection, EvaluationFunctionID id, Detections detections) {
    return (*evaluation_methods[id].func)(dgadetection, detections, evaluation_methods[id].beta);
}

void detect_reset_detections(Detections detections) {
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        memset(detections[cl].sources._, 0, detections[cl].sources.number * sizeof(TF));
        memset(&detections[cl].windows, 0, sizeof(TF));
    }
}

void detect_evaluations_init(Evaluations evaluations, int32_t n_sources[N_DGACLASSES]) {
    for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
        for (int ev = 0; ev <= N_EVALUATEMETHODs; ev++) {
            for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
                evaluations[dd][ev].detections[cl].sources.number = n_sources[cl];
                evaluations[dd][ev].detections[cl].sources._ = calloc(n_sources[cl], sizeof(TF));
            }
            detect_reset_detections(evaluations[dd][ev].detections);
        }
    }
}

void detect_evaluations_free(Evaluations evaluations) {
    for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
        for (int ev = 0; ev <= N_EVALUATEMETHODs; ev++) {
            for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
                free(evaluations[dd][ev].detections[cl].sources._);
            }
        }
    }
}

void detect_detect(DatasetRWindows drw, double th, Detections detections) {
    detect_reset_detections(detections);

    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        Detection* detection = &detections[cl];

        for (int32_t i = 0; i < drw[cl].number; i++) {
            Window* window = drw[cl]._[i];
            int prediction = window->logit >= th;
            int infected = cl > 0;

            if (prediction == infected) {
                detection->windows.trues++;
                detection->sources._[window->source_classindex].trues++;
            } else {
                detection->windows.falses++;
                detection->sources._[window->source_classindex].falses++;
            }
        }
    }
}

void detect_evaluate(DGADetection dd, int ev_id, DatasetRWindows drw, double th, Evaluation* evaluation) {
    detect_reset_detections(evaluation->detections);

    evaluation->th = th;

    detect_detect(drw, evaluation->th, evaluation->detections);

    evaluation->score = detect_score_func(dd, ev_id, evaluation->detections);
}

void detect_evaluate_many(DatasetRWindows drw, double th, Evaluations evaluations) {
    for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
        for (int ev = 0; ev < N_EVALUATEMETHODs; ev++) {
            detect_evaluate(dd, ev, drw, th, &evaluations[dd][ev]);
        }
    }
}