
DO $$
DECLARE
  page_size int := 1000000;
  min_id integer;
  max_id integer;
  tot integer;
BEGIN
    RAISE NOTICE 'Getting rows to update';
	
	SELECT COUNT(*) INTO tot FROM DAC WHERE DATE_BEGIN IS NULL AND DATE_END IS NULL;
    RAISE NOTICE 'Rows to update: %', tot;
	
  SELECT
    min(id),
    max(id)
  INTO
    min_id,
    max_id
  FROM DAC WHERE DATE_BEGIN IS NULL AND DATE_END IS NULL;
    RAISE NOTICE 'Min id: %, max_id: %', min_id, max_id;
  
  FOR index IN min_id..max_id BY page_size LOOP
    UPDATE DAC
    SET 
		DATE_BEGIN=TO_TIMESTAMP(ts_s_begin - 3600)::TIMESTAMP WITHOUT TIME ZONE,
		DATE_END=TO_TIMESTAMP(ts_s_end - 3600)::TIMESTAMP WITHOUT TIME ZONE
    WHERE id >= index AND id < index+page_size AND (DATE_BEGIN IS NULL AND DATE_END IS NULL);
    COMMIT;
    RAISE NOTICE '%', index;
  END LOOP;
END;
$$;