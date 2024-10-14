DO $$
DECLARE
    partition_number INT;        -- Variabile per il numero di partizione
    result_count INT;            -- Variabile per memorizzare il risultato di COUNT(*)
BEGIN
    -- Loop sui numeri di partizione (ad esempio da 1 a 10)
    FOR partition_number IN 0..9 LOOP

        -- Esegui la query dinamica utilizzando EXECUTE e FORMAT
        EXECUTE format('SELECT COUNT(id) FROM message2_it2016_%s', partition_number)
        INTO result_count;

        -- Stampa il risultato della query
        RAISE NOTICE 'Partition %: % rows', partition_number, result_count;

    END LOOP;
END $$;

-- NOTICE:  Partition 0: 33989215 rows
-- NOTICE:  Partition 1: 41892232 rows
-- NOTICE:  Partition 2: 45436361 rows
-- NOTICE:  Partition 3: 43513965 rows
-- NOTICE:  Partition 4: 42449729 rows
-- NOTICE:  Partition 5: 32558896 rows
-- NOTICE:  Partition 6: 32888664 rows
-- NOTICE:  Partition 7: 39173420 rows
-- NOTICE:  Partition 8: 41474268 rows
-- NOTICE:  Partition 9: 39593694 rows
-- DO

-- Query returned successfully in 5 min 51 secs.