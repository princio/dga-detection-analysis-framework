#ifndef __DN_H__
#define __DN_H__

#include <stdint.h>
#include <time.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0


#define MAX_WSIZES 20
#define MAX_Sources 100

#define N_CLASSES 3

extern char CLASSES[3][50];

extern char WINDOWING_NAMES[3][10];

extern char NN_NAMES[11][10];

extern int trys;
extern char try_name[5];


typedef int32_t IDX;


typedef struct Message {
    int64_t id;
    int64_t fn_req;
    double value;
    double logit;
    int32_t is_response;
    int32_t top10m;
    int32_t dyndns;
} Message;

typedef enum Class {
    CLASS__NOT_INFECTED,
    CLASS__INFECTED_NOTDGA,
    CLASS__INFECTED_DGA,
} Class;


//   P A R A M E T E R S

typedef struct Whitelisting {
    int32_t rank;
    double value;
} Whitelisting;

typedef enum WindowingType {
    WINDOWING_Q,
    WINDOWING_R,
    WINDOWING_QR
} WindowingType;

typedef enum NN {
    NN_ICANN = 7,
    NN_NONE,
    NN_PRIVATE,
    NN_TLD
} NN;

typedef enum CaptureType {
    CAPTURETYPE_PCAP,
    CAPTURETYPE_LIST
} CaptureType;

typedef struct InfiniteValues {
    double ninf;
    double pinf;
} InfiniteValues;

typedef struct LogitRange {
    double min;
    double max;
} LogitRange;

typedef struct PSet {
    int32_t id;
    LogitRange logit_range;
    Whitelisting whitelisting;
    WindowingType windowing;
    InfiniteValues infinite_values;
    NN nn;
} PSet;

typedef struct WSize {
    int32_t id;
    int32_t value;
} WSize;


typedef struct PSetGenerator {
    int32_t n_whitelisting;
    Whitelisting* whitelisting;

    int32_t n_windowing;
    WindowingType* windowing;

    int32_t n_infinitevalues;
    InfiniteValues* infinitevalues;

    int32_t n_nn;
    NN* nn;
} PSetGenerator;

typedef PSet* PSetPtr;


typedef struct PSets {
    int32_t number;
    PSet* _;
} PSets;


typedef struct WSizes {
    int32_t number;
    WSize* _;
} WSizes;


typedef struct Experiment {
    char name[100];
    char rootpath[500];
    
    WSizes wsizes;
    PSets psets;

    int N_SPLITRATIOs;
    double split_percentages[10];

    int KFOLDs;
    
    // EvaluationMetricFunctions evmfs;

} Experiment;


//   W I N D O W,    C A P T U R E    A N D    W I N D O W I N G


typedef struct WindowMetricSet {
    IDX wsize_idx;
    IDX source_idx;
    IDX window_idx;
    IDX     pi_id;

    int32_t wcount;
    double  logit;
    int32_t whitelistened;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} WindowMetricSet;

typedef struct WindowMetricSets {
    int32_t number;
    WindowMetricSet* _;
} WindowMetricSets;

typedef struct Window {
    IDX source_id;
    IDX dataset_id;
    IDX window_id;

    Class class;

    int32_t wcount;
    double  logit;
    int32_t whitelistened;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} Window;

typedef struct Source {
    int32_t id;
    
    int32_t galaxy_id;

    CaptureType capture_type;

    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t nmessages;
    int64_t fnreq_max;

    char name[50];
    char source[50];

    Class class;
} Source;

typedef Source* SourcePtr;


typedef struct Sources {
    int32_t number;
    Source* _;
} Sources;


typedef struct WSet {
    int32_t number;
    Window* _;
} WSet;

typedef WSet* WSets;
typedef WSet* WSetPtr;

typedef struct WindowingConfig {
    char name[100];
    char rootpath[500];

    time_t time;

    Sources Sources;

    PSets psets;

    int32_t wsize;

} WindowingConfig;


