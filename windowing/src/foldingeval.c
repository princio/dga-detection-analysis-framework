#include "foldingeval.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const int n_methods[N_FOLDINGEVAL_METHODS_TYPES] = {
    N_FOLDINGEVAL_MACROMETHODS,
    N_FOLDINGEVAL_MICROMETHODS,
};

const FoldingEvalMethod foldingeval_macro_methods[N_FOLDINGEVAL_MACROMETHODS] = {
    {
        .type = FOLDINGEVAL_FUNC_MACRO,
        .id = FOLDINGEVAL_MACRO_TH,
        .range = FOLDINGEVAL_RANGE_INF,
        .name = "",
        .func = NULL //&foldingeval_macro_th 
    }
};

const FoldingEvalMethod foldingeval_micro_methods[N_FOLDINGEVAL_MICROMETHODS] = {
    {
        .type = FOLDINGEVAL_FUNC_MICRO,
        .id = FOLDINGEVAL_MICRO_FALSES,
        .range = FOLDINGEVAL_RANGE_INT_POS,
        .name = "",
    },
    {
        .type = FOLDINGEVAL_FUNC_MICRO,
        .id = FOLDINGEVAL_MICRO_TRUES,
        .range = FOLDINGEVAL_RANGE_INT_POS,
        .name = "",
    },
    {
        .type = FOLDINGEVAL_FUNC_MICRO,
        .id = FOLDINGEVAL_MICRO_FALSERATIO,
        .range = FOLDINGEVAL_RANGE_01,
        .name = "",
    },
    {
        .type = FOLDINGEVAL_FUNC_MICRO,
        .id = FOLDINGEVAL_MICRO_TRUERATIO,
        .range = FOLDINGEVAL_RANGE_01,
        .name = "",
    },
    {
        .type = FOLDINGEVAL_FUNC_MICRO,
        .id = FOLDINGEVAL_MICRO_SOURCES_DETECTED,
        .range = FOLDINGEVAL_RANGE_INT_POS,
        .name = "",
    }
};

void _metrics_reset_range(FoldingEvalRange range, FoldingEvalMetrics* femetrics) {
    switch (range) {
        case FOLDINGEVAL_RANGE_01:
            femetrics->avg = 0;
            femetrics->min = 1;
            femetrics->max = 0;
        break;
        case FOLDINGEVAL_RANGE_INF:
            femetrics->avg = 0;
            femetrics->min = DBL_MAX;
            femetrics->max = -1 * DBL_MAX;
        break;
        case FOLDINGEVAL_RANGE_INT_POS:
            femetrics->avg = 0;
            femetrics->min = INT32_MAX;
            femetrics->max = 0;
        break;
    }
}

void _metrics_reset(FoldingEvalFunctionType type, int id, FoldingEvalDescribe* femetrics) {
    if (type == FOLDINGEVAL_FUNC_MACRO) {
        _metrics_reset_range(foldingeval_macro_methods[id].range, &femetrics->macro[id]);
    } else {
        for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
            _metrics_reset_range(foldingeval_micro_methods[id].range, &femetrics->micro[id][cl]);
        }
    }
}

void _set_fem(FoldingEvalMetrics* fem, double value) {
        fem->avg += value;
        if (fem->min > value) fem->min = value;
        if (fem->max < value) fem->max = value;
}

double foldingeval_macro(FoldingEvalMacroFunctionID id, Evaluation* ev) {
    switch (id)
    {
    case FOLDINGEVAL_MACRO_TH:
        return ev->th;
    default:
        break;
    }
    fprintf(stderr, "Non reachable code reached\n");
    return 0.0;
}

