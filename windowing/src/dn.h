
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

typedef struct LogitRange {
    double min;
    double max;
} LogitRange;

typedef struct ConfusionMatrix {
    double th;
    
    int tn;
    int fp;
    int fn;
    int tp;
} ConfusionMatrix;

typedef struct Pi
{
    int id;
    ConfusionMatrix *cm;
    LogitRange logit_range;
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
    
    int wcount;

    double logit;
    int dn_bad_05;
    int dn_bad_09;
    int dn_bad_099;
    int dn_bad_0999;
} WindowMetrics;

typedef struct Window
{
    int wsize;
    int wnum;
    int nmetrics;
    int infected;
    int pcap_id;

    WindowMetrics *metrics;
} Window;

typedef struct PCAPWindowing {
    int64_t pcap_id;

    int infected;
    int nwindows;
    int wsize;
    
    Window *windows;
} PCAPWindowing;

typedef struct WindowingTotals {
    int total;
    int positives;
    int negatives;
} WindowingTotals;

typedef struct WindowingDataset3 {
    Window **windows;
    int total;
} WindowingDataset3;

typedef struct WindowingDataset2 {
    WindowingDataset3 n;
    WindowingDataset3 p;
} WindowingDataset2;

typedef struct WindowingDataset {
    WindowingDataset2 train;
    WindowingDataset2 test;
    float split_percentage;
} WindowingDataset;


typedef struct AllWindows {
    Window **windows;
    Window **windows_positives;
    Window **windows_negatives;
    int wsize;
    int total;
    int total_positives;
    int total_negatives;
} AllWindows;

typedef struct  AllWindowsCursor {
    int all;
    int positives;
    int negatives;
} AllWindowsCursor;

