
DO $$
DECLARE
  page_size int := 1000000;
  tot integer;
  countt integer;
  summ integer;
BEGIN
    RAISE NOTICE 'Getting rows:';
	SELECT CEIL(COUNT(DISTINCT DN) / page_size::float) INTO tot FROM DAC;
    RAISE NOTICE 'Rows: %', tot;

  FOR index IN 0..tot BY 1 LOOP
SELECT
	COUNT(*),
	SUM((DN ~* '^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.)+[a-z0-9][a-z0-9-]{0,61}[a-z0-9]$')::INT) into countt, summ
FROM
	(SELECT DISTINCT ON (DN) * FROM DAC ORDER BY DN
		OFFSET page_size * (index)
		LIMIT page_size
	);
    RAISE NOTICE '%/%: count: %, sum: %', index, tot, countt, summ;
  END LOOP;
END;
$$;