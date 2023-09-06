#ifndef __DN_H__
#define __DN_H__

#include <stdint.h>

extern char WINDOWING_NAMES[3][10];

extern char NN_NAMES[11][10];

typedef struct Message
{
    int64_t fn_req;
    double value;
    double logit;
    int32_t is_response;
    int32_t top10m;
    int32_t dyndns;
} Message;

typedef struct PCAP
{
    int64_t id;
    int32_t infected;
    int64_t qr;
    int64_t q;
    int64_t r;
    int64_t nmessages;
} PCAP;


typedef struct Whitelisting
{
    int32_t rank;
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
    
    int32_t tn;
    int32_t fp;
    int32_t fn;
    int32_t tp;
} ConfusionMatrix;

typedef struct Pi
{
    int32_t id;
    ConfusionMatrix *cm;
    LogitRange logit_range;
    Whitelisting whitelisting;
    WindowingType windowing;
    InfiniteValues infinite_values;
    NN nn;
    int32_t wsize;
} Pi;

typedef struct WindowMetrics
{
    Pi* pi;
    
    int32_t wcount;

    double logit;
    int32_t dn_bad_05;
    int32_t dn_bad_09;
    int32_t dn_bad_099;
    int32_t dn_bad_0999;
} WindowMetrics;

typedef struct Window
{
    int32_t wsize;
    int32_t wnum;
    int32_t nmetrics;
    int32_t infected;
    int32_t pcap_id;

    WindowMetrics *metrics;
} Window;

typedef struct PCAPWindowing {
    int64_t pcap_id;

    int32_t infected;
    int32_t nwindows;
    int32_t wsize;
    
    Window *windows;
} PCAPWindowing;

typedef struct WindowingTotals {
    int32_t total;
    int32_t positives;
    int32_t negatives;
} WindowingTotals;

typedef struct WindowingDataset3 {
    Window **windows;
    int32_t total;
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
    int32_t wsize;
    int32_t total;
    int32_t total_positives;
    int32_t total_negatives;
} AllWindows;

typedef struct  AllWindowsCursor {
    int32_t all;
    int32_t positives;
    int32_t negatives;
} AllWindowsCursor;

#endif