#ifndef __WWCOMMON_H__
#define __WWCOMMON_H__

#include <stdint.h>
#include <time.h>
#include <openssl/sha.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

#define MAX_WSIZES 20
#define MAX_Sources 100

#define N_CLASSES 3

extern char CLASSES[3][50];

extern char WINDOWING_NAMES[3][10];

extern char NN_NAMES[11][10];

typedef int32_t IDX;

typedef enum Class {
    CLASS__NOT_INFECTED,
    CLASS__INFECTED_NOTDGA,
    CLASS__INFECTED_DGA,
} Class;


typedef struct DNSMessage {
    int64_t id;
    int64_t fn_req;
    double value;
    double logit;
    int32_t is_response;
    int32_t top10m;
    int32_t dyndns;
} DNSMessage;

//   P A R A M E T E R S

typedef struct WSize {
    int32_t id;
    int32_t value;
} WSize;

typedef struct WSizes {
    int32_t number;
    WSize* _;
} WSizes;

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

//   W I N D O W,    C A P T U R E    A N D    W I N D O W I N G

typedef struct Window {
    IDX source_index;
    IDX source_classindex;
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

typedef struct Windows {
    int32_t number;
    Window* _;
} Windows;

typedef struct WindowRefs {
    int32_t number;
    Window** _;
} WindowRefs;

//   D A T A S E T

typedef struct CMValue {
    int32_t falses; // wrong
    int32_t trues; // right
} CMValue;

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