# CLAUDE.md

Guidance for Claude Code (and humans) working in this repository.

## What this project is

A **PhD research framework for detecting DGA (Domain Generation Algorithm) malware
by analyzing DNS traffic at scale**. It is a multi-language research codebase, not a
production system: it ingests raw packet captures (pcap), extracts and labels DNS
queries, scores domains with an LSTM classifier, stores everything in PostgreSQL, and
runs large-scale statistical/ML analysis over the result.

Because it is research code, expect: experimental dead ends kept "just in case",
multiple iterations of the same component (`suite` → `suite2`), notebooks used as
scratchpads, and notes/comments in Italian. When in doubt about intent, prefer the
newer component (see "Component status" below).

## Pipeline (how the pieces fit)

```
pcap files
  │
  ├─ dns_parse/ (C)      raw pcap ─▶ human-readable DNS records
  ├─ psltrie/ (C)        domain ─▶ registered/effective domain (Public Suffix List trie)
  ├─ lstm_dga/ (Py/TF)   domain ─▶ DGA-vs-benign score (LSTM, 4 sub-models)
  ├─ dgarchive/ (Py)     ground-truth labels — joined in SQL/notebooks, NOT in suite2
  ├─ tranco / top10m     whitelisting against top-domain lists
  │
  ▼
PostgreSQL ── populated by scripts/
  (`ti2016` main; `dns_mac` in suite2; `dns2` in the C windowing)
  │
  ├─ asset/sql/          33 SQL files: the big-data analysis layer
  ├─ windowing/ (C)      time-windowed features + k-fold validation + confusion matrices
  ├─ scripts/windowing*  Python windowing over the DB (ti2016 dataset)
  ├─ ml/ (notebooks)     analysis, datasets, simulations, FPR studies
  └─ web/mwdb/           web UI (Next.js frontend + NestJS backend + Python plotting)
```

## Components

| Path | Lang | Role |
|------|------|------|
| `dns_parse/` | C | Parse pcap → trivially-parseable ASCII DNS. Third-party (LANL, Paul Ferrell), `make` to build. |
| `psl_list/` | Python | Fetches and parses the Public Suffix List, IANA and TLD lists into `psl_list.csv`. Standalone; consumed by `psltrie/`. |
| `psltrie/` | C | Public Suffix List trie; fast registered-domain extraction. Built on a C project template (`make`, see `project.conf`). |
| `windowing/` | C | Windowed feature calculation, k-cross-fold validation, confusion matrices. `make`. See `windowing/README.md`. |
| `suite2/` | Python | **Newer** rewrite of `suite`. Prefer this. No CLI entry point (`__main__.py` is empty) — driven by `scripts/*.py`; services under `suite2/suite2/*/`. |
| `lstm_dga/` | Python/TensorFlow | LSTM DGA classifier. `predict.py` / `predict_dns_parse_output.py`. Models in `nns/`. See `lstm_dga/README.md`. |
| `dgarchive/` | Python (notebooks) | DGArchive ground-truth malware/DGA labels. |
| `ml/` | Python/Jupyter | 36 notebooks (57 repo-wide): analysis, datasets, simulations, false-positive-rate studies. |
| `scripts/` | Python | DB population, materialized views, per-dataset analysis (`ti2016`). |
| `asset/sql/` | SQL | 33 hand-written SQL files — queries, DDL and functions (Postgres 17). |
| `whitelisting/` | Python/SQL | Tranco + top10m whitelist preparation and loading. See `whitelisting/README.md`. |
| `mac_address/` | Notebook | Investigation: concluded the source MAC is **not** preserved (`CONCLUSION.md`). |
| `web/mwdb/` | TS (Next/Nest) + Py | Web UI to browse results. |
| `tikz/` | LaTeX | Thesis diagrams. |
| `run.py`, `conf.json` | Python/JSON | Top-level single-pcap pipeline runner + its config. |

### Component status (which to prefer)

- **`suite2` is the only orchestration package.** The older `suite/` was deleted; its
  public-suffix-list helper is now the standalone `psl_list/` at the repository root.
  Note `suite2/suite2/__main__.py` is a 0-byte file: the real entry points are the
  `scripts/*.py`, each of which builds its own `Suite2Container`.
- `scripts/windowing_ti2016/` is the renamed/maintained windowing; the top-level
  `scripts/windowing.py` writes output to `../output/windowing_py/`.

## Build & run

### C components (`make` in each dir)
```sh
cd dns_parse && make          # → bin/dns_parse  (needs libpcap)
cd psltrie   && make          # → bin/...        (config in project.conf)
cd windowing && make          # → bin/...        (config in project.conf)
```

### Python
- Python **3.12.12** (see `.python-version`), managed with `pyenv` + a virtualenv named `phd`.
- TensorFlow pins the stack: install it first, then the rest.
- `lstm_dga/` historically needs an **older** Python (3.8–3.11) + TensorFlow 2.13.0 —
  it is run in its own venv, separate from the main 3.12 env.

