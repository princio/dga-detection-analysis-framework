#include "reducer.h"

#include "gatherer2.h"
#include "windowing.h"

#include <assert.h>
#include <math.h>

int64_t reducer_logit(double logit, const int reducer) {
    return (int64_t) floor(logit) / reducer * reducer;
}

Reducer reducer_run(const size_t nblocks, const size_t nlogits_max, const int64_t reducer) {
    LOG_INFO("Get all Logits with nblocks#%7ld, nlogits_max$%7ld, reducer#%d", nblocks, nlogits_max, reducer);
    if (nlogits_max < nblocks * 2) {
        LOG_ERROR("Block size too large relatively to nlogits_max (%ldx2 > nlogits_max)");
        exit(1);
    }

    Reducer logits;
    uint64_t blocks[nblocks];
    size_t count_logits;

    logits.n_blocks = nblocks;
    logits.n_logit_max = nlogits_max;
    logits.reducer = reducer;
    logits.many._ = NULL;
    logits.many.number = 0;

    int64_t logit_min;
    int64_t logit_max;

    __MANY windowingmany = g2_array(G2_WING);

    { // finding number of logits, maximum and minimum of logit
        count_logits = 0;
        logit_min = INT64_MAX;
        logit_max = INT64_MIN;
        for (size_t i = 0; i < windowingmany.number; i++) {
            RWindowing windowing = windowingmany._[i];

            for (size_t idxwindow = 0; idxwindow < windowing->window0many->number; idxwindow++) {
                for (size_t idxconfig = 0; idxconfig < configsuite.configs.number; idxconfig++) {
                    const int64_t logit = reducer_logit(windowing->window0many->_[idxwindow].applies._[idxconfig].logit, reducer);

                    if (logit_min > logit) logit_min = logit;
                    if (logit_max < logit) logit_max = logit;

                    count_logits++;
                }
            }
        }
    }

    LOG_INFO("count_logits %7ld", count_logits);

    const int64_t begin = logit_min;
    const int64_t end = logit_max + reducer;
    const int64_t step = (end - begin) / nblocks;

    PRINTF("Logit: min=%7ld\tmax=%7ld\tstep=%7ld\n", begin, end, step);

    { // counting the logits for each block
        memset(blocks, 0, sizeof(int64_t) * nblocks);

        __MANY windowingmany = g2_array(G2_WING);

        for (size_t i = 0; i < windowingmany.number; i++) {
            RWindowing windowing = windowingmany._[i];
            LOG_TRACE("Counting source#%4ld having %7ld windows", windowing->source->g2index, windowing->window0many->number);
            for (size_t idxwindow = 0; idxwindow < windowing->window0many->number; idxwindow++) {
                for (size_t idxconfig = 0; idxconfig < configsuite.configs.number; idxconfig++) {
                    const int64_t logit = reducer_logit(windowing->window0many->_[idxwindow].applies._[idxconfig].logit, reducer);

                    const size_t index = floor(((double) (logit - begin)) / step);

                    if (index >= nblocks) {
                        LOG_ERROR("Index bigger than number of blocks: %ld > %ld", index,  nblocks);
                    }

                    blocks[index]++;
                }
            }
        }
    }

    size_t nlogits_final = 0;
    { // counting new final logit number
        size_t nlogits = 0;
        for (size_t i = 0; i < nblocks; i++) {
            const double block_perc = (double) blocks[i] / count_logits;
            const size_t block_nlogits = block_perc ? ceil(block_perc * nlogits_max) : 1;
            nlogits += block_nlogits;
        }
        nlogits_final = nlogits + 1; // the rightest bound of the last box
    }

    assert(nlogits_final > 0);

    MANY_INIT(logits.many, nlogits_final, int64_t);

    {
        size_t nlogits = 0;
        for (size_t i = 0; i < nblocks; i++) {
            const int64_t block_begin = begin + step * i;
            const int64_t block_end = block_begin + step;

            const double block_perc = (double) blocks[i] / count_logits;
            const size_t block_nlogits = block_perc ? ceil(block_perc * nlogits_max) : 1;

            PRINTF("[ %8ld to %8ld ) = %8ld  ->  ", block_begin, block_end, blocks[i]);
            PRINTF("%15.12f%% x %ld = %15.4f\n", block_perc, nlogits_max, ceil(block_perc * nlogits_max));

            const int64_t block_step = step / block_nlogits;
            for (size_t kk = 0; kk < block_nlogits; kk++) {
                if (nlogits < nlogits_final) {
                    logits.many._[nlogits] = block_begin + block_step * kk;
                    PRINTF("%8ld -> %ld\n", nlogits, logits.many._[nlogits]);
                    nlogits++;
                } else {
                    LOG_ERROR("Maximum number of logits reached (nlogits > nlogits_final): %ld > %ld", nlogits, nlogits_final);
                }
            }
        }
        logits.many._[nlogits++] = (nblocks + 1) * step;
        PRINTF("%8ld -> %ld\n", nlogits, logits.many._[nlogits]);
        if (logits.many.number < nlogits) {
            LOG_WARN("Number of logits not correspond (logits.number <> nlogits): %ld <> %ld", logits.many.number, nlogits);
        }
        logits.many.number = nlogits;
    }

    return logits;
}