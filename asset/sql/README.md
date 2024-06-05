# Backup

The command is:
```
pg_dump -U postgres -d dns_mac -c -C -f dump.sql -F d -j 8 -x -v -Z5
```
- j: jobs
- x: remove soemthing about security
- v verbose
- Z x: compression of x within 0,10
- c: perform clean, wipe everything already exists.
- C: create database (doesn't seem to work).
- F: type=directory

It seems very fast - also in restoring -  and size is not more than 500MB.

# Restore pgdump

The command is:
```
pg_restore -d dns_mac dump.sql -v
```

Before you need to create the database, from pgadmin or with:
```
CREATE DATABASE sas
    WITH
    OWNER = postgres
    ENCODING = 'UTF8'
    LOCALE_PROVIDER = 'libc'
    CONNECTION LIMIT = -1
    IS_TEMPLATE = False;
```

If installed postgres from Homebrew, do:
```
/opt/homebrew/Cellar/postgresql@16/16.3/bin/createuser -s postgres
```
