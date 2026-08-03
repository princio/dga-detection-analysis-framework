# Setup

Install and run notes for the DGA detection framework. This is research code: it was
developed across two machines (macOS and Linux) against datasets that are not
redistributable, and **it does not run out of the box**. Expect to edit paths and
database settings before anything works. See [Known rough edges](#known-rough-edges).

---

## 1. PostgreSQL

Version **17**.

After installing, set a password for the `postgres` role:

```sh
sudo -i -u postgres
psql -c "ALTER USER postgres PASSWORD 'postgres';"
```

Creating a separate, non-superuser role (via `pgadmin` or `createuser`) is recommended
rather than working as `postgres`.

Three database names appear in the code, for different components:

| Database | Used by |
|---|---|
| `ti2016` | the main dataset — `scripts/*`, most notebooks |
| `dns_mac` | `suite2`, the `web/mwdb` backend |
| `dns2` | the C `windowing` binary (hardcoded in `windowing/src/stratosphere*.c`) |

### Backup / restore

From `asset/sql/README.md` — a full dump compresses to under 500 MB:

```sh
pg_dump -U postgres -d dns_mac -c -C -f dump.sql -F d -j 8 -x -v -Z5
pg_restore -d dns_mac dump.sql -v
```

---

## 2. Python

Two separate environments are needed, because the LSTM code predates the rest.

### Main environment — Python 3.12

Managed with [pyenv](https://github.com/pyenv/pyenv) (`.python-version` pins `3.12.12`):

```sh
pyenv install 3.12.12            # install the interpreter
pyenv local 3.12.12              # activate it for this directory
pyenv virtualenv 3.12.12 phd     # create a virtualenv named "phd"
pyenv activate phd               # activate it
```

Then install the project itself, in editable mode:

```sh
pip install -e .              # psl_list and suite2, plus their dependencies
pip install -e '.[analysis]'  # adds scikit-learn, scipy, matplotlib, mlxtend, jupyter
```

That is all the path setup there is. `pyproject.toml` registers `psl_list` and `suite2`
as real packages, so `import suite2` and `import psl_list` resolve from anywhere — the
scripts in `scripts/` no longer manipulate `sys.path`, and there is no symlink to create.

> **TensorFlow is deliberately not installed by the above**, and is an optional extra
> (`pip install -e '.[lstm]'`) rather than a core dependency, because the versions do not
> reconcile. `lstm_dga/requirements.txt` pins `tensorflow==2.12.0`, both the root and
> `lstm_dga` READMEs say `2.13.0`, and the model directory actually loaded at runtime is
> `lstm_dga/nns/json_tf2.13/` — so **2.13.0** is the version to prefer. But TensorFlow
> 2.13 does not support Python 3.12, which is what the main environment is pinned to.
> Anything that loads a model therefore needs the 3.10 environment below. This is a real
> unresolved problem, recorded rather than papered over.

### LSTM environment — Python 3.10

`lstm_dga/` needs an older interpreter (`lstm_dga/.python-version` pins `3.10.14`; its
README says 3.8 or newer but *not* 3.12). Give it its own virtualenv:

```sh
cd lstm_dga
pyenv install 3.10.14
pyenv virtualenv 3.10.14 lstm
pyenv activate lstm
pip install -r requirements.txt
```


---

## 3. C components

Each builds independently with `make`. `psltrie` and `windowing` are built on
[pantuza/c-project-template](https://github.com/pantuza/c-project-template), so they use
`make debug` / `make prod` and write to `bin/binary_debug` / `bin/binary_prod`.

```sh
cd dns_parse && make          # -> bin/dns_parse        requires libpcap
cd psltrie   && make prod     # -> bin/binary_prod      (config in project.conf)
cd windowing && make prod     # -> bin/binary_prod      requires libpq, libssl, ncurses
```

`windowing` compiles against PostgreSQL headers (`-I/usr/include/postgresql/`) and links
`-lpq -lm -lssl -lcrypto -ldl -pthread -lncurses`. On Debian/Ubuntu:

```sh
sudo apt install libpcap-dev libpq-dev libssl-dev libncurses-dev
```

### Usage

```sh
# pcap -> CSV (the -o CSV mode is a local addition; see dns_parse/MODIFICATIONS.md)
dns_parse/bin/dns_parse -o out.csv input.onlydns.pcap

# split domains by public suffix: reads column 0 of in.csv
psltrie/bin/binary_prod psl_list.csv csv 0 in.csv out.csv
```

---

## 4. Running the pipeline

The entry points are the scripts in `scripts/`. Each one builds the `suite2` dependency
container itself, so **there is no single CLI** — you run a script directly:

```sh
python scripts/fill_dn_columns.py
python scripts/build_materialized_views.py
python scripts/windowing.py
```

Before running any of them, edit the configuration block near the bottom of the file:

```python
    application.config.from_dict({
        "env": "debug",
        "db": {
            "host": "localhost",
            "user": "princio",
            "password": "postgres",
            "dbname": "ti2016",
            "port": 5432
        },
        ...
```

> **`env` matters.** With `"env": "debug"`, `Database.commit()` is a no-op — the code runs
> and logs `Commit disabled: environment is «debug»` but writes nothing. Set it to
> something else to actually persist.

Typical order for populating a database from scratch:

1. `PCAPService.new_partition` — ingest pcaps: `tcpdump` → `tshark` → `dns_parse` → the
   `dn` and partitioned `message3` tables.
2. `scripts/fill_dn_columns.py` — fill in `psltrie` columns on `dn`, then LSTM scores into `dn_nn`.
3. `scripts/build_materialized_views.py` — build the per-partition `*_compact` views.
4. `scripts/windowing.py` or the `windowing` C binary — windowed features and k-fold
   experiments.

DGArchive labelling is **not** part of this sequence. `DNService.db_dgarchive()` is an
empty stub; the join to ground truth happens in SQL (`dac`, `dac_dn`, and the `_compact`
/ `mv3_0` views) and in the notebooks.

---

## 5. Datasets

None of the datasets are in this repository, and most cannot be redistributed:

| Dataset | How to get it |
|---|---|
| **DGArchive** | Request access from [Fraunhofer FKIE](https://dgarchive.caad.fkie.fraunhofer.de/) |
| **Tranco** | Download a list from [tranco-list.eu](https://tranco-list.eu/) |
| **Public Suffix List** | Fetched at runtime from `publicsuffix.org` by `psl_list/` |
| **CTU / Stratosphere pcaps** | [Stratosphere IPS](https://www.stratosphereips.org/datasets-overview) |
| **ti2016** | A private 10-day university network capture — not distributable |

`.gitignore` deliberately excludes `**/*.csv`, `output/**`, and the DGArchive dumps, so
intermediate artifacts are not tracked.

---

## Known rough edges

Being explicit so you do not lose time to them:

- **`run.py` does not work.** The top-level single-pcap runner uses SQLite-style `?`
  placeholders with `psycopg2`, calls `cursor.lastrowid`, has a duplicated column list in
  one `INSERT` (a syntax error), passes 9 placeholders for 8 columns, and never commits.
  Use the `scripts/` entry points instead.
- **`conf.json` points at the repository's old name** (`malware-detection-predict-file`)
  under macOS paths, so it needs rewriting before use.
- **`suite2/suite2/__main__.py` is empty** (0 bytes) — it is not an entry point.
- **Absolute paths are hardcoded throughout** — `/Users/princio/…`, `/home/princio/…`,
  `/media/princio/ssd512/…` — including inside C sources (`windowing/src/main.c` writes to
  `/home/princio/Desktop/…`, `psltrie` logs to `/tmp/psltrie.log`).
- **Database credentials are hardcoded** in roughly 25 files, including three C sources
  and `web/mwdb/backend/src/app.module.ts`. They are throwaway local values, not secrets.
