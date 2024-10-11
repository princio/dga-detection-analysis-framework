WITH RECURSIVE tables AS (
  SELECT
    c.oid AS parent,
    c.oid AS relid,
    1     AS level
  FROM pg_catalog.pg_class c
  LEFT JOIN pg_catalog.pg_inherits AS i ON c.oid = i.inhrelid
    -- p = partitioned table, r = normal table
  WHERE c.relkind IN ('p', 'r')
    -- not having a parent table -> we only get the partition heads
    AND i.inhrelid IS NULL

  UNION ALL

  SELECT
    p.parent         AS parent,
    c.oid            AS relid,
    p.level + 1      AS level
  FROM tables AS p
  LEFT JOIN pg_catalog.pg_inherits AS i ON p.relid = i.inhparent
  LEFT JOIN pg_catalog.pg_class AS c ON c.oid = i.inhrelid AND c.relispartition
  WHERE c.oid IS NOT NULL
)
SELECT
  parent ::REGCLASS                                  AS table_name,
  pg_size_pretty(sum(pg_total_relation_size(relid))) AS pretty_total_size,
  sum(pg_total_relation_size(relid))                 AS total_size,
  array_agg(relid :: REGCLASS)                       AS all_partitions
FROM tables
GROUP BY parent
ORDER BY sum(pg_total_relation_size(relid)) DESC