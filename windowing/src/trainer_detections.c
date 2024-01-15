#include "trainer_detections.h"

void trainer_detections_run(DatasetSplit split, MANY(ThsDataset) ths, size_t n_configs, MANY(Detection) detections[n_configs]) {
    CLOCK_START(calculating_detections_for_all_ths);
    for (size_t idxwindow = 0; idxwindow < split.train->windows.all.number; idxwindow++) {
        RWindow window0 = split.train->windows.all._[idxwindow];
        RSource source = window0->windowing->source;

        for (size_t idxconfig = 0; idxconfig < n_configs; idxconfig++) {
            for (size_t idxth = 0; idxth < ths._[idxconfig].number; idxth++) {
                double th = ths._[idxconfig]._[idxth];
                const int prediction = window0->applies._[idxconfig].logit >= th;
                const int infected = window0->windowing->source->wclass.mc > 0;
                const int is_true = prediction == infected;
                detections[idxconfig]._[idxth].th = th;
                detections[idxconfig]._[idxth].windows[source->wclass.mc][is_true]++;
                detections[idxconfig]._[idxth].sources[source->wclass.mc][source->index.multi][is_true]++;
            }
        }
    } // calculating detections

    CLOCK_END(calculating_detections_for_all_ths);
}