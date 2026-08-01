# Modifications to dns_parse

This directory contains a **modified copy** of `dns_parse`, originally authored by
Paul Ferrell (`pferrell@lanl.gov`) at Los Alamos National Laboratory. See `LICENSE`
and `README` for the upstream notice and documentation.

The LANL license requires that derivative works be clearly marked so they are not
confused with the version distributed by LANL. This file is that marking.

**This is not the upstream version.** Do not report issues with it to LANL.

## What was changed

All changes were made to support the DNS/DGA analysis pipeline in the parent
repository, which needs machine-readable output rather than the upstream
human-readable ASCII format.

1. **CSV output mode (`-o <path>`).**
   A new option writes one row per DNS message to a CSV file instead of, or
   alongside, the standard textual output. Added `csv_path` / `csv_file` to the
   configuration struct (`dns_parse.h:77-78`) and the option handling in
   `dns_parse.c:148-150`.

   The header written (`dns_parse.c:332`) is:

   ```
   time,size,protocol,macsrc,macdst,src,dst,qr,AA,rcode,fnreq,
   qdcount,ancount,nscount,arcount,qcode,dn,answer
   ```

2. **MAC address columns (`macsrc`, `macdst`).**
   The upstream tool does not emit link-layer addresses. The Ethernet source and
   destination addresses are now carried out of the frame parser and written to
   the CSV, so traffic can be attributed to a terminal on the LAN.

3. **Request sequence number (`fnreq`).**
   A per-capture counter incremented per request, used downstream to pair
   responses with their queries and to order messages within a capture
   (`dns_parse.c:124`, `:564`, `:578`).

## Known local artifacts

`dns_parse.c` contains a hardcoded debug path (`/Users/princio/Desktop/psql_domains.csv`)
inside a branch that exits immediately. It is leftover scaffolding, not part of the
normal code path.
