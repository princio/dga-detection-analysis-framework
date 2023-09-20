
#include "windowing.h"

void windowing_fetch(WindowingPtr windowing) {
    for (int32_t i = 0; i < windowing->captures.number; ++i) {
        windowing->captures._[i].fetch("", &windowing, i);
    }
}