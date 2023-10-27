
#ifndef __DETECT_H__
#define __DETECT_H__

#include "dataset.h"
#include "sources.h"
#include "windows.h"

#define N_EVALUATEMETHODs 5
#define N_DGADETECTIONs 2

typedef enum EvaluationFunctionID {
    DETECT_SF_F1SCORE,
    DETECT_SF_FPR, // il train non ha senso sugli infetti
    DETECT_SF_TPR  // il train non ha senso sui non infetti
} EvaluationFunctionID;

const char evaluation_functions[3][50];

typedef enum DGADetection {
    DGADETECTION_IGNORE_1,
    DGADETECTION_MERGE_1
} DGADetection;

const char dgadetection_names[N_DGADETECTIONs][50];

typedef struct TF {
    int32_t falses;
    int32_t trues;
} TF;

typedef struct TFs {
    int32_t number; // number of trues/falses windows for each source
    TF* _; // number of trues/falses windows for each source
} TFs;

typedef struct Detection {
    TF windows;
    TFs sources;
} Detection;

typedef Detection Detections[N_DGACLASSES];

typedef double (*EvaluationFunctionPtr)(DGADetection, Detections, double);

typedef struct EvaluationMethod {
    EvaluationFunctionID id;
    char name[20];
    double beta;
    EvaluationFunctionPtr func;
    int greater;
} EvaluationMethod;

const EvaluationMethod evaluation_methods[N_EVALUATEMETHODs];

typedef struct Evaluation {
    double score;
    double th;
    Detections detections;
} Evaluation;

typedef Evaluation Evaluations[N_DGADETECTIONs][N_EVALUATEMETHODs];

void detect_evaluations_init(Evaluations, SourcesArrays);
void detect_evaluations_free(Evaluations);

double detect_f1score_beta(DGADetection, Detections, double);
double detect_fpr(DGADetection, Detections, double);
double detect_tpr(DGADetection, Detections, double);

void detect_reset_tfs(Detections);

void detect_detect(DatasetRWindows, double, Detections);

void detect_evaluate(DGADetection, int, DatasetRWindows, double, Evaluation*);
void detect_evaluate_many(DatasetRWindows, double, Evaluations);

#endif