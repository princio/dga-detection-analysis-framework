# ml

Analysis notebooks. **This is a working scratchpad, not a library** — read it as a record
of what was tried, not as a set of runnable experiments.

Two things to know before you spend time here:

- **Most of these notebooks cannot be re-run.** `.gitignore` excludes `**/*.csv`, so the
  intermediate data they consume is gone. What survives is their *saved output* — the
  plots and printed tables embedded in the `.ipynb` files.
- **Exactly one notebook here backs a published number.**
  [`pcap_analysis/mw_analyzer_new.ipynb`](pcap_analysis/mw_analyzer_new.ipynb) is the
  source for [`../docs/RESULTS.md` §5](../docs/RESULTS.md), the CTU-13 capture scoring.
  Everything else is exploratory.

## What is where

| Directory | Contents | Trust |
|---|---|---|
| [`pcap_analysis/`](pcap_analysis/) | Scores CTU-13 captures and counts detections per capture | **Cited in RESULTS.md §5** |
| [`dataset/`](dataset/) | [`pcaps.md`](dataset/pcaps.md) — the CTU-13 capture inventory: malware id, SHA-256, query/response counts, `dga_ratio`, per capture | Reference data, useful |
| [`rule/`](rule/) | Simulation study: ROC/AUC of a detection rule as a function of benign traffic volume, parameterised by `mul`, `th_p`, σ | **Deliberately excluded** from RESULTS.md — inputs gone, numbers not re-derivable |
| [`pcap_plot/`](pcap_plot/) | Per-capture detection plots (`detection.pdf`, `healthy.pdf`, `malicious_s.pdf`) | Figures only |
| [`book/`](book/) | Thesis figures — slot/duration/queries-per-second plots | Figures only |
| [`traffics/`](traffics/), [`highway/`](highway/) | Simulated DNS traffic and worked examples | Exploratory |
| [`analysis/`](analysis/) | Python modules the notebooks import (`rule.py`, `plots.py`, `qt.py`, `defs.py`) | Support code |
| [`simulation/`](simulation/) | Traffic simulation | Exploratory |
| [`tikz/`](tikz/) | LaTeX diagrams. `tikz/datasets/main.tex` is a working standalone TikZ flow diagram — it is the model the pipeline figure in [`../docs/figures/`](../docs/figures/) was built from | Source |
| [`pdf/`](pdf/) | Helper scripts for assembling PDFs (`jp2latex.sh`, `transl.py`) | Utility |

`pcap_analysis/TIMELINE.md` is not documentation: it is a lab log from April 2015
recording when a Windows VM was infected and powered off. Kept because it is provenance
for the captures.

## Why the results here are thin

The strong numbers in this project come from elsewhere — the LSTM training artifacts in
`../lstm_dga/nns/`, the DGArchive evaluation in `../scripts/lstm/`, and the windowing
experiment in `../scripts/windowing_ti2016/`. This directory is where ideas were tried
before they were promoted, or after they were abandoned. The `rule/` simulation is the
clearest example: a substantial body of work whose source CSVs no longer exist, so its
conclusions cannot be checked and are not quoted.

## Environment

`.python-version` pins a different interpreter from the repository root. These notebooks
predate the packaging in `../pyproject.toml` and connect to PostgreSQL directly with
pandas and psycopg2 rather than going through `suite2`.

```sh
pip install -e '..[analysis]'   # scikit-learn, scipy, matplotlib, mlxtend, jupyter
```

Figures were removed from git where they were large and regenerable — the notebook that
wrote each one still contains the `savefig` call. See the pre-public checklist in
[`../CLAUDE.md`](../CLAUDE.md).
