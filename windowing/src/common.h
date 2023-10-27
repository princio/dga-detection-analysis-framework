#ifndef __WWCOMMON_H__
#define __WWCOMMON_H__

#include <stdint.h>
#include <time.h>
#include <openssl/sha.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

#define MAX_WSIZES 20
#define MAX_Sources 100

#define N_DGACLASSES 3

extern char CLASSES[N_DGACLASSES][50];

extern char WINDOWING_NAMES[3][10];

extern char NN_NAMES[11][10];

typedef int32_t IDX;

typedef enum DGAClass {
    DGACLASS_0,
    DGACLASS_1,
    DGACLASS_2,
} DGAClass;

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

typedef struct Ths {
    int32_t number;
    double* _;
} Ths;

#endif