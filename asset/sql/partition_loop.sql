DO $$ 
DECLARE
    i int;
    partizione text;
BEGIN
    FOR i IN 5..9
    LOOP
		partizione := format('message2_it2016_%s', i);
		RAISE NOTICE 'Copying %', partizione;
        EXECUTE format('COPY %I TO ''/Volumes/princio-APFS/%s.csv'' DELIMITER '','' CSV HEADER', partizione, partizione);
		RAISE NOTICE 'Copied %', partizione;
    END LOOP;
END $$;
