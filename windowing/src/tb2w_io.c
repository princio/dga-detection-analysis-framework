
#include "tb2w_io.h"
// #include "logger.h"

#include <errno.h>
#include <string.h>

#define RW_CHAR (rw == IO_WRITE ? 'w' : 'r')

void tb2w_md(char fpath[PATH_MAX], const RTB2W tb2);

void tb2_io_flag_testbed(IOReadWrite rw, FILE* file, WSize wsize, int line) {
    char flag_code[IO_FLAG_LEN];
    memset(flag_code, 0, sizeof(flag_code));
    sprintf(flag_code, "testbed_%ld", wsize);
    io_flag(rw, file, flag_code, line);
}

void tb2_io_flag_config(IOReadWrite rw, FILE* file, size_t idxconfig, int line) {
    char flag_code[IO_FLAG_LEN];
    memset(flag_code, 0, sizeof(flag_code));
    sprintf(flag_code, "config_%ld", idxconfig);
    io_flag(rw, file, flag_code, line);
}

void tb2_io_flag_source(IOReadWrite rw, FILE* file, int id, int line) {
    char flag_code[IO_FLAG_LEN];
    memset(flag_code, 0, sizeof(flag_code));
    sprintf(flag_code, "source_%d", id);
    io_flag(rw, file, flag_code, line);
}

void tb2_io_flag_windowing(IOReadWrite rw, FILE* file, RWindowing windowing, int line) {
    char flag_code[IO_FLAG_LEN];
    memset(flag_code, 0, sizeof(flag_code));
    sprintf(flag_code, "windowing_%ld_%d", windowing->wsize, windowing->source->id);
    io_flag(rw, file, flag_code, line);
}

void tb2w_io_sources(IOReadWrite rw, FILE* file, RTB2W tb2) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    size_t sources_number = 0;

    if (rw == IO_WRITE) {
        sources_number = tb2->sources.number;
    }

    FRW(sources_number);

    for (size_t t = 0; t < sources_number; t++) {
        RSource source;

        tb2_io_flag_source(rw, file, 0, __LINE__);
        
        if (rw == IO_READ) {
            source = sources_alloc();
        } else {
            source = tb2->sources._[t];
        }

        FRW(source->index);
        FRW(source->galaxy);
        FRW(source->name);
        FRW(source->wclass);
        FRW(source->id);
        FRW(source->qr);
        FRW(source->q);
        FRW(source->r);
        FRW(source->fnreq_max);

        if (rw == IO_READ) {
            tb2w_source_add(tb2, source);
        }

        tb2_io_flag_source(rw, file, source->id, __LINE__);
    }
}

void tb2w_windowing_path(RTB2W tb2, RWindowing windowing, char fpath[PATH_MAX]) {
    char fname[100];
    sprintf(fname, "windowing__source_%d.bin", windowing->source->id);
    io_path_concat(tb2->rootdir, fname, fpath);
}

