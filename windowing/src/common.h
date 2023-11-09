#ifndef __WWCOMMON_H__
#define __WWCOMMON_H__

#include <stdint.h>
#include <time.h>
#include <openssl/sha.h>


#define MAX_WSIZES 20
#define MAX_Sources 100

#define N_DGACLASSES 3
#define DGABINARY(cl) (cl > 0)

#define DGACURSOR ID_DGA_cursor

#define DGAFOR(cl) for (int32_t cl = 0; cl < N_DGACLASSES; cl++)

#define TCP_(T) T const *
#define TCPC(T) T const * const
#define T_PC(T) T * const

#define REF(A) R ## A
#define DGA(A) A ## __dga
#define MANY(T) T ## __s
#define DGAMANY(T) T ## __dga__s
#define MANYDGA(T) T ## __s__dga
#define MANYCONST(T) c__ ## T ## __s
#define MULTICLASS(T) T ## __mc

#define MAKEMANY(T) typedef struct MANY(T){\
    int32_t number; \
    T* _; \
} MANY(T)

#define MAKEDGA(A) typedef A DGA(A)[N_DGACLASSES]
#define MAKEDGAMANY(A) typedef MANY(A) DGAMANY(A)[N_DGACLASSES]

#define MAKEMANYDGA(T) typedef struct MANYDGA(T){\
    int32_t number; \
    DGA(T)* _; \
} MANYDGA(T)

#define MAKEMANYCONST(T) typedef struct MANYCONST(T){\
    const int32_t number; \
    const T* _; \
} MANYCONST(T)

#define MANY2CONST(a, b, T) MANYCONST(T) a = { .number = b.number, ._ = b._ };

#define MAKEMULTICLASS(T) typedef struct MULTICLASS(T) {\
    T all;\
    REF(T) binary[2];\
    DGA(REF(T)) multi;\
} MULTICLASS(a)

#define INITMANY(A, N, T) A.number = N;\
            A._ = calloc(A.number, sizeof(T))

#define INITMANYREF(A, N, T) A->number = N;\
            A->_ = calloc(A->number, sizeof(T))

typedef int32_t NSourcesID_DGA[N_DGACLASSES];

typedef int32_t IDX;

typedef struct __MANY_STRUCT {
    int32_t number;
    void* _;
} __MANY_STRUCT;

inline int32_t dgamany_number_sum(__MANY_STRUCT many[N_DGACLASSES]){
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

#define DGA2BINARY(D) D > 0

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

typedef struct  {
    int32_t number;
    double* _;
} DGA__s;

extern char CLASSES[N_DGACLASSES][50];

extern char WINDOWING_NAMES[3][10];

extern char NN_NAMES[11][10];

#endif