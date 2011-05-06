CREATE OR REPLACE FUNCTION grantPriv(TEXT, INTEGER) RETURNS BOOL AS $$
DECLARE
  pUsername ALIAS FOR $1;
  pPrivid ALIAS FOR $2;
  _test INTEGER;

BEGIN

  SELECT usrpriv_id INTO _test
  FROM usrpriv
  WHERE ( (usrpriv_username=pUsername)
   AND (usrpriv_priv_id=pPrivid) );

  IF (FOUND) THEN
    RETURN FALSE;
  END IF;

  INSERT INTO usrpriv
  ( usrpriv_username, usrpriv_priv_id )
  VALUES
  ( pUsername, pPrivid );

  RETURN TRUE;

END;
$$ LANGUAGE 'plpgsql';