typedef struct Windows {
    int32_t number;
    Window* _;
} Windows;

typedef struct WindowRefs {
    int32_t number;
    Window** _;
} WindowRefs;

typedef struct Windowing {

    int32_t id;

    WSize wsize;

    PSet* pset;

} Windowing;


typedef struct Windowed {

    int32_t id;

    Windowing windowing;

    Source* source;

    Windows windows; // By CAPTURE,WINDOW

} Windowed;

typedef struct Windoweds {
    int32_t number;
    Windowed* _;
} Windoweds;


typedef struct WindowingID {

    int32_t pset_id;
    int32_t wsize_id;
    int32_t source_id;

} WindowingID;

typedef struct Windowings {
    int32_t number;
    Windowing* _;
} Windowings;

typedef void (*FetchPtr)(Source*, Windowings);

typedef struct WindowingConfig* WindowingConfigPtr;
typedef struct Windowing* WindowingPtr;

typedef struct WindowingTotals {
    int32_t total;
    int32_t positives;
    int32_t negatives;
} WindowingTotals;


typedef struct WSetRef {
    int32_t number;
    Window** _;
} WSetRef;

typedef struct Dataset0 {
    Windowing windowing;

    Windows windows[N_CLASSES];
} Dataset0;


typedef struct Dataset0s {
    int32_t number;
    Dataset0* _;
} Dataset0s;

// CONFUSION MATRIX

typedef struct CMValue {
    int32_t falses; // wrong
    int32_t trues; // right
} CMValue;

typedef struct Prediction {

    int32_t wsize;

    PSet* pset;

    CMValue single[2];

    CMValue multi[N_CLASSES];

} Prediction;

typedef struct ClassificationMetricsAverages {

    int32_t wsize;

    PSet* pset;

    int32_t classes[N_CLASSES][2];

} ClassificationMetricsAverages;

typedef double (*EvaluationMetricFunctionPtr)(Prediction);

typedef struct EvaluationMetricFunction {
    char name[50];
    EvaluationMetricFunctionPtr fnptr;
} EvaluationMetricFunction;

typedef struct EvaluationMetricFunctions {
    int32_t number;
    EvaluationMetricFunctionPtr* _;
} EvaluationMetricFunctions;

typedef struct EvaluationMetric {
    int32_t emf_idx;
    double value;
} EvaluationMetric;

typedef struct EvaluationMetrics {
    int32_t number;
    EvaluationMetric* _;
} EvaluationMetrics;

typedef struct Predictions {
    int32_t number;
    Prediction* _;
    EvaluationMetrics ems;
    double th;
} Predictions;




//   D A T A S E T

typedef struct DatasetClass {
    char name[50];
    WSetRef windowingref_set;
} DatasetClass;

typedef struct DatasetTrainTest {

    int32_t wsize;

    double percentage_split; // train over test

    WSetRef train[N_CLASSES];
    WSetRef test[N_CLASSES];

    Predictions eval;

    Predictions ptest;

} DatasetTrainTest;

typedef DatasetTrainTest* DatasetTrainTestPtr;

typedef struct Dataset {

    int32_t wsize;
    WSetRef windows_all;
    WSetRef windows[N_CLASSES]; // One for each class
} Dataset;



//   M E T R I C S


typedef struct CM {
    CMValue single[2];
    CMValue multi[N_CLASSES];
} CM;

typedef struct RatioValue {
    int32_t nwindows;
    int32_t absolute[2];
    double relative[2];
} RatioValue;

typedef struct Ratio {
    int32_t nwindows;

    RatioValue single_ratio;

    RatioValue multi_ratio[N_CLASSES];
} Ratio;

typedef struct Ratios {
    int32_t number;
    Ratio* _;
} Ratios;

typedef struct AVG {
    int32_t mask;
    Ratios ratios;
} AVG;

#endif