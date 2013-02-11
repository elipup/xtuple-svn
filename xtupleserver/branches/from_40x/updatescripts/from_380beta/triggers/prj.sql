CREATE OR REPLACE FUNCTION _prjBeforeDeleteTrigger() RETURNS TRIGGER AS $$
DECLARE
  _recurid     INTEGER;
  _newparentid INTEGER;
BEGIN

  IF (TG_OP = 'DELETE') THEN
    SELECT recur_id INTO _recurid
      FROM recur
     WHERE ((recur_parent_id=OLD.prj_id)
        AND (recur_parent_type='J'));

    IF (_recurid IS NOT NULL) THEN
      SELECT MIN(prj_id) INTO _newparentid
        FROM prj
       WHERE ((prj_recurring_prj_id=OLD.prj_id)
          AND (prj_id!=OLD.prj_id));

      -- client is responsible for warning about deleting a recurring prj
      IF (_newparentid IS NULL) THEN
        DELETE FROM recur WHERE recur_id=_recurid;
      ELSE
        UPDATE recur SET recur_parent_id=_newparentid
         WHERE recur_id=_recurid;
      END IF;

    END IF;

    RETURN OLD;
  END IF;

  RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

SELECT dropIfExists('TRIGGER', 'prjbeforedeletetrigger');
CREATE TRIGGER prjbeforedeletetrigger
  BEFORE DELETE
  ON prj
  FOR EACH ROW
  EXECUTE PROCEDURE _prjBeforeDeleteTrigger();
