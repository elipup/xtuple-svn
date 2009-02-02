CREATE OR REPLACE FUNCTION enterCount(int, numeric, text) RETURNS integer AS '
DECLARE
  pInvcntid ALIAS FOR $1;
  pQty ALIAS FOR $2;
  pComments ALIAS FOR $3;
BEGIN

  UPDATE invcnt
  SET invcnt_qoh_after = pQty,
      invcnt_comments = CASE WHEN ( (LENGTH(invcnt_comments) = 0) AND
                                    (LENGTH(pComments) > 0) ) THEN pComments
                             WHEN (LENGTH(pComments) > 0) THEN (invcnt_comments || ''\n'' || pComments)
                             ELSE invcnt_comments
                        END,
      invcnt_cntdate = CURRENT_TIMESTAMP,
      invcnt_cnt_username = CURRENT_USER
  WHERE (invcnt_id=pInvcntid);

  RETURN 0;
END;
' LANGUAGE 'plpgsql';
