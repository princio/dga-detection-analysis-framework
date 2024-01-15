#include "sources.h"

#include "gatherer.h"
// #include "logger.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

RGatherer sources_gatherer = NULL;

RSource sources_alloc() {
    if (sources_gatherer == NULL) {
        LOG_DEBUG("Gatherer: creating sources.");
        gatherer_alloc(&sources_gatherer, "sources", NULL, 50, sizeof(__Source), 10);
    }

    return (RSource) gatherer_alloc_item(sources_gatherer);
}