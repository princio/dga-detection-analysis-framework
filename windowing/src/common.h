#ifndef __WWCOMMON_H__
#define __WWCOMMON_H__

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

// #define IO_DEBUG
#define LOGGING

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
#define CLOCK_START(A) printf("%60s: %s\n", "start", #A); struct timeval begin_ ## A; gettimeofday(&begin_ ## A, NULL);
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

#define MANY_INIT(A, N, T) { if ((N) == 0) printf("Warning[%s::%d]: initializing empty block.\n", __FILE__, __LINE__); \
            (A).size = (N); \
            (A).number = (N); \
            (A).element_size = sizeof(T); \
            (A)._ = calloc(A.number, sizeof(T)); }

#define MANY_INIT_0(A, N, T) MANY_INIT(A, N, T); (A).number = 0;

#define MANY_INITREF(A, N, T) { if ((N) == 0) printf("Warning[%s::%d]: initializing empty block.\n", __FILE__, __LINE__);\
            (A)->size = (N); \
            (A)->number = (N); \
            (A)->element_size = sizeof(T); \
            (A)->_ = calloc((A)->number, sizeof(T)); }

#define MANY_INITSIZE(A, N, S) { if ((N) == 0) printf("Warning[%s::%d]: initializing empty block.\n", __FILE__, __LINE__); \
            (A).size = (N); \
            (A).number = (N); \
            (A).element_size = (S); \
            (A)._ = calloc(A.number, (S)); }

#define CLONEMANY(A, B) MANY_INITSIZE((A), (B).number, (B).element_size); memcpy((A)._, (B)._, (B).element_size * (B).number);

#define FREEMANY(A) { if (A.number) free(A._); A.number = 0; A.size = 0; }
#define FREEMANYREF(A) { if (A->number) free(A->_); A->number = 0; A->size = 0; }

#define BY_SETN(BY, NAME, N) memcpy((size_t*) &BY.n.NAME, &N, sizeof(size_t));

#define BY_GET(BY, N1) BY.by ## N1._[idx ## N1]
#define BY_GET2(BY, N1, N2) BY_GET(BY_GET(BY, N1), N2)
#define BY_GET3(BY, N1, N2, N3) BY_GET2(BY_GET(BY, N1), N2, N3)
#define BY_GET4(BY, N1, N2, N3, N4) BY_GET3(BY_GET(BY, N1), N2, N3, N4)
#define BY_GET5(BY, N1, N2, N3, N4, N5) BY_GET4(BY_GET(BY, N1), N2, N3, N4, N5)

#define BY_INIT(BY, BYSUB, NAME, T) \
    MANY_INIT(BYSUB.by ## NAME, BY.n.NAME, T ## _ ## NAME);
#define BY_INIT1(BY, N1, T) \
    MANY_INIT(BY.by ## N1, BY.n.N1, T ## _ ## N1);
#define BY_INIT2(BY, N1, N2, T) \
    MANY_INIT(BY_GET(BY, N1).by ## N2, BY.n.N2, T ## _ ## N2);
#define BY_INIT3(BY, N1, N2, N3, T) \
    MANY_INIT(BY_GET2(BY, N1, N2).by ## N3, BY.n.N3, T ## _ ## N3);

#define BY_FOR(BY, NAME)\
    for (size_t idx ## NAME = 0; idx ## NAME < BY.n.NAME; idx ## NAME++)
#define BY_INITFOR(BY, BYSUB, NAME, T) BY_INIT(BY, BYSUB, NAME, T); BY_FOR(BY, NAME)


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

typedef struct __Windowing* RWindowing;
typedef struct __Window0* RWindow;
typedef struct __Source* RSource;
typedef struct __Dataset* RDataset;
typedef struct __Fold* RFold0;
typedef struct TB2W* RTB2W;
typedef struct TB2D* RTB2D;

MAKEMANY(RWindow);
MAKEMANY(RDataset);
MAKEMANY(RWindowing);
MAKEMANY(RFold0);
MAKEMANY(size_t);
MAKEMANY(double);
MAKEMANY(MANY(double));


#endif