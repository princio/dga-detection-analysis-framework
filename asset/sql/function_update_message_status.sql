CREATE OR REPLACE FUNCTION update_pcap_tables_from_select()
RETURNS void
AS $$
DECLARE
  ids INTEGER[];
  current_id INTEGER;
  table_name VARCHAR(255);
BEGIN
  -- Iterate over the retrieved IDs
  FOR current_id IN (SELECT id FROM pcap_malware WHERE infected = true)
  LOOP
    table_name := 'message_' || current_id;

    -- Execute the update statement for the current table
    EXECUTE FORMAT('UPDATE %I SET status.pcap_status=''infected''', table_name); 
  END LOOP;
END;
$$ LANGUAGE plpgsql;

SELECT update_pcap_tables_from_select();

