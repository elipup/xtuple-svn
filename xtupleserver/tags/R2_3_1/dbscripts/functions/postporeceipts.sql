CREATE OR REPLACE FUNCTION postPoReceipts(INTEGER) RETURNS INTEGER AS '
BEGIN
  RETURN postReceipts(''PO'', $1, NULL);
END;
' LANGUAGE 'plpgsql';
