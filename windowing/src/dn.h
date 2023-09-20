#ifndef __DN_H__
#define __DN_H__

#include <stdint.h>

#define MAX_WSIZES 20
#define MAX_CAPTURES 100

#define N_CLASSES 3

extern char CLASSES[3][50];

extern char WINDOWING_NAMES[3][10];

extern char NN_NAMES[11][10];

typedef struct Message {
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
    int32_t wsize;
} PSet;

typedef PSet* PSetPtr;
typedef PSet* PSets;



//   W I N D O W,    C A P T U R E    A N D    W I N D O W I N G

typedef struct WindowMetrics {
    PSet* pi;
    int pi_id;
    
    int32_t wcount;

    double logit;
    int32_t whitelistened;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} WindowMetrics;

typedef struct Window {
    int32_t parent_id;

    int32_t wsize;
    int32_t wnum;
    Class class;

    int32_t nmetrics;
    WindowMetrics* metrics;
} Window;

typedef struct Capture {
    int32_t id;

    CaptureType capture_type;

    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t nmessages;
    int64_t fnreq_max;

    char name[50];
    char source[50];

    FetchPtr fetch;

    Class class;
} Capture;

typedef Capture* CapturePtr;
typedef Capture* Captures;


typedef struct WindowingSet {
    int32_t n_windows;
    Window* windows;
} WindowingSet;

typedef WindowingSet* WindowingSetPtr;
typedef WindowingSet* WindowingSets;

typedef struct Windowing {

    int32_t n_psets;
    PSet* psets;

    int32_t n_wsizes;
    int32_t wsizes[MAX_WSIZES];

    int32_t n_captures;
    Capture* captures[MAX_CAPTURES];
    WindowingSet captures_windowings[MAX_CAPTURES][MAX_WSIZES];

} Windowing;

typedef struct Windowing* WindowingPtr;

typedef void (*FetchPtr)(char*, WindowingPtr, int32_t);

typedef struct WindowingTotals {
    int32_t total;
    int32_t positives;
    int32_t negatives;
} WindowingTotals;






//   D A T A S E T

typedef struct WindowingRefSet {
    int32_t n_windows;
    Window** windows;
} WindowingRefSet;



typedef struct DatasetClass {
    char name[50];
    WindowingRefSet windowingref_set;
} DatasetClass;



typedef struct DatasetTrainTest {

    int32_t wsize;
    double percentage_split; // train over test
    WindowingRefSet train[N_CLASSES];
    WindowingRefSet test[N_CLASSES];

} DatasetTrainTest;

typedef DatasetTrainTest* DatasetTrainTestPtr;



typedef struct Dataset {

    int32_t wsize;
    WindowingRefSet total;
    WindowingRefSet* classes;

} Dataset;

typedef Dataset* DatasetPtr;



#endif