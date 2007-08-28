CREATE OR REPLACE FUNCTION reprioritizeWo(INTEGER, INTEGER, BOOLEAN) RETURNS INTEGER AS '
DECLARE
  pWoid ALIAS FOR $1;
  pPriority ALIAS FOR $2;
  pChangeChildren ALIAS FOR $3;
  _status CHAR(1);
  _result INTEGER;

BEGIN

  SELECT wo_status INTO _status
  FROM wo
  WHERE (wo_id=pWoid);

  IF (NOT (_status IN (''O'', ''E''))) THEN
    return -1;
  END IF;

  UPDATE wo
  SET wo_priority=pPriority
  WHERE (wo_id=pWoid);

  IF ( (_status = ''E'') AND (pChangeChildren) ) THEN
    SELECT COALESCE(MIN(reprioritizeWo(wo_id, pPriority, TRUE)), 1) INTO _result
    FROM wo
    WHERE ( (wo_ordtype=''W'')
     AND (wo_ordid=pWoid) );
  ELSE
    _result = 1;
  END IF;

  RETURN _result;

END;
' LANGUAGE 'plpgsql';