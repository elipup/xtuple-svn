
CREATE OR REPLACE FUNCTION deleteIncident(INTEGER) RETURNS INTEGER AS '
  DECLARE
    pincdtid    ALIAS FOR $1;
    _count      INTEGER := 0;
  BEGIN
    SELECT COUNT(*) INTO _count
    FROM todoitem
    WHERE (todoitem_incdt_id=pincdtid);
    IF (_count > 0) THEN
      RETURN -1;
    END IF;

    DELETE FROM comment
     WHERE((comment_source=''INCDT'')
       AND (comment_source_id=pincdtid));

    DELETE FROM incdthist
     WHERE (incdthist_incdt_id=pincdtid);

    DELETE FROM incdt
      WHERE (incdt_id=pincdtid);

    RETURN 0;
  END;
' LANGUAGE 'plpgsql';

