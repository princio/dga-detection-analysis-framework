DO $$
DECLARE
    r dn%ROWTYPE;                  -- Variabile per memorizzare ogni riga di 'dn'
    total_rows BIGINT;                -- Numero totale di righe da aggiornare
    count_dns_processed BIGINT := 0;        -- Contatore per tracciare le righe aggiornate
	updated_rows BIGINT := 0;
	updated_rows_1000 BIGINT := 0;
BEGIN
    -- Conta il numero di righe in 'dn' dove dac_id NON è NULL
    SELECT COUNT(*) INTO total_rows FROM dn WHERE dac_id IS NOT NULL;

    -- Stampa il numero totale di righe da aggiornare
    RAISE NOTICE 'Total rows to update: %', total_rows;

    -- Loop su tutte le righe di 'dn' dove dac_id NON è NULL
    FOR r IN
        SELECT id, dac_id FROM dn WHERE dac_id IS NOT NULL
    LOOP
        UPDATE message2_it2016_6 AS m2
        SET dac_id = r.dac_id
        WHERE m2.dn_id = r.id;

        count_dns_processed := count_dns_processed + 1;

        GET DIAGNOSTICS updated_rows = ROW_COUNT;
        updated_rows_1000 := updated_rows_1000 + updated_rows;

        IF count_dns_processed % 1000 = 0 THEN	
            RAISE NOTICE 'Processed %/% dn, updated %', count_dns_processed, total_rows, updated_rows_1000;
        	updated_rows_1000 := 0;
        END IF;
    END LOOP;

    -- Stampa il totale finale delle righe aggiornate
    RAISE NOTICE 'Total updated rows: %', count_dns_processed;
END $$;