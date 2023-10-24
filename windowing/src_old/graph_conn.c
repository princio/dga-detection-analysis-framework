#include "graph.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define N_LABELS 15

typedef struct IPc {
    int labels[N_LABELS][2];
} IPc;

typedef struct ConnB {
    char id[40];
    char ip1[20];
    char ip2[20];
    int conn12[N_LABELS][2];
    int conn21[N_LABELS][2];
} ConnB;

typedef struct Conns {
    int number;
    ConnB* _;
} Conns;

char Labels[N_LABELS][30] = {
    "BENIGN",
    "DoS slowloris",
    "Heartbleed",
    "DoS Slowhttptest",
    "DoS Hulk",
    "DoS GoldenEye",
    "Infiltration",
    "Web Attack - Brute Force",
    "Web Attack - XSS",
    "Web Attack - Sql Injection",
    "PortScan",
    "DDoS",
    "Bot",
    "SSH-Patator",
    "FTP-Patator"
};

int get_field_end(char * row, int cursor) {
    int cursor_end;
    
    char * location_end = strchr(row + cursor, ',');

    if (location_end) {
        cursor_end = location_end - row;
    } else {
        cursor_end = strlen(row);
    }

    if (cursor_end > (int) strlen(row)) {
        printf("Something is wrong: %d > %zu [str len]", cursor_end, strlen(row));
        cursor_end = strlen(row);
    }

    return cursor_end;
}

int ips_exist(Conns* ips, char* ip, int *idx) {
    for (int i = 0; i < ips->number; ++i) {
        if (0 == strcmp(ips->_[i].id, ip)) {
            *idx = i;
            return 1;
        }
    }
    return 0;
}

int ips_add(Conns* ips, char ip1[20], char ip2[20], int *idx) {
    char id[40];

    sprintf(id, "%s/%s", ip1, ip2);

    if (0 == ips_exist(ips, id, idx)) {
        strcpy(ips->_[ips->number].id, id);
        strcpy(ips->_[ips->number].ip1, ip1);
        strcpy(ips->_[ips->number].ip2, ip2);
        *idx = ips->number;
        ++ips->number;
        return 1;
    }
    return 0;
}

int ips_add2(Conns* ips, GRow* gr) {
    int idx = 0;
    int switched = 0;

    int correct = gr->predicted == gr->binary;

    char *ip1, *ip2;
    if (strcmp(gr->src, gr->dst) > 0) {
        ip1 = gr->dst;
        ip2 = gr->src;
        switched = 1;
    } else {
        ip1 = gr->src;
        ip2 = gr->dst;
    }

    ips_add(ips, ip1, ip2, &idx);

    if (switched) {
        ips->_[idx].conn21[gr->glabel][correct] += 1;
    } else {
        ips->_[idx].conn12[gr->glabel][correct] += 1;
    }

    return 0;
}