double foldingeval_micro(FoldingEvalMicroFunctionID id, Detection* det) {
    switch (id)
    {
    case FOLDINGEVAL_MICRO_FALSERATIO:
        return ((double) det->windows.falses) / (det->windows.falses + det->windows.trues);

    case FOLDINGEVAL_MICRO_TRUERATIO:
        return ((double) det->windows.trues) / (det->windows.falses + det->windows.trues);

    case FOLDINGEVAL_MICRO_FALSES:
        return ((double) det->windows.falses);

    case FOLDINGEVAL_MICRO_TRUES:
        return ((double) det->windows.trues);

    case FOLDINGEVAL_MICRO_SOURCES_DETECTED: {
            int32_t sources_trues = 0;
            int32_t sources_n = 0;
            for (int32_t i = 0; i < det->sources.number; i++) {
                TF* tf = &det->sources._[i];
                if (tf->trues + tf->falses) {
                    sources_n++;
                }
                sources_trues += tf->trues > 0;
            }
            return ((double) sources_trues) / sources_n;
        }
    }
    fprintf(stderr, "Non reachable code reached\n");
    return 0.0;
}

void foldingeval_reset(FoldingEval fe) {
    memset(fe, 0, sizeof(FoldingEval));
    for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
        for (int ev = 0; ev < N_EVALUATEMETHODs; ev++) {
            for (FoldingEvalFunctionType type = 0; type < 2; type++) {
                for (int fev = 0; fev < n_methods[(int) type]; fev++) {
                    _metrics_reset(type, fev, &fe[dd][ev]);
                }
            }
        }
    }
}

void foldingeval_metrics_print(char* specifier, FoldingEvalMetrics* metrics) {
    char format[200];
    sprintf(format, "%s%s%s", specifier, specifier, specifier);
    printf(format, metrics->avg, metrics->min, metrics->max);
}

void foldingeval_divide(int32_t kfolds, FoldingEvalDescribe* describe) {
    for (int type = 0; type < N_FOLDINGEVAL_METHODS_TYPES; type++) {
        for (int id = 0; id < n_methods[type]; id++) {
            if (type == FOLDINGEVAL_FUNC_MACRO) {
                describe->macro[id].avg /= kfolds;
            } else {
                for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
                    describe->micro[id][cl].avg /= kfolds;
                }
            }
        }
    }
}

void foldingeval_set(Evaluation* evaluation, FoldingEvalFunctionType type, int id, FoldingEvalDescribe* describe) {
    if (type == FOLDINGEVAL_FUNC_MACRO) {
        FoldingEvalMetrics* fem = &describe->macro[id];
        double value = foldingeval_macro(id, evaluation);
        _set_fem(fem, value);
    } else {
        for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
            FoldingEvalMetrics* fem = &describe->micro[id][cl];

            double value = foldingeval_micro(id, &evaluation->detections[cl]);
            _set_fem(fem, value);
        }
    }
}

void foldingeval(Folding* folding, FoldingEval fe) {

    foldingeval_reset(fe);


    for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
        for (int ev = 0; ev < N_EVALUATEMETHODs; ev++) {
            for (int k = 0; k < folding->config.kfolds; k++) {
                Evaluation* evaluation = &folding->ks[k].test.evaluations[dd][ev];
                for (int type = 0; type < N_FOLDINGEVAL_METHODS_TYPES; type++) {
                    for (int id = 0; id < n_methods[type]; id++) {
                        foldingeval_set(evaluation, type, id, &fe[dd][ev]);
                    }
                }
            }
            foldingeval_divide(folding->config.kfolds, &fe[dd][ev]);
        }
    }
}

void foldingeval_print(FoldingEval fe) {
    for (int type = 0; type < N_FOLDINGEVAL_METHODS_TYPES; type++) {
        for (int id = 0; id < n_methods[type]; id++) {
            for (int dd = 0; dd < N_DGADETECTIONs; dd++) {
                printf("dga detection = %s\n", dgadetection_names[dd]);
                for (int ev = 0; ev < N_EVALUATEMETHODs; ev++) {
                    printf("\t%s\n", evaluation_methods[ev].name);
                    if (type == FOLDINGEVAL_FUNC_MACRO) {
                        printf("\t%20s\t", foldingeval_macro_methods[id].name);
                        foldingeval_metrics_print("%12.4f", &fe[dd][ev].macro[id]);
                    } else {
                        for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
                            printf("%20s\t", foldingeval_micro_methods[id].name);
                            foldingeval_metrics_print("%12.4f", &fe[dd][ev].micro[id][cl]);
                        }
                    }
                }
            }
        }
    }
}