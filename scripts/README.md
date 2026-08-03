# scripts

**These are the entry points.** `suite2` has no CLI — `suite2/suite2/__main__.py` is a
0-byte file — so each script here builds its own `Suite2Container`, wires it, and drives
the services directly. Every `.py` in this directory follows that shape:

```python
from suite2.container import Suite2Container
...
application = Suite2Container()
application.config.from_dict({ ... })   # <- edit this before running
```

The import works because the repository root `pyproject.toml` registers `suite2` as a real
package — run `pip install -e .` first. These scripts used to prepend `sys.path` by hand;
they no longer do.

> **Edit the config block first.** Database credentials, paths and the target database
> name are inline at the bottom of each file. Note especially `"env": "debug"`, under which
> `Database.commit()` is a **no-op** — the script runs, logs
> `Commit disabled: environment is «debug»`, and writes nothing. See
> [`../docs/SETUP.md`](../docs/SETUP.md).

## Order of operations

Populating a database from scratch:

| # | Script | What it does |
|---|---|---|
| 1 | [`ingest_pcap.py`](ingest_pcap.py) | One pcap → `tcpdump`/`tshark`/`dns_parse` → the `dn` and `message3` tables |
| 2 | [`ingest_pcap_batch.py`](ingest_pcap_batch.py) | Same for many pcaps, and scores them |
| 3 | [`fill_dn_columns.py`](fill_dn_columns.py) | Fills the `psltrie` suffix columns on `dn`, then LSTM scores into `dn_nn` |
| 4 | [`build_materialized_views.py`](build_materialized_views.py) | Builds the per-partition `*_compact` views |
| 5 | [`windowing.py`](windowing.py) | Windowed features over the database → `../output/windowing_py/` |

[`ingest_ti2016.py`](ingest_ti2016.py) does 1–3 in one pass for the `ti2016` dataset:
extracts the archive, creates a partition per day, fills columns, and joins DGArchive.

## By purpose

**Ingestion** — `ingest_pcap.py`, `ingest_pcap_batch.py`, `ingest_ti2016.py`

**Population** — `fill_dn_columns.py`, `build_materialized_views.py`

**Scoring** — [`score_domains.py`](score_domains.py) batch-scores a CSV of domain names
through all four LSTM sub-models, writing one CSV per batch and skipping batches that
already exist, so a long run resumes after a crash. Takes the dataset as an argument:

```sh
python scripts/score_domains.py dgarchive    # the DGArchive dump
python scripts/score_domains.py training     # the LSTM training set
python scripts/score_domains.py tranco       # the Tranco whitelist
```

`convert_lstm_models.py` converts the saved models between formats.

**Analysis and export** — `fpr_analysis.py` (false-positive-rate queries),
`export_ti2016_malware_daily.py`, `export_ti2016_malware_dacrank1.py` and
`export_ti2016_malware_hourly_dacrank1.py`, which write CSVs into `it16/` for the
notebooks to pick up.

**Repairs** — `fix_dn_lowercase.py` and `fix_csv_utf8.py` are one-off fixes for data
already loaded. They are not part of the normal flow.

**Smoke test** — `smoke_test_lstm.py` loads the models and scores a handful of domains.
Useful for checking an environment before a long run.

## Notebooks

The subdirectories are a different world: **14 notebooks and no Python modules**, and none
of them import `suite2`. They connect to PostgreSQL directly with pandas and psycopg2,
reading the database the scripts above populated.

| Directory | Contents |
|---|---|
| [`windowing_ti2016/`](windowing_ti2016/) | Hourly per-host windows, Random Forest + sequential feature selection. Source of [`../docs/RESULTS.md` §4](../docs/RESULTS.md) |
| [`it16/`](it16/) | Per-family traffic plots over the 10-day capture |
| [`lstm/`](lstm/) | The 120M-domain DGArchive evaluation. Source of [`../docs/RESULTS.md` §3](../docs/RESULTS.md) |

Note that `windowing_ti2016/` is the **Python** windowing — hourly windows — and is a
different implementation from the C engine in [`../windowing/`](../windowing/), which uses
fixed-count request windows. Do not read results from one as evidence about the other.

## Naming

Files are named `<verb>_<object>.py` so that listing the directory groups them by
pipeline stage. The dataset is spelled `ti2016` throughout, matching the database name —
but the *partition* names it creates are `it2016_{day}` (`ingest_ti2016.py:55`), a string
that identifies live tables and so has been left alone.
