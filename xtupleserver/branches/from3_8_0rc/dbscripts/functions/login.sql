
CREATE OR REPLACE FUNCTION login() RETURNS integer AS $$
-- Copyright (c) 1999-2011 by OpenMFG LLC, d/b/a xTuple. 
-- See www.xtuple.com/CPAL for the full text of the software license.
DECLARE 
  _p RECORD;

BEGIN

  PERFORM pg_try_advisory_lock(datid::integer, procpid)
     FROM pg_stat_activity
    WHERE(procpid = pg_backend_pid());

  -- This is new to version 9.0 and higher and will error on older versions
  IF (select CAST(split_part(split_part(version(), ' ', 2),'.',1) AS integer) >= 9) THEN
    SET bytea_output TO escape;
  END IF;

  -- this is temporary until either qt fixes the postgres driver or we find &
  -- fix all of the places in our app that can write strings with backslashes
  SET standard_conforming_strings TO false;

  SELECT usr_id, userCanLogin(usr_username) AS usr_active INTO _p
  FROM usr
  WHERE (usr_username=getEffectiveXtUser());

  IF (NOT FOUND) THEN
    RETURN -1;

  ELSIF (NOT _p.usr_active) THEN
    IF(SELECT metric_value='AdminOnly'
         FROM metric
        WHERE metric_name='AllowedUserLogins') THEN
      RETURN -3;
    END IF;
    RETURN -2;
  ELSE
    RETURN 1;
  END IF;

END;
$$ LANGUAGE 'plpgsql';

