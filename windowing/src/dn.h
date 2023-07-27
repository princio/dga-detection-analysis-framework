
#include <stdint.h>

typedef struct Message
{
    int64_t fn_req;
    double value;
    double logit;
    int is_response;
    int top10m;
    int dyndns;
} Message;

typedef struct PCAP
{
    int64_t id;
    int infected;
    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t nmessages;
} PCAP;


typedef struct Whitelisting
{
    int rank;
    double value;
} Whitelisting;

typedef enum WindowingType
{
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

typedef struct InfiniteValues
{
    double ninf;
    double pinf;
} InfiniteValues;

typedef struct Pi
{
    Whitelisting whitelisting;
    WindowingType windowing;
    InfiniteValues infinite_values;
    NN nn;
    int wsize;
} Pi;

char WINDOWING_NAMES[3][10] = {
    "QUERY",
    "RESPONSE",
    "BOTH"
};

char NN_NAMES[11][10] = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "ICANN",
    "NONE",
    "PRIVATE",
    "TLD"
};

typedef struct WindowMetrics
{
    Pi* pi;
    double logit;
    int dn_bad_05;
    int dn_bad_09;
    int dn_bad_099;
    int dn_bad_0999;
} WindowMetrics;

typedef struct Window
{
    int wnum;
    int wcount;
    int nmetrics;
    int infected;

    WindowMetrics *metrics;
} Window;

typedef struct Windowing {
    int64_t pcap_id;

    int infected;
    int nwindows;
    int wsize;
    
    Window *windows;
} Windowing;