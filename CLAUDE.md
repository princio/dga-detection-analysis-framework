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
  ├─ dgarchive/ (Py)     ground-truth malware-family labels (DGArchive)
  ├─ tranco / top10m     whitelisting against top-domain lists
  │
  ▼
PostgreSQL (db: `ti2016`, also `dns_mac` in suite2) ── populated by scripts/
  │
  ├─ asset/sql/          35 SQL queries: the big-data analysis layer
  ├─ windowing/ (C)      time-windowed features + k-fold validation + confusion matrices
  ├─ scripts/windowing*  Python windowing over the DB (ti2016 dataset)
  ├─ ml/ (notebooks)     analysis, datasets, simulations, FPR studies
  └─ web/mwdb/           web UI (Next.js frontend + NestJS backend + Python plotting)
```

## Components

| Path | Lang | Role |
|------|------|------|
| `dns_parse/` | C | Parse pcap → trivially-parseable ASCII DNS. Third-party (LANL, Paul Ferrell), `make` to build. |
| `psltrie/` | C | Public Suffix List trie; fast registered-domain extraction. Built on a C project template (`make`, see `project.conf`). |
| `windowing/` | C | Windowed feature calculation, k-cross-fold validation, confusion matrices. `make`. See `windowing/README.md`. |
| `suite/` | Python | **Older** orchestration framework: pcap/psl/lstm/db processing. Dependency-injection based. |
| `suite2/` | Python | **Newer** rewrite of `suite`. Prefer this. Entry point `suite2/suite2/__main__.py`; services under `suite2/suite2/*/`. |
| `lstm_dga/` | Python/TensorFlow | LSTM DGA classifier. `predict.py` / `predict_dns_parse_output.py`. Models in `nns/`. See `lstm_dga/README.md`. |
| `dgarchive/` | Python (notebooks) | DGArchive ground-truth malware/DGA labels. |
| `ml/` | Python/Jupyter | 57 notebooks: analysis, datasets, simulations, false-positive-rate studies. |
| `scripts/` | Python | DB population, materialized views, per-dataset analysis (`ti2016`). |
| `asset/sql/` | SQL | 35 hand-written analysis queries (Postgres 17). |
| `tranco/`, `top10m.py` | Python | Whitelisting / top-domain lists. |
| `mac_address/` | Notebook | Investigation: concluded the source MAC is **not** preserved (`CONCLUSION.md`). |
| `web/mwdb/` | TS (Next/Nest) + Py | Web UI to browse results. |
| `tikz/` | LaTeX | Thesis diagrams. |
| `run.py`, `conf.json` | Python/JSON | Top-level single-pcap pipeline runner + its config. |

### Component status (which to prefer)

- **`suite2` supersedes `suite`.** `suite` is effectively deprecated; its own README
  notes it is no longer used by `scripts/`. Do new Python orchestration work in `suite2`.
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
pip install tensorflow==2.13.0
pip install pandas psycopg2-binary sqlalchemy dependency_injector requests tabulate
python scripts/<name>.py
```

Requirements files: `lstm_dga/requirements.txt`, `suite/psl_list/requirements.txt`.

### Database
PostgreSQL **17**. Datasets/DB names referenced in code: `ti2016` (main) and `dns_mac`
(suite2). Scripts populate materialized views (`scripts/create_materialized_view.py`,
`scripts/dbfill.py`).

## Conventions & gotchas

- **Hardcoded config is everywhere.** DB credentials, and absolute paths like
  `/Users/princio/...`, are embedded in `conf.json`, `suite/config.py`,
  `suite2/suite2/__main__.py`, `scripts/*`, etc. These are **local-dev throwaway**
  values, not secrets to protect — but they make components non-portable. When running,
  expect to edit the inline `config`/`from_dict({...})` block at the bottom of each script.
- **Italian notes/comments** appear throughout (READMEs, comments, the root README).
- **`pass` as a block terminator**: the Python style here closes most blocks with a
  trailing `pass`. Match the surrounding style when editing.
- **Notebooks are scratchpads** — names like `Untitled.ipynb`, `... copy.ipynb`,
  `*_old.ipynb` are experimental and may not run end-to-end.
- **Many branches exist** (`gatherer`, `gatherer_windows`, `refactor`, etc.); `main`
  is the integration branch (last merged from `gatherer_windows`).
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
  out of `conf.json`, `suite*/config`, `scripts/*` into a `config.example.json` /
  `.env.example` template + a small loader. (Low security risk — these are throwaway
  local creds — but high credibility cost as-is.)
- [ ] **Audit git history for real secrets** — confirm no API keys, dataset tokens, or
  private/internal IPs (an internal IP `172.26.197.241` already appears in history).
  Rotate anything real; consider history rewrite only if a genuine secret leaked.
- [ ] **Add per-component READMEs** where missing (e.g. `suite2/`, `scripts/`, `ml/`)
  with a one-paragraph "what + how to run".

### Phase 3 — Make it tidy
- [ ] **Clarify or delete the deprecated `suite/`** — either remove it or add a banner
  pointing to `suite2`. Resolve the "I don't remember the difference" ambiguity.
- [ ] **Remove scratch files** — `tmp.py`, `ml/analysis/Untitled.ipynb`,
  `ml/fpr_normal_approach copy.ipynb`, `*_old.ipynb`, etc. (move to an `archive/`
  branch if you want to keep them out of the way without losing them).
- [ ] **Prune branches** — collapse the ~10 experimental branches down to `main`
  (+ maybe one active dev branch).
- [ ] **Pin dependencies** — a top-level `requirements.txt` / `pyproject.toml` and a
  note on the two Python envs (3.12 main vs 3.8–3.11 for `lstm_dga`).

### Phase 4 — Nice-to-have
- [ ] **Reproducibility** — a tiny sample pcap + a one-command demo so a reviewer can
  run a slice of the pipeline end-to-end.
- [ ] **CI smoke test** — build the C binaries + import the Python packages on push.
- [ ] **License + citation** — add a `LICENSE` and a `CITATION.cff` linking the thesis/papers.
