# windowing

The host-detection engine: it turns a stream of individually-scored DNS messages into
per-window scores, sweeps a parameter space over those scores, and evaluates the result
with *k*-fold cross-validation.

There are three main steps:

- **windowing** — window value calculation for a given parameter set;
- **experiment** — *k*-cross-fold validation and confusion matrices;
- **comparing** — compares the results of each cross-fold validation, grouped by parameter.

Windowing saves and loads files depending on the parameter set and the source. The
experiment depends on the parameter sets, the sources and the folding config. Comparing
depends on both previous steps.

---

## What a window is

**A window is a fixed number of consecutive DNS requests, not a span of time.**

`windowing_build()` (`src/windowing.c:56`) partitions a source's request-sequence range
`[0, fnreq_max]` into contiguous chunks of `wsize` requests, giving
`N_WINDOWS = (fnreq_max + 1) / wsize` windows (`src/common.h:39`). The `fnreq` counter it
slices on is the per-capture request sequence number added by this project's fork of
`dns_parse` (see `../dns_parse/MODIFICATIONS.md`).

`wsize` is set at `src/main.c:129` (currently `50`); up to `MAX_WSIZES` (20) sizes can be
compared in one run. A **source** (`src/source.h`) is one capture — name, galaxy, class,
day — so windows are per-capture, and a per-host reading only holds for single-host
captures.

> The separate Python implementation in `../scripts/windowing_ti2016/` uses **hourly**
> windows with 36 features and is a different thing. The results in `../docs/RESULTS.md` §4
> come from that one, not from this engine.

## How a window is scored

Each DNS message carries four LSTM probabilities, one per sub-model. Scoring a window is a
sum of **logits** (`src/wapply.c`):

```c
logit = log(value / (1 - value));      // wapply.c:62
wapply->logit += logit * multiplier;   // wapply.c:75
```

Alongside the sum, each window keeps a 5-bin histogram of its domain scores — `dn_bad`,
binned on `{0, 0.1, 0.25, 0.5, 0.9, 1.1}` (`src/wapply.c:15`).

The problem this creates is the reason the parameter sweep exists. A saturated LSTM emits
exactly `0.0` or `1.0`, so `logit` is `±∞` and a single confident verdict would swamp the
window. Parameters 2, 3 and 6 below are the clamps, and sweeping them asks: **how much
should one domain be allowed to dominate a host's score?**

## The parameters

Eight of them — `N_PARAMETERS` in `src/configsuite.h:10`, enumerated at `:21`, carried in
`Config` at `:86`, and applied in `src/wapply.c`. Sweep values live in
`src/parameter_generator.c`.

| # | Name | Type | What it controls | Values |
|---|---|---|---|---|
| 1 | `unique` | `int` | count each domain once (`1`) or weight it by how often it was queried (`0`) | `0, 1` |
| 2 | `ninf` | `double` | finite stand-in for `logit(0) = -∞` | `0, -10, -50, -150` |
| 3 | `pinf` | `double` | finite stand-in for `logit(1) = +∞` | `0, 10, 50, 150` |
| 4 | `nn` | `NN` | which LSTM sub-model supplies the score | `NONE, TLD, ICANN, PRIVATE` |
| 5 | `wl_rank` | `size_t` | top10m rank below which a domain counts as whitelisted | `0, 100, 1000, 100000` |
| 6 | `wl_value` | `double` | logit *forced* onto a whitelisted domain | `0, -10, -50, -100, -150` |
| 7 | `windowing` | `WindowingType` | count queries, responses, or both | `Q, R, QR` |
| 8 | `nx_logit_increment` | `double` | bonus added when the response is NXDOMAIN (`rcode == 3`) | `0` (`0.05`, `0.1` present but commented out) |

`2 × 4 × 4 × 4 × 4 × 5 × 3 × 1 = ` **7,680 configurations** — `parametergenerator_default()`.
`parametergenerator_default_tiny()` reduces this to **8** for development, and
`src/main.c:118` currently selects tiny.

Note that `wsize` is *not* one of the eight; it is a separate axis, varied across runs
rather than within a sweep.

### Why these eight

- **`unique`** decides whether the signal is *which* domains a host asked for or *how
  often* — a DGA generates many distinct names, so `unique=1` emphasises breadth.
- **`ninf` / `pinf`** bound the influence of a saturated classifier output. `0` disables
  the contribution entirely; `-150` / `150` let one certain verdict carry a whole window.
- **`nn`** connects back to the §2 result in `../docs/RESULTS.md`: the sub-model that wins
  on isolated domains is not automatically the one that wins on aggregated windows.
- **`wl_rank` / `wl_value`** are the false-positive brake. Popular domains are common in
  benign traffic and dominate any sum; whitelisting overrides their logit outright rather
  than merely down-weighting it.
- **`windowing`** matters because a DGA's signature is in the *failed* lookups: queries
  without responses. `Q` and `R` separate the two sides.
- **`nx_logit_increment`** is the direct encoding of that intuition — pay a bonus for
  NXDOMAIN. It is swept at `0` only, so **this idea is defined but untested**.

All 7,680 configurations are evaluated in a **single pass over the messages** — the config
loop is nested inside the per-message loop (`src/stratosphere.c:190-196`), so the data is
read once, not 7,680 times. Results are cached with a SHA-256 of their contents stored
alongside and verified on reload (`src/gatherer2.c:218-250`).

---

## Build

```sh
make prod      # -> bin/binary_prod
make debug     # -> bin/binary_debug
```

Requires `libpq` (PostgreSQL headers at `-I/usr/include/postgresql/`), `libssl`,
`libncurses`. Configuration lives in `project.conf`.

> The database name (`dns2`) and some output paths are hardcoded in
> `src/stratosphere*.c` and `src/main.c`. See `../docs/SETUP.md`.

# Code conduct

## Function declaration

[name]([...unmodified-variables], [...modified-variables]);
