#ifndef __WWCOMMON_H__
#define __WWCOMMON_H__

#include <stdint.h>
#include <time.h>
#include <openssl/sha.h>
#include <stdlib.h>

#define N_WINDOWS(FNREQ_MAX, WSIZE) ((FNREQ_MAX + 1) / WSIZE + ((FNREQ_MAX + 1) % WSIZE > 0)) // +1 because it starts from 0

#define MAX_WSIZES 20
#define MAX_Sources 100

#define N_DGACLASSES 3
#define DGABINARY(cl) (cl > 0)

#define DGACURSOR ID_DGA_cursor

#define BINFOR(bn) for (int32_t bn = 0; bn < 2; bn++)
#define DGAFOR(cl) for (int32_t cl = 0; cl < N_DGACLASSES; cl++)

#define TCP_(T) T const *
#define TCPC(T) T const * const
#define T_PC(T) T * const

#define __CONCATENATE(a,b) a ## b
#define CONCATENATE(x,y) __CONCATENATE(x,y)

#define REF(A) CONCATENATE(R, A)
#define DGA(A) CONCATENATE(A, __dga)
#define MANY(T) CONCATENATE(T, __s)
#define TETRA(T) CONCATENATE(T, __tetra)

#define MAKEMANY(T) typedef struct MANY(T) {\
    size_t size; \
    size_t number; \
    size_t element_size; \
    T* _; \
} MANY(T)

#define MAKETETRA(T) typedef struct TETRA(T) {\
    T all;\
    T binary[2];\
    T multi[N_DGACLASSES];\
} TETRA(T)


#define INITMANY(A, N, T) { if ((N) == 0) printf("Warning[%s::%d]: initializing empty block.\n", __FILE__, __LINE__); \
            (A).size = (N); \
            (A).number = (N); \
            (A).element_size = sizeof(T); \
            (A)._ = calloc(A.number, sizeof(T)); }

#define INITMANYREF(A, N, T) { if ((N) == 0) printf("Warning[%s::%d]: initializing empty block.\n", __FILE__, __LINE__);\
            (A)->size = (N); \
            (A)->number = (N); \
            (A)->element_size = sizeof(T); \
            (A)->_ = calloc((A)->number, sizeof(T)); }

#define INITMANYSIZE(A, N, S) { if ((N) == 0) printf("Warning[%s::%d]: initializing empty block.\n", __FILE__, __LINE__); \
            (A).size = (N); \
            (A).number = (N); \
            (A).element_size = (S); \
            (A)._ = calloc(A.number, (S)); }

#define CLONEMANY(A, B) INITMANYSIZE((A), (B).number, (B).element_size); memcpy((A)._, (B)._, (B).element_size * (B).number);

#define FREEMANY(A) { if (A.number) free(A._); A.number = 0; A.size = 0; }
#define FREEMANYREF(A) { if (A->number) free(A->_); A->number = 0; A->size = 0; }

typedef int32_t IDX;

typedef struct Index {
    size_t all;
    size_t binary[2];
    size_t multi[N_DGACLASSES];
} Index;

typedef struct __MANY {
    size_t size;
    size_t number;
    size_t element_size;
    uint8_t* _;
} __MANY;

inline int32_t dgamany_number_sum(__MANY many[N_DGACLASSES]){
    int32_t sum = 0;
    for (int32_t cl = 0; cl < N_DGACLASSES; cl++) {
        sum += many[cl].number;
    }
    return sum;
}

typedef enum BinaryClass {
    BINARYCLASS_0,
    BINARYCLASS_1
} BinaryClass;

typedef enum DGAClass {
    DGACLASS_0,
    DGACLASS_1,
    DGACLASS_2,
} DGAClass;

typedef struct WClass {
    BinaryClass bc;
    DGAClass mc;
} WClass;

typedef struct DNSMessage {
    int64_t id;
    int64_t fn_req;
    double value;
    double logit;
    int32_t is_response;
    int32_t top10m;
    int32_t dyndns;
    int32_t rcode;
} DNSMessage;

//   P A R A M E T E R S

typedef struct WSize {
    size_t index;
    size_t value;
} WSize;

MAKEMANY(WSize);

typedef struct Whitelisting {
    size_t rank;
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

typedef struct  {
    int32_t number;
    double* _;
} DGA__s;

extern char CLASSES[N_DGACLASSES][50];

extern char WINDOWING_NAMES[3][10];

extern char NN_NAMES[11][10];

typedef struct __Windowing* RWindowing;
typedef struct __Window0* RWindow0;
typedef struct __Source* RSource;
typedef struct __Dataset0* RDataset0;
typedef struct __Fold* RFold0;
typedef struct __TestBed2* RTestBed2;

MAKEMANY(RWindow0);
MAKEMANY(RDataset0);
MAKEMANY(RWindowing);
MAKEMANY(RFold0);
MAKEMANY(double);
MAKEMANY(MANY(double));

#endif