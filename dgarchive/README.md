# dgarchive

Ingestion of the **DGArchive** ground truth into PostgreSQL.

DGArchive (Fraunhofer FKIE) is a corpus of domains produced by reversing the domain
generation algorithms of real malware families. It is the label source for this project:
every claim of the form "this domain is DGA, and it belongs to family *X*" ultimately
comes from here.

**The data is licensed and is not in this repository.** Access is granted on request from
<https://dgarchive.caad.fkie.fraunhofer.de/>. `.gitignore` excludes the raw dump
(`2020/full/**`) and every intermediate artifact, so what is tracked here is the
*procedure*, not the corpus.

## What is here

| Path | What it does |
|---|---|
| [`2020/build_dac_tables.ipynb`](2020/build_dac_tables.ipynb) | Reads the 2020 dump and produces the `dac` / `dac_malware` tables |

## How the ingestion works

Place the DGArchive release under `2020/full/` — 93 CSV files, one per malware
configuration (`conficker_dga.csv`, `matsnu_dga.csv`, `gameover_p2p.csv`, …). Then run the
notebook. It:

1. **Reads and concatenates all 93 files.** The dump is not schema-uniform: some files
   carry 5 columns (`dn`, csv id, validity begin, validity end, family) and some only 3.
   The notebook normalises the short form by filling `ts_begin` / `ts_end` with nulls.
   Roughly 3m30s, dominated by I/O.
2. **Splits the family column.** It arrives as `<family>_dga_<hash>` — e.g.
   `tsifiri_dga_1e17a8bc` — where the hash identifies a particular *seed/configuration* of
   that family's algorithm. Family and hash are separated, then factorized into integer IDs.
3. **Writes two CSVs**: `dac_malware_table.csv` (id, hash, family) and `DF.csv` (the domains
   themselves, with `malware_id` as a foreign key). Also dumps `DF.pickle` for reuse from
   other notebooks.
4. **Prints the DDL and `COPY` statements** — as markdown cells, to be run by hand against
   the target database. Substitute the paths marked `[path-to:…]`.

The resulting schema:

```
dac_malware (id, hash, family)
    ▲
    │ malware_id
dac (id, dn, malware_id, ts_begin, ts_end, dac_csv_id, dac_original_index)
```

`ts_begin` / `ts_end` bound the window in which the domain was expected to be active — DGAs
are usually time-seeded, so a domain is only meaningful ground truth within its validity
window. `dac_original_index` preserves the row's position in the source file so a label can
always be traced back to the release.

A covering index carries the validity columns in its payload, since the common query is a
domain lookup that immediately needs the dates:

```sql
CREATE INDEX dac_dn_index ON public.dac USING btree (dn)
    INCLUDE (dn, ts_begin, ts_end) WITH (deduplicate_items = True);
```

## Attaching labels to observed traffic

The final step is a markdown cell rather than code, and it is what actually joins ground
truth to the captured DNS:

```sql
UPDATE DN SET dac_id = COALESCE(dac.id, NULL) FROM dac WHERE DN.DN = DAC.DN;
```

From that point on the labels are reachable from SQL (`asset/sql/message3_view.sql`,
`asset/sql/final/mv3_x.sql`, the `_compact` and `mv3_0` views) and from the notebooks.

> **This is the only path by which DGArchive labels enter the data.**
> `suite2`'s `DNService.db_dgarchive()` is an empty stub — the labelling is *not* part of
> the automated pipeline. Run this notebook, then the SQL, by hand.

## Notes

- **93 files is not 56 families.** The dump counts malware *configurations*, including
  several variants of the same family (`pykspa`, `pykspa2`, `pykspa2s`; `ud2`, `ud3`,
  `ud4`). The evaluation in [`../docs/RESULTS.md` §3](../docs/RESULTS.md) scored **56**
  families. Do not conflate the two numbers.
- An earlier version of this notebook (`main.ipynb`, removed) built a third table,
  `dac_family`, normalising the family name behind its own foreign key, and stored the
  validity dates as `real` epoch seconds. The epoch conversion never worked — it tried to
  reparse already-parsed timestamp strings with `unit='s'` — so this version keeps the
  dates as native `timestamp` and lets PostgreSQL parse them on `COPY`. It also drops a
  `sort_values(by='dn')` that cost three minutes and bought nothing.
