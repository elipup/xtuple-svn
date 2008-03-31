CREATE OR REPLACE FUNCTION replaceVoidedCheck(INTEGER) RETURNS INTEGER AS '
DECLARE
  pCheckid	ALIAS FOR $1;
  _newCheckid	INTEGER;

BEGIN
  IF ( ( SELECT ( (NOT checkhead_void) OR checkhead_posted OR checkhead_replaced )
         FROM checkhead
         WHERE (checkhead_id=pCheckid) ) ) THEN
    RETURN -1;
  END IF;

  SELECT NEXTVAL(''checkhead_checkhead_id_seq'') INTO _newCheckid;

  INSERT INTO checkhead
  ( checkhead_id, checkhead_recip_id, checkhead_recip_type,
    checkhead_bankaccnt_id, checkhead_checkdate,
    checkhead_number, checkhead_amount,
    checkhead_for, checkhead_journalnumber,
    checkhead_notes,
    checkhead_misc, checkhead_expcat_id, checkhead_curr_id )
  SELECT _newCheckid, checkhead_recip_id, checkhead_recip_type,
	 checkhead_bankaccnt_id, checkhead_checkdate,
	 fetchNextCheckNumber(checkhead_bankaccnt_id), checkhead_amount,
	 checkhead_for, checkhead_journalnumber,
         checkhead_notes || ''\nReplaces voided check '' || checkhead_number,
	 checkhead_misc, checkhead_expcat_id, checkhead_curr_id
  FROM checkhead
  WHERE (checkhead_id=pCheckid);

  INSERT INTO checkitem
  ( checkitem_checkhead_id, checkitem_amount, checkitem_discount,
    checkitem_ponumber, checkitem_vouchernumber, checkitem_invcnumber,
    checkitem_apopen_id, checkitem_aropen_id,
    checkitem_docdate, checkitem_curr_id )
  SELECT _newCheckid, checkitem_amount, checkitem_discount,
         checkitem_ponumber, checkitem_vouchernumber, checkitem_invcnumber,
	 checkitem_apopen_id, checkitem_aropen_id,
	 checkitem_docdate, checkitem_curr_id
  FROM checkitem
  WHERE (checkitem_checkhead_id=pCheckid);

  UPDATE checkhead
  SET checkhead_replaced=TRUE
  WHERE (checkhead_id=pCheckid);

  RETURN _newCheckid;

END;
' LANGUAGE 'plpgsql';
