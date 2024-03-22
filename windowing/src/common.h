#ifndef __WWCOMMON_H__
#define __WWCOMMON_H__

#define LOGGING

#include "logger.h"

#include <curses.h>
#include <linux/limits.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>  
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

#define BENCHMARKING

#define DELLINE "\033[A\033[2K"
#define DELCHARS(N) { for (int i = 0; i < N; i++) { printf("\b"); }  }

#define DIR_MAX (PATH_MAX - 100)

WINDOW* cwin;

char CACHE_DIR[DIR_MAX];

#ifdef PRINTF_DEBUG
#define PRINTF(F, ...) printf(F, __VA_ARGS__)
#else 
#define PRINTF(F, ...) do { } while(0)
#endif

// #define IO_DEBUG

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

#define MAKEMANYNAME(NAME, T) typedef struct NAME {\
    size_t size; \
    size_t number; \
    size_t element_size; \
    T* _; \
} NAME

#define MAKETETRA(T) typedef struct TETRA(T) {\
    T all;\
    T binary[2];\
    T multi[N_DGACLASSES];\
} TETRA(T)

#ifdef BENCHMARKING
// #define CLOCK_START(A) printf("%60s: %s\n", "start", #A); struct timeval begin_ ## A; gettimeofday(&begin_ ## A, NULL);
#define CLOCK_START(A) struct timeval begin_ ## A; gettimeofday(&begin_ ## A, NULL);
#define CLOCK_END(A) { struct timeval end_ ## A; gettimeofday(&end_ ## A, NULL); \
    printf("%60s: %f ms\n", #A, ((end_ ## A.tv_sec - begin_ ## A.tv_sec) * 1000.0 + (end_ ## A.tv_usec - begin_ ## A.tv_usec) / 1000.0)); }
#elif BENCHMARKING_CLOCK
#define CLOCK_START(A) clock_t begin_ ## A = clock();
#define CLOCK_END(A) { double time_spent_ ## A = (double)(clock() - begin_ ## A) / CLOCKS_PER_SEC;\
    printf("%60s: %f µs\n", #A, time_spent_ ## A * 1000000); }
#else
#define CLOCK_START(A)
#define CLOCK_END(A)
#endif

#define MANY_INIT(A, N, T) {\
            if ((N) == 0) { LOG_WARN("MANY: initializing empty block."); }\
            else {\
            if ((A).number && (A)._) LOG_WARN("MANY: block already initialized."); \
            (A).size = (N); \
            (A).number = (N); \
            (A).element_size = sizeof(T); \
            (A)._ = calloc((A).number, sizeof(T));\
            if ((A)._ == NULL) LOG_ERROR("MANY: initialization failed");\
            }}

#define MANY_INIT_0(A, N, T) MANY_INIT(A, N, T); (A).number = 0;

#define MANY_INITREF(A, N, T) {\
            if ((N) == 0) { LOG_WARN("MANY: initializing empty block."); }\
            else {\
            if ((A)->number && (A)->_) LOG_WARN("MANY: block already initialized."); \
            (A)->size = (N); \
            (A)->number = (N); \
            (A)->element_size = sizeof(T); \
            (A)->_ = calloc((A)->number, sizeof(T)); }}

#define MANY_INITSIZE(A, N, S) {\
            if ((N) == 0) { LOG_WARN("MANY: initializing empty block."); }\
            else {\
            if ((A).number && (A)._) LOG_WARN("MANY: block already initialized."); \
            (A).size = (N); \
            (A).number = (N); \
            (A).element_size = (S); \
            (A)._ = calloc(A.number, (S)); }}

#define MANY_CLONE(DST, SRC) MANY_INITSIZE((DST), (SRC).number, (SRC).element_size); memcpy((DST)._, (SRC)._, (SRC).element_size * (SRC).number);

#define MANY_FREE(A) { if (A.number) free(A._); A.number = 0; A.size = 0; }
#define MANY_FREEREF(A) { if (A->number) free(A->_); A->number = 0; A->size = 0; }

#define BY_SETN(BY, NAME, N) memcpy((size_t*) &(BY).n.NAME, &N, sizeof(size_t));

