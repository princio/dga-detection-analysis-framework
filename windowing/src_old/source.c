
#include "source.h"

Windows source_init_windows(Source* source, Windowing* windowing) {
    Windows windows;

    windows.number = N_WINDOWS(source->fnreq_max, windowing->wsize.value);

    windows._ = calloc(windows.number, sizeof(Window));

    for (int widx = 0; widx < windows.number; widx++) {
        Window *window = &windows._[widx++];

        window->class = source->class;

        window->source_idx = source->id;
        window->window_idx = widx;
        window->pi_id = windowing->pset.id;
        window->wsize_idx = windowing->wsize.id;
    }
}

