
#ifndef __FOLDING_EVAL_H__
#define __FOLDING_EVAL_H__

#include "folding.h"

#include <stdint.h>

#define N_FOLDINGEVAL_METHODS_TYPES 2
#define N_FOLDINGEVAL_MACROMETHODS 1
#define N_FOLDINGEVAL_MICROMETHODS 5

typedef struct FoldingEvalMetrics {
    double min;
    double max;
    double avg;
} FoldingEvalMetrics;

// same Evaluation methods accross all different foldings


typedef enum FoldingEvalRange {
    FOLDINGEVAL_RANGE_01,
    FOLDINGEVAL_RANGE_INF,
    FOLDINGEVAL_RANGE_INT_POS,
} FoldingEvalRange;

typedef enum FoldingEvalFunctionType {
    FOLDINGEVAL_FUNC_MACRO,
    FOLDINGEVAL_FUNC_MICRO
} FoldingEvalFunctionType;

typedef enum FoldingEvalMacroFunctionID {
    FOLDINGEVAL_MACRO_TH
} FoldingEvalMacroFunctionID;

typedef enum FoldingEvalMicroFunctionID {
    FOLDINGEVAL_MICRO_FALSES,
    FOLDINGEVAL_MICRO_TRUES,
    FOLDINGEVAL_MICRO_FALSERATIO,
    FOLDINGEVAL_MICRO_TRUERATIO,
    FOLDINGEVAL_MICRO_SOURCES_DETECTED,
} FoldingEvalMicroFunctionID;

typedef double (*FoldingEvalMacroFunctionPtr)(Evaluation*);
typedef double (*FoldingEvalMicroFunctionPtr)(Detection*);

typedef struct FoldingEvalMethod {
    FoldingEvalFunctionType type;
    int id;
    char name[20];
    void* func;
    FoldingEvalRange range;
    int increasing;
} FoldingEvalMethod;



// typedef struct FoldingEvalMacroStat {
//     FoldingEvalMetrics methods[N_FOLDINGEVAL_MACROMETHODS];
// } FoldingEvalMacroStat;

// typedef struct FoldingEvalMacroStat {
//     FoldingEvalMetrics methods[N_FOLDINGEVAL_MICROMETHODS];
// } FoldingEvalMacroStat;

typedef FoldingEvalMetrics FoldingEvalMacro[N_FOLDINGEVAL_MACROMETHODS];
typedef FoldingEvalMetrics (*FoldingEvalMacroPtr);

typedef FoldingEvalMetrics FoldingEvalMicro[N_FOLDINGEVAL_MICROMETHODS][N_DGACLASSES];
typedef FoldingEvalMetrics (*FoldingEvalMicroPtr)[N_DGACLASSES];


typedef struct FoldingEvalDescribe {
    FoldingEvalMetrics macro[N_FOLDINGEVAL_MACROMETHODS];
    FoldingEvalMetrics micro[N_FOLDINGEVAL_MICROMETHODS][N_DGACLASSES];
} FoldingEvalDescribe;

const FoldingEvalMethod folding_eval_methods[N_FOLDINGEVAL_METHODS_TYPES][N_FOLDINGEVAL_MACROMETHODS];

typedef FoldingEvalDescribe FoldingEval[N_DGADETECTIONs][N_EVALUATEMETHODs];

void foldingeval(Folding*, FoldingEval);

void foldingeval_print(FoldingEval fe);

#endif