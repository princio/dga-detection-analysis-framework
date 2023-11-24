#include "tt.h"

#include "parameters.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// void bo(TT0 tt, MANY(PSet) psets) {
//     TTPsets ttpsets_k;
//     INITMANY(ttpsets_k, psets.number, TT);

//     DGAFOR(cl) {
//         MANY(Window0) windows0 = tt[cl].train;
//         RWindow windows_psets[psets.number];

//         for (int32_t p = 0; p < psets.number; p++) {
//             ttpsets_k._[p][cl].train = window_alloc(windows0.number);
//         }

//         for (int32_t w = 0; w < windows0.number; w++) {
//             for (int32_t p = 0; p < psets.number; p++) {
//                 windows_psets[p] = ttpsets_k._[p][cl].train._[w];
//             }
//             RWindow0 window0 = windows0._[w];
//             window0->source->windowfetch(window0.source, psets, window0.fn_req_min, window0.fn_req_max, window0.wnum, windows_psets);
//         }
//     }
// }