#define MANY_FOR(OBJMANY, IDXNAME)\
    for (size_t idx ## IDXNAME = 0; idx ## IDXNAME < (OBJMANY).number; idx ## IDXNAME++)

#define MANY_GET(BY, N1) (BY).by ## N1._[idx ## N1]
#define MANY_GET2(BY, N1, N2) BY_GET(BY_GET((BY), N1), N2)
#define MANY_GET3(BY, N1, N2, N3) BY_GET2(BY_GET((BY), N1), N2, N3)
#define MANY_GET4(BY, N1, N2, N3, N4) BY_GET3(BY_GET((BY), N1), N2, N3, N4)
#define MANY_GET5(BY, N1, N2, N3, N4, N5) BY_GET4(BY_GET((BY), N1), N2, N3, N4, N5)

#define BY_GET(BY, N1) (BY).by ## N1._[idx ## N1]
#define BY_GET2(BY, N1, N2) BY_GET(BY_GET((BY), N1), N2)
#define BY_GET3(BY, N1, N2, N3) BY_GET2(BY_GET((BY), N1), N2, N3)
#define BY_GET4(BY, N1, N2, N3, N4) BY_GET3(BY_GET((BY), N1), N2, N3, N4)
#define BY_GET5(BY, N1, N2, N3, N4, N5) BY_GET4(BY_GET((BY), N1), N2, N3, N4, N5)

#define BY_INIT(BY, BYSUB, NAME, T) \
    MANY_INIT(BYSUB.by ## NAME, (BY).n.NAME, T ## _ ## NAME);
#define BY_INIT1(BY, N1, T) \
    MANY_INIT((BY).by ## N1, (BY).n.N1, T ## _ ## N1);
#define BY_INIT2(BY, N1, N2, T) \
    MANY_INIT(BY_GET((BY), N1).by ## N2, (BY).n.N2, T ## _ ## N2);
#define BY_INIT3(BY, N1, N2, N3, T) \
    MANY_INIT(BY_GET2((BY), N1, N2).by ## N3, (BY).n.N3, T ## _ ## N3);
#define BY_INIT4(BY, N1, N2, N3, N4, T) \
    MANY_INIT(BY_GET3((BY), N1, N2, N3).by ## N4, (BY).n.N4, T ## _ ## N4);
#define BY_INIT5(BY, N1, N2, N3, N4, N5, T) \
    MANY_INIT(BY_GET4((BY), N1, N2, N3, N4).by ## N5, (BY).n.N5, T ## _ ## N5);

#define BY_FOR(BY, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < (BY).n.NAME; idx ## NAME++)


#define BY_FOR1(BY, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < (BY).by ## NAME.number; idx ## NAME++)
#define BY_FOR2(BY, N1, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < BY_GET(BY, N1).by ## NAME.number; idx ## NAME++)
#define BY_FOR3(BY, N1, N2, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < BY_GET2(BY, N1, N2).by ## NAME.number; idx ## NAME++)
#define BY_FOR4(BY, N1, N2, N3, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < BY_GET3(BY, N1, N2, N3).by ## NAME.number; idx ## NAME++)
#define BY_FOR5(BY, N1, N2, N3, N4, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < BY_GET4(BY, N1, N2, N3, N4).by ## NAME.number; idx ## NAME++)

#define BY_INITFOR(BY, BYSUB, NAME, T) BY_INIT((BY), BYSUB, NAME, T); BY_FOR(BY, NAME)


typedef int32_t IDX;

typedef struct IndexMC {
    size_t all;
    size_t binary[2];
    size_t multi[N_DGACLASSES];
} IndexMC;

typedef struct __MANY {
    size_t size;
    size_t number;
    size_t element_size;
    void** _;
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

#define N_WAPPLYDNBAD 4

#define FOR_DNBAD for (size_t idxdnbad = 0; idxdnbad < N_WAPPLYDNBAD; idxdnbad++)

typedef enum WApplyDNBad {
    WAPPLYDNBAD_05,
    WAPPLYDNBAD_09,
    WAPPLYDNBAD_099,
    WAPPLYDNBAD_0999
} WApplyDNBad;

extern const double WApplyDNBad_Values[N_WAPPLYDNBAD];

//   P A R A M E T E R S

typedef size_t WSize;

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

#define N_NN 4

typedef enum NN {
    NN_NONE = 1,
    NN_TLD,
    NN_ICANN,
    NN_PRIVATE
} NN;

typedef struct DNSMessage {
    int64_t id;
    int64_t fn_req;
    int32_t is_response;
    int32_t top10m;
    int32_t dyndns;
    int32_t rcode;
    double value[N_NN];
} DNSMessage;

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

typedef struct __Source* RSource;

typedef struct __Windowing* RWindowing;
typedef struct __Window* RWindow;
typedef struct __WindowMC* RWindowMC;
typedef struct __WindowFold* RWindowFold;
typedef struct __WindowSplit* RWindowSplit;
typedef struct __Window0Many* RWindow0Many;
typedef struct __WindowMany* RWindowMany;

typedef struct __Trainer* RTrainer;
typedef struct TB2W* RTB2W;
typedef struct __TB2D* RTB2D;
typedef struct __TB2S* RTB2S;



MAKEMANY(RWindow);
MAKEMANY(RWindowing);

MAKEMANY(size_t);
MAKEMANY(int64_t);
MAKEMANY(double);
MAKEMANY(MANY(double));

typedef struct MinMax {
    double min;
    double max;
} MinMax;

MAKEMANY(MinMax);

typedef uint32_t DetectionValue;
typedef DetectionValue DetectionPN[2];

typedef struct Detection {
    double th;

    size_t dn_count[N_DGACLASSES];
    size_t dn_whitelistened_count[N_DGACLASSES];

    DetectionValue alarms[N_DGACLASSES][N_WAPPLYDNBAD];
    DetectionPN windows[N_DGACLASSES];
    DetectionPN sources[N_DGACLASSES][50];
} Detection;

MAKEMANY(Detection);

#define PN_TRUES(DTF, CL) ((DTF[CL])[CL > 0]) // 0: 0, 1-2: 1
#define PN_FALSES(DTF, CL) ((DTF[CL])[CL == 0]) // 0: 1, 1-2: 0
#define PN_TOTAL(DTF, CL) ((DTF[CL])[0] + (DTF[CL])[1])

#define PN_TRUERATIO(DET, CL) ((double) PN_TRUES((DET), CL)) / PN_TOTAL((DET), CL)
#define PN_FALSERATIO(DET, CL) ((double) PN_FALSES((DET), CL)) / PN_TOTAL((DET), CL)

#define PN_FP(DET) PN_FALSES(DET, 0)
#define PN_TN(DET) PN_TRUES(DET, 0)

#define PN_FN_CL(DET, CL) PN_FALSES(DET, CL)
#define PN_TP_CL(DET, CL) PN_TRUES(DET, CL)

#define PN_FN(DET) (PN_FALSES(DET, 1) + PN_FALSES(DET, 2))
#define PN_TP(DET) (PN_TRUES(DET, 1) + PN_TRUES(DET, 2))

MAKEMANYNAME(ManyDetection_Ths, MANY(Detection));

typedef MANY(double) ThsDataset;
typedef MANY(Detection) DetectionDataset[3];
MAKEMANY(ThsDataset);


#endif