void tb2w_io_create(IOReadWrite rw, FILE* file, char rootdir[DIR_MAX], RTB2W* tb2w) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    WSize wsize;
    size_t n_tries = 0;

    if (rw == IO_WRITE) {
        wsize = (*tb2w)->wsize;
    }

    FRW(wsize);

    ParameterGenerator pg;
    
    if (rw == IO_WRITE) {
        memcpy(&pg, &(*tb2w)->configsuite.pg, sizeof(ParameterGenerator));
    } else {
        memset(&pg, 0, sizeof(ParameterGenerator));
    }
    {
        FRW(pg.max_size);

        #define __PG(NAME_LW, NAME_UP) \
            tb2_io_flag_config(rw, file, PE_ ## NAME_UP, __LINE__); \
            FRW(pg.NAME_LW ## _n); \
            __FRW(pg.NAME_LW, sizeof(NAME_LW ## _t) * pg.NAME_LW ## _n, file);

        __PG(ninf, NINF);
        __PG(pinf, PINF);
        __PG(nn, NN);
        __PG(wl_rank, WL_RANK);
        __PG(wl_value, WL_VALUE);
        __PG(windowing, WINDOWING);
        __PG(nx_epsilon_increment, NX_EPSILON_INCREMENT);

        #undef __PG
    }

    if (rw == IO_READ) {
        *tb2w = tb2w_create(rootdir, wsize, pg);
    }
}

void tb2_io_windows(IOReadWrite rw, FILE* file, RTB2W tb2w, MANY(RWindow) windows) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    for (size_t w = 0; w < windows.number; w++) {
        RWindow window = windows._[w];
        size_t idxwindowing;

        if (rw == IO_WRITE) {
            idxwindowing = window->windowing->index;
        }

        FRW(idxwindowing);

        FRW(window->index);
        FRW(window->duration);
        FRW(window->fn_req_min);
        FRW(window->fn_req_max);
        
        FRW(window->applies.number);

        __FRW(window->applies._, tb2w->configsuite.configs.number * sizeof(WApply), file);
    }
}

int tb2w_io_windowing(IOReadWrite rw, RTB2W tb2, RWindowing windowing) {
    FRWNPtr __FRW = rw ? io_freadN : io_fwriteN;

    char fpath[PATH_MAX];

    if (!io_direxists(tb2->rootdir)) {
        LOG_ERROR("Directory not exists: %s", tb2->rootdir);
        return -1;
    }

    tb2w_windowing_path(tb2, windowing, fpath);

    if (rw == IO_WRITE) {
        strcat(fpath, ".tmp");
    }

    FILE* file;
    file = io_openfile(rw, fpath);
    if (!file) {
        LOG_DEBUG("%s: File not found: %s.", rw == IO_WRITE ? "Writing" : "Reading", fpath);
        return -1;
    }

    if (rw == IO_WRITE) {
        LOG_TRACE("Writing: %s", fpath);
    } else {
        LOG_TRACE("Reading: %s", fpath);
    }

    tb2_io_flag_windowing(rw, file, windowing, __LINE__);

    tb2_io_windows(rw, file, tb2, windowing->windows);

    tb2_io_flag_windowing(rw, file, windowing, __LINE__);

    if(fclose(file)) {
        LOG_ERROR("%s: file %s close: %s.", rw == IO_WRITE ? "Writing" : "Reading", fpath, strerror(errno));
        return -1;
    }

    if (rw == IO_WRITE) {
        char fpath2[PATH_MAX];
        tb2w_windowing_path(tb2, windowing, fpath2);
        if (rename(fpath, fpath2)) {
            LOG_ERROR("%s: file rename (%s -> %s): %s.", rw == IO_WRITE ? "Writing" : "Reading", fpath, fpath2, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int tb2w_io(IOReadWrite rw, char rootdir[DIR_MAX], RTB2W* tb2w) {
    char fpath[PATH_MAX];

    {
        const int direxists = io_direxists(rootdir);
        if (rw == IO_WRITE) {
            if (!direxists && io_makedirs(rootdir)) {
                LOG_ERROR("Impossible to create the directory: %s", rootdir);
                return -1;
            } 
        } else
        if (!direxists) {
            LOG_ERROR("Directory not exists: %s", rootdir);
            return -1;
        }
    }

    io_path_concat(rootdir, "tb2w.bin", fpath);

    if (rw == IO_WRITE && io_fileexists(fpath)) {
        LOG_INFO("Not saving, TB2W already exists: %s", rootdir);
        return 0;
    }

    FILE* file;
    file = io_openfile(rw, fpath);
    if (file == NULL) {
        LOG_ERROR("Impossible to %s file <%s>.", rw == IO_WRITE ? "WRITE" : "READ", fpath);
        return -1;
    }

    LOG_TRACE("%s file %s.", rw == IO_WRITE ? "Writing" : "Reading", fpath);

    tb2_io_flag_testbed(rw, file, 0, __LINE__);

    tb2w_io_create(rw, file, rootdir, tb2w);

    tb2_io_flag_testbed(rw, file, (*tb2w)->wsize, __LINE__);

    tb2w_io_sources(rw, file, (*tb2w));

    tb2_io_flag_testbed(rw, file, (*tb2w)->sources.number, __LINE__);

    if (rw == IO_READ) {
        tb2w_windowing(*tb2w);
    
        tb2w_apply(*tb2w);
    }

    fclose(file);

    if (rw == IO_WRITE) {
        char fpath2[PATH_MAX];
        sprintf(fpath2, "%s.md", rootdir);
        tb2w_md(fpath2, *tb2w);
    }

    return 0;
}

void tb2w_md(char fpath[PATH_MAX], const RTB2W tb2w) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    FILE* file;

    file = fopen(fpath, "w");
    if (file == NULL) {
        LOG_WARN("impossible to open md-file <%s>.", fpath);
        return;
    }

    t = time(NULL);
    tm = *localtime(&t);

    #define FP(...) fprintf(file, __VA_ARGS__);
    #define FPNL(N, ...) fprintf(file, __VA_ARGS__); for (size_t i = 0; i < N; i++) fprintf(file, "\n");

    FPNL(2, "# TestBed");

    FPNL(3, "Saved on date: %d/%02d/%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    FPNL(2, "WSize: %ld", tb2w->wsize);

    FPNL(2, "## Sources");
    {
        const int len_header = 15;
        const char* headers[] = {
            "name",
            "galaxy",
            "wclass",
            "id",
            "qr",
            "q",
            "r",
            "fnreq_max"
        };
        const size_t n_headers = sizeof (headers) / sizeof (const char *);
        for (size_t i = 0; i < n_headers; i++) {
            FP("|%*s", len_header, headers[i]);
        }
        FPNL(1, "|");
        for (size_t i = 0; i < n_headers; i++) {
            FP("|");
            for (int t = 0; t < len_header; t++) {
                FP("-");
            }
        }
        FPNL(1, "|");
        for (size_t i = 0; i < tb2w->sources.number; i++) {
            RSource source = tb2w->sources._[i];
            FP("|%*s", len_header, source->name);
            FP("|%*s", len_header, source->galaxy);
            FP("|%*d", len_header, source->wclass.mc);
            FP("|%*d", len_header, source->id);
            FP("|%*ld", len_header, source->qr);
            FP("|%*ld", len_header, source->q);
            FP("|%*ld", len_header, source->r);
            FPNL(1, "|%*ld|", len_header, source->fnreq_max);
        }
    }

    FP("\n\n");

    FPNL(2, "## Parameters");
    size_t applies_applied_count = 0;
    {
        const int len_header = 18;
        const char* headers[] = {
            "id",
            "applied",
            "ninf",
            "pinf",
            "nn",
            "windowing",
            "nx eps increment",
            "rank",
            "value"
        };
        const size_t n_headers = sizeof (headers) / sizeof (const char *);
        for (size_t i = 0; i < n_headers; i++) {
            FP("|%*s", len_header, headers[i]);
        }
        FPNL(1, "|");
        for (size_t i = 0; i < n_headers; i++) {
            FP("|");
            for (int t = 0; t < len_header; t++) {
                FP("-");
            }
        }
        FPNL(1, "|");
        for (size_t i = 0; i < tb2w->configsuite.configs.number; i++) {
            TCPC(Config) pset = &tb2w->configsuite.configs._[i];
            FP("|%*ld", len_header, i);
            FP("|%*f", len_header,  pset->ninf);
            FP("|%*f", len_header,  pset->pinf);
            FP("|%*s", len_header,  NN_NAMES[pset->nn]);
            FP("|%*s", len_header,  WINDOWING_NAMES[pset->windowing]);
            FP("|%*f", len_header,  pset->nx_epsilon_increment);
            FP("|%*ld", len_header, pset->wl_rank);
            FPNL(1, "|%*f|", len_header, pset->wl_value);
        }
    }

    FP("\n\n");

    fclose(file);

    #undef FP
    #undef FPNL
}