```sh
pip install -e '.[analysis]'      # psl_list + suite2 + analysis deps
python scripts/<name>.py
```

`pyproject.toml` at the root registers `psl_list` and `suite2` as real packages — the
scripts no longer touch `sys.path`. TensorFlow is the `[lstm]` extra, not a core
dependency: 2.13 is the version the models need and it does not support Python 3.12.

Requirements files: `lstm_dga/requirements.txt`, `psl_list/requirements.txt`.

### Database
PostgreSQL **17**. Datasets/DB names referenced in code: `ti2016` (main), `dns_mac`
(suite2) and `dns2` (hardcoded in `windowing/src/stratosphere*.c`). Scripts populate materialized views (`scripts/build_materialized_views.py`,
`scripts/fill_dn_columns.py`).

## Conventions & gotchas

- **Hardcoded config is everywhere.** DB credentials, and absolute paths like
  `/Users/princio/...`, are embedded in `conf.json`,
  `suite2/suite2/suite2_run.py`, `scripts/*`, etc. These are **local-dev throwaway**
  values, not secrets to protect — but they make components non-portable. When running,
  expect to edit the inline `config`/`from_dict({...})` block at the bottom of each script.
- **`DNService.db_dgarchive()` is an empty stub.** DGArchive labels reach the data only
  through SQL (`dac`, `dac_dn`, the `_compact` / `mv3_0` views) and notebooks — not through
  the `suite2` pipeline.
- **`run.py` is broken** (SQLite-style `?` placeholders with psycopg2, a duplicated INSERT
  column list, no commit). Use the `scripts/*.py` entry points.
- **Italian notes/comments** appear throughout (READMEs, comments, the root README).
- **`pass` as a block terminator**: the Python style here closes most blocks with a
  trailing `pass`. Match the surrounding style when editing.
- **Notebooks are scratchpads** — names like `Untitled.ipynb`, `... copy.ipynb`,
  `*_old.ipynb` are experimental and may not run end-to-end.
- **Many branches exist** (`gatherer`, `gatherer_windows`, `refactor`, etc.); `main`
  is the integration branch (last merged from `gatherer_windows`).
- **`backup/` is gitignored.** Local holding area for files removed from git that are
  worth keeping on disk — currently `suite/assets/models_univpm/` (byte-identical to
  `lstm_dga/nns/json_tf2.13/`). Not synced anywhere; git history is the real backup.
- **Git remotes**: two remotes point at the same GitHub repo — `origin` (SSH) and
  `origin-http` (HTTPS). Both work; `origin` is the default for push/pull.

## Portfolio roadmap

This repo is being prepared as a portfolio piece. The research work is solid; the
*packaging* is what needs work. Ordered by impact-per-effort. Don't undertake an item
unless asked — but keep the destination in mind when touching nearby code.

### Phase 1 — Make it readable (highest impact)
- [ ] **Replace the root `README.md`** (currently install-notes titled `# Smadonnone`,
  in Italian) with a clean English project README: the problem (DGA detection in DNS),
  the pipeline diagram, the tech stack, and **results** (accuracy / FPR / confusion
  matrices from `ml/` and `windowing/`).
- [ ] **Add a hero diagram** — the pcap→parse→classify→DB→analysis pipeline. Source
  art already exists in `tikz/`; export to PNG/SVG and embed it.
- [ ] **Surface results** — pull the strongest plots/tables out of the notebooks into
  the README or a `docs/` folder so a visitor sees outcomes without running anything.

### Phase 2 — Make it credible
- [ ] **Scrub hardcoded config** — move DB creds and absolute `/Users/princio/...` paths
  out of `conf.json`, `suite2/*`, `scripts/*` into a `config.example.json` /
  `.env.example` template + a small loader. (Low security risk — these are throwaway
  local creds — but high credibility cost as-is.)
- [ ] **Audit git history for real secrets** — confirm no API keys, dataset tokens, or
  private/internal IPs (an internal IP `172.26.197.241` already appears in history).
  Rotate anything real; consider history rewrite only if a genuine secret leaked.
- [ ] **Add per-component READMEs** where missing (e.g. `suite2/`, `scripts/`, `ml/`)
  with a one-paragraph "what + how to run".

### Phase 3 — Make it tidy
- [x] **Deleted the deprecated `suite/`.** Its one live part, `psl_list/`, was promoted to
  the root-level `psl_list/` (it had been reached through a tracked symlink); the other
  35 files were superseded by `suite2` and are recoverable from git history.
- [ ] **Remove scratch files** — `tmp.py`, `ml/analysis/Untitled.ipynb`,
  `ml/fpr_normal_approach copy.ipynb`, `*_old.ipynb`, etc. (move to an `archive/`
  branch if you want to keep them out of the way without losing them).
- [ ] **Prune branches** — collapse the ~10 experimental branches down to `main`
  (+ maybe one active dev branch).
- [x] **Pinned dependencies** — root `pyproject.toml` with `[analysis]`, `[lstm]` and
  `[web]` extras, and the two-environment note in `docs/SETUP.md`.

### Phase 4 — Nice-to-have
- [ ] **Reproducibility** — a tiny sample pcap + a one-command demo so a reviewer can
  run a slice of the pipeline end-to-end.
- [ ] **CI smoke test** — build the C binaries + import the Python packages on push.
- [ ] **License + citation** — add a `LICENSE` and a `CITATION.cff` linking the thesis/papers.

## Pre-public removal checklist

Audited 2026-08-03. **Nothing here is urgent while the repo is private** — but items 1, 2
and 4 must all be done in a *single* `git filter-repo` pass, before the repo is ever made
public, because deleting at the tip leaves the data reachable in history. That rewrite
changes every commit hash and invalidates existing clones and pushed branches, so it is a
one-shot operation to schedule deliberately, not something to do piecemeal.

### 1. Licensed data — must not be redistributed (blocking)

- [ ] **`asset/ml/dataset_training.tar.gz`** (7.4 MB → `dataset_training.csv`, 20 MB,
  674,898 rows `legit,class,dn`). **Confirmed by the author to contain DGArchive records
  or equivalent.** DGArchive is request-only ground truth from Fraunhofer FKIE; publishing
  this redistributes it. Tracked since its commit, so it needs history removal, not `git rm`.
  Note `docs/RESULTS.md` cites the 674,898 figure via `asset/ml/dataset_training_mterics.ipynb`
  — keep the notebook and the number, drop the data.
- [ ] **`pslregex2/data/flashstart/dataset.txt`** (12 MB) — *history only*, not in the
  working tree. Labelled `domain,is_malware,is_dga` list from **FlashStart**, a commercial
  DNS-filtering vendor. Reachable via commits `162485f` and `18cd128`.
- [ ] `asset/ctu-sme-11/Windows Client VM 1.md` — verbatim prose from the CTU-SME-11
  dataset documentation. Check the Stratosphere license (probably permissive); replace with
  a link if not.

### 2. Privacy

- [ ] **6 distinct MAC addresses** in notebook *outputs*:
  `scripts/windowing_ti2016/classification/MAINMAIN.ipynb`,
  `.../main_isolationforest.ipynb`, `scripts/windowing_ti2016/plots/main.ipynb`.
  OUIs resolve to network gear (Extreme, Cisco, Tekelec) plus two locally-administered —
  infrastructure, not end-user devices, consistent with `mac_address/CONCLUSION.md`. Low
  risk, but they identify hardware on a real university network. Strip outputs.
- [ ] **`172.26.197.241`** — internal IP, history only. Goes away with the same pass.
- Not a concern: the RFC1918 addresses in `conf.json` / `.vscode/launch.json`
  (`192.168.1.108`, `.209`) are lab VMs from the public CTU dataset. Nor are the
  credentials — the only ones present are `postgres`/`postgre` against `localhost`.
  No API keys or tokens anywhere in the tree or history.

### 3. Scratch files (safe to delete any time — no history rewrite needed)

`tmp.py` · `.vscode/tmp.txt` · `asset/tmp/compare.txt` ·
`ml/analysis/Untitled.ipynb` · `ml/pcap_analysis/Untitled.ipynb` ·
`ml/simulation/Untitled.ipynb` · `ml/traffics/Untitled.ipynb` ·
`ml/fpr_normal_approach copy.ipynb` · `ml/book/dataset4_old.ipynb` ·
`psltrie/Makefile copy` · `web/mwdb/pybackend/tmp/{sql,sql2,svg}.txt`

Keep `lstm_dga/test_model_loading.py`, `scripts/smoke_test_lstm.py`,
`windowing/test/main.c` — thin, but real tests.

### 4. Bloat and duplicates (~30 MB; only shrinks the clone if rewritten)

| Item | Size | Note |
|---|---|---|
| `ml/book/slots-pcap.svg` | 17 MB | Largest file in the repo. Re-export as PNG or drop. |
| `scripts/it16/megaplot/malware_hour.ipynb` | 8.3 MB | Bloated by embedded output images — strip outputs. |
| `ml/pcap_plot/main.ipynb`, `ml/book/translate_expand.ipynb`, `ml/rule/_.simulate_with_real_TP.ipynb` | 3.7 / 3.3 / 1.7 MB | Same. |
| `scripts/it16/megaplot/plot_malwares.svg` | 1.8 MB | PDF twin is 140 KB; figure already exported to `docs/figures/`. |
| `lstm_dga/nns/json_tf2.13/{none,tld,icann,private}/model.h5` | 4.8 MB | Duplicates the sibling `model_*.h5` in the parent dir — pick one layout. |
| `lstm_dga/nns/json/*.h5` | 4.8 MB | Pre-2.13 model set, superseded. |

Model duplication alone is ~14 MB of the 43 MB `.git`.

### Done

- [x] `web/mwdb/frontend/` — superseded CRA frontend and its nested `tmp/` duplicate
  removed in `5ebeaea` (42 files, −40,346 lines; dropped 2 of the 4 tracked lockfiles).