int graph_parse() {

    char buffer[1024];

    Conns ips;

    {
        FILE* fp = fopen("/home/princio/Desktop/lavoro/graph/CIC_IDS_s1000_mode0_test_2_cm.csv", "r");

        if (!fp){
            printf("Can't open file\n");
            exit(1);
        }

        int lines = -1;
        while(fgets(buffer, 1024, fp)) {
            ++lines;
        }
        ips.number = 0;
        ips._ = calloc(lines, sizeof(ConnB));


        GRow* grows = calloc(lines, sizeof(GRow));
        if (!grows){
            printf("Can't open file\n");
            exit(1);
        }

        rewind(fp);

        int line_number = 0;
        fgets(buffer, 1024, fp); // skip the header
        while(fgets(buffer, 1024, fp)) {
            GRow* grow = &grows[line_number];
            int cursor = 0;
            int cursor_end = 0;

            // Source_IP
            {
                int l;
                cursor_end = get_field_end(buffer, cursor);
                l = cursor_end - cursor;
                memcpy(grow->src, &buffer[cursor], l);
                cursor = cursor_end + 1;
            }
            
            // Destination_IP
            {
                int l;
                cursor_end = get_field_end(buffer, cursor);
                l = cursor_end - cursor;
                memcpy(grow->dst, &buffer[cursor], l);
                cursor = cursor_end + 1;
            }
            
            // Timestamp
            {
                cursor_end = get_field_end(buffer, cursor);
                cursor = cursor_end + 1;
            }


            // Label
            {
                int l;
                cursor_end = get_field_end(buffer, cursor);
                l = cursor_end - cursor;
                memcpy(grow->label, &buffer[cursor], l);
                cursor = cursor_end + 1;
                for (int i = 0; i < N_LABELS; i++)
                {
                    if (!strcmp(Labels[i], grow->label)) {
                        grow->glabel = i;
                        break;
                    }
                }
                
            }


            // Binary_Label
            {
                int l;
                cursor_end = get_field_end(buffer, cursor);
                l = cursor_end - cursor;
                char bl[5];
                memcpy(bl, &buffer[cursor], l);
                bl[l] = '\0';
                grow->binary = atoi(bl);
                cursor = cursor_end + 1;
            }


            // y_pred
            {
                int l;
                cursor_end = get_field_end(buffer, cursor);
                l = cursor_end - cursor;
                char bl[5];
                memcpy(bl, &buffer[cursor], l);
                bl[l] = '\0';
                grow->predicted = atoi(bl);
                cursor = cursor_end + 1;
            }


            ips_add2(&ips, grow);

            if (++line_number % 100000 == 0) printf("%d/%d\n", lines - line_number, lines);
        }
        fclose(fp);
    }

    FILE* fpp = fopen("df_conn_test.csv", "w");


    {
        fprintf(fpp, "-,-,");
        // for (int j = 0; j < N_LABELS; j++) {
            // fprintf(fpp, "src,src,");
        // }
        fprintf(fpp, "ip1->ip2,ip1->ip2,ip1->ip2,ip1->ip2,ip1->ip2,");
        // for (int j = 0; j < N_LABELS; j++) {
            // fprintf(fpp, "dst,dst,");
        // }
        fprintf(fpp, "ip2->ip1,ip2->ip1,ip2->ip1,ip2->ip1,ip2->ip1,");
        fprintf(fpp, "both,both,both,both,both\n");
    }

    {
        fprintf(fpp, "ip1,ip2,");
        // for (int j = 0; j < N_LABELS; j++) {
        //     fprintf(fpp, "f,t,");
        // }
        fprintf(fpp, "tn,fp,fn,tp,nattacks,");
        // for (int j = 0; j < N_LABELS; j++) {
        //     fprintf(fpp, "f,t,");
        // }
        fprintf(fpp, "tn,fp,fn,tp,nattacks,");
        fprintf(fpp, "tn,fp,fn,tp,nattacks\n");
    }

    for (int i = 0; i < ips.number; i++)
    {
        fprintf(fpp, "\"%s\",", ips._[i].ip1);
        fprintf(fpp, "\"%s\",", ips._[i].ip2);

        int tn= 0;
        int fp = 0;
        int fn = 0;
        int tp = 0;
        int nattacks = 0;
        {
            int tn_src = 0;
            int fp_src = 0;
            int fn_src = 0;
            int tp_src = 0;
            int nattacks_src = 0;

            for (int j = 0; j < N_LABELS; j++) {
                int f = ips._[i].conn12[j][0];
                int t = ips._[i].conn12[j][1];
                // fprintf(fpp, "%d,%d,", f, t);
                if (j == BENIGN) {
                    fp_src += f;
                    tn_src += t;
                } else {
                    fn_src += f;
                    tp_src += t;
                    nattacks_src += ((f + t) > 0);
                }
            }
            fprintf(fpp, "%d,%d,%d,%d,%d,", tn_src,fp_src,fn_src,tp_src,nattacks_src);
            tn += tn_src;
            fp +=  fp_src;
            fn +=  fn_src;
            tp +=  tp_src;
            nattacks +=  nattacks_src;
        }

        {
            int tn_dst = 0;
            int fp_dst = 0;
            int fn_dst = 0;
            int tp_dst = 0;
            int nattacks_dst = 0;
            for (int j = 0; j < N_LABELS; j++) {
                int f = ips._[i].conn21[j][0];
                int t = ips._[i].conn21[j][1];
                // fprintf(fpp, "%d,%d,", f, t);
                if (j == BENIGN) {
                    fp_dst += f;
                    tn_dst += t;
                } else {
                    fn_dst += f;
                    tp_dst += t;
                    nattacks_dst += ((f + t) > 0);
                }
            }
            fprintf(fpp, "%d,%d,%d,%d,%d,", tn_dst,fp_dst,fn_dst,tp_dst,nattacks_dst);
            tn += tn_dst;
            fp +=  fp_dst;
            fn +=  fn_dst;
            tp +=  tp_dst;
            nattacks +=  nattacks_dst;
        }
        fprintf(fpp, "%d,%d,%d,%d,%d\n", tn, fp, fn, tp, nattacks);
    }
    fclose(fpp);

    return 0;
}