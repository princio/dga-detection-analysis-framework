#include <string.h>

#include "parse_suffixes.h"

typedef struct _Label {
    int pos;
    int len;
} Label;


typedef struct _Domain {

    char domain[200];

    Label l0;
    Label l1;
    Label l2;
    Label l3;
    Label l4;

    Suffix suffix;

} Domain;

void load_domains_from_csv(char *filepath, char delimitator, int column);
