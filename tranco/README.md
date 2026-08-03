# tranco

Preparation of the **whitelist** — the list of popular domains that a detection is
measured against.

Whitelisting is not cosmetic here. A DGA classifier that fires on `google.com` is useless,
and popular domains dominate real DNS traffic by volume, so they dominate any aggregate
score. Two places depend on this data directly:

- **`windowing/`** — the `wl_rank` and `wl_value` parameters override a whitelisted
  domain's logit outright rather than merely down-weighting it. See
  [`../windowing/README.md`](../windowing/README.md#the-parameters).
- **The DGArchive evaluation** — the per-family TPRs in
  [`../docs/RESULTS.md` §3](../docs/RESULTS.md) are computed as
  `(eps > 0.5) & tranco_rank.isna()`, so a whitelisted domain can never count as a
  detection.

**The lists are not in this repository.** Download one from <https://tranco-list.eu/>.

## What is here

| Path | What it does |
|---|---|
| [`build_whitelist_csv.ipynb`](build_whitelist_csv.ipynb) | Reshapes a downloaded Tranco list into the `whitelist_list` column order |

Twelve lines, three cells. It reads `tranco_<id>.csv` (rank, domain), attaches a
`whitelist_id`, adds a surrogate `id`, and writes `tranco_full.csv` ready to be `COPY`'d.
There is no database connection — loading is done by hand, like the DGArchive ingestion in
[`../dgarchive/`](../dgarchive/).

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

The notebook produces **stage 1**. Stage 2 is the resolution of each domain string to a
`dn.id`, which also splits the rank in two: `rank_dn` for the full domain and `rank_bdn`
for its registered base domain — so `mail.google.com` can inherit the popularity of
`google.com`.

> **`whitelist_id = 28` is a magic number.** The notebook hardcodes it, and it is a foreign
> key into a `whitelist` row that was created by hand. Nothing in this repository records
> what list row 28 refers to. If you re-run this, insert your own `whitelist` row first and
> use its id.

Similarly, the source file is named `tranco_7XQJX.csv`. `7XQJX` is a Tranco list ID, so the
exact list is permanently retrievable at <https://tranco-list.eu/list/7XQJX> — but its date
is not recorded here.

## The other whitelist

Two lists were used. Tranco is one; the other is **top10m** (7,254,902 domains, 2020),
which is the figure quoted in the root README and in
[`../docs/RESULTS.md` §1](../docs/RESULTS.md).

Its loader is [`../top10m.py`](../top10m.py), and **it does not work.** It calls
`cursor.lastrowid`, a SQLite/MySQL idiom that returns `None` under psycopg2, so the
`whitelist_id` it then inserts is null. The `7254902` count is a literal in its `INSERT`
statement rather than something it computes. Treat it as a record of what was done, not as
a runnable script — the same caveat as [`../run.py`](../run.py).

`suite/suite/whitelisting.py` contains a third, batched implementation that inserts into
`whitelist_list` directly and creates a partition per list. It is part of the deprecated
`suite/`.
