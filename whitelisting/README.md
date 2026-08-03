# whitelisting

Preparation and loading of the **whitelist** — the list of popular domains that a
detection is measured against.

Whitelisting is not cosmetic here. A DGA classifier that fires on `google.com` is useless,
and popular domains dominate real DNS traffic by volume, so they dominate any aggregate
score. Two places depend on this data directly:

- **`../windowing/`** — the `wl_rank` and `wl_value` parameters override a whitelisted
  domain's logit outright rather than merely down-weighting it. See
  [the parameter reference](../windowing/README.md#the-parameters).
- **The DGArchive evaluation** — the per-family TPRs in
  [`../docs/RESULTS.md` §3](../docs/RESULTS.md) are computed as
  `(eps > 0.5) & tranco_rank.isna()`, so a whitelisted domain can never count as a
  detection.

**The lists themselves are not in this repository.** Two were used:

| List | Size | Source |
|---|---|---|
| **top10m** | 7,254,902 domains (2020) | the figure quoted in the root README |
| **Tranco** | 1M | <https://tranco-list.eu/> |

## Contents

| File | Stage | What it does |
|---|---|---|
| [`build_tranco_csv.ipynb`](build_tranco_csv.ipynb) | 1 | Reshapes a downloaded Tranco list into the `whitelist_list` column order |
| [`load_top10m.py`](load_top10m.py) | 1 | The equivalent for top10m — **does not run**, see below |
| [`merge_whitelist_dn.sql`](merge_whitelist_dn.sql) | 2 | Resolves loaded domain strings against the `dn` table |

## Schema

Three tables, in two stages:

```
whitelist       (id, name, year, number)          -- the registry: which list, which year
    ▲
    │ whitelist_id
whitelist_list  (whitelist_id, dn, rank)          -- stage 1: raw domain strings
                                                  --   partitioned by whitelist_id
whitelist_dn    (id, dn_id, whitelist_id,         -- stage 2: resolved against the dn table
                 rank_dn, rank_bdn)               --   unique on (dn_id, whitelist_id)
```

**Stage 1** loads the list as text. `build_tranco_csv.ipynb` produces a CSV in the right
column order — three cells, no database connection — to be `COPY`'d in by hand, the same
pattern as the DGArchive ingestion in [`../dgarchive/`](../dgarchive/).

**Stage 2** is `merge_whitelist_dn.sql`. It joins the loaded strings against `dn` twice —
once on `dn.dn` and once on `dn.bdn` — so each domain gets *two* ranks:

- `rank_dn` — the popularity of the full domain;
- `rank_bdn` — the popularity of its registered base domain.

That split is the point. It lets `mail.google.com` inherit the standing of `google.com`
without the two being treated as the same name.

## Known problems

Both of these are recorded because the code does not record them anywhere else.

**`load_top10m.py` cannot run.** It calls `cursor.lastrowid`, a SQLite/MySQL idiom that
returns `None` under psycopg2, so the `whitelist_id` it inserts afterwards is null. It is
the same class of bug as [`../run.py`](../run.py). Treat it as a record of what was done,
not as a runnable script. Note in particular that its `7254902` is a **literal in the
INSERT statement**, not a computed count — and that literal is the source of the whitelist
size quoted in the root README and in [`../docs/RESULTS.md` §1](../docs/RESULTS.md).

**The two stages disagree about `whitelist_id`.** `build_tranco_csv.ipynb` hardcodes
`whitelist_id = 28`; `merge_whitelist_dn.sql` hardcodes `1` and reads the partition
`whitelist_list_1`. Both refer to rows in the `whitelist` registry that were created by
hand, and nothing in this repository records what either id denotes. If you re-run this,
insert your own `whitelist` row first and use its id in both places.

The Tranco source file was named `tranco_7XQJX.csv`. `7XQJX` is a Tranco list ID, so that
exact list remains retrievable at <https://tranco-list.eu/list/7XQJX>, though its date is
not recorded here.

## Related, elsewhere

- **`../scripts/score_domains.py tranco`** — scores the Tranco list through all four LSTM
  sub-models. Scoring, not loading, so it lives with the other entry points.
- **`../asset/sql/message2_dac_whitelist.sql`** — an analysis query over the result.
- **`suite/suite/whitelisting.py`** — a third implementation existed in the old `suite/`
  package: batched inserts straight into `whitelist_list`, creating a partition per list.
  It was the most complete of the three, but it imported that package's `config` and
  `utils` and could not survive its deletion. Recoverable from git history if the batching
  is ever wanted.
