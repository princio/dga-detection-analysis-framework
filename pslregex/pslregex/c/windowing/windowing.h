
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
} PCAP;



typedef struct Window
{
    int64_t pcap_id;
    int infected;
    int wnum;
    int wsize;
    double logit;
    int dn_bad_05;
    int dn_bad_09;
    int dn_bad_099;
    int dn_bad_0999;
} Window;


int windowing();