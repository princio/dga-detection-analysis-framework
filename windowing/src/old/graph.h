

#ifndef __GRAPH_H__
#define __GRAPH_H__



typedef enum GLabel {
    BENIGN,
    DoS_slowloris,
    Heartbleed,
    DoS_Slowhttptest,
    DoS_Hulk,
    DoS_GoldenEye,
    Infiltration,
    WebAttack_BruteForce,
    WebAttack_XSS,
    WebAttack_Sql_Injection,
    PortScan,
    DDoS,
    Bot,
    SSHPatator,
    FTPPatator
} GLabel;



typedef struct GRow{
    char src[50];
    char dst[50];
    char label[50];
    GLabel glabel;
    int binary;
    int predicted;
} GRow;

int graph_parse();


#endif