CREATE OR REPLACE FUNCTION indentedBOM(INTEGER) RETURNS INTEGER AS '
DECLARE
  pItemid ALIAS FOR $1;
  _revid INTEGER;

BEGIN

  --See if revcontrol turned on
  IF (fetchmetricbool(''RevControl'')) THEN
    SELECT getActiveRevId(''BOM'',pItemid) INTO _revid;
  ELSE
    _revid:=-1;
  END IF;

  RETURN indentedBOM(pItemid, _revid);

END;
' LANGUAGE 'plpgsql';


CREATE OR REPLACE FUNCTION indentedBOM(INTEGER, INTEGER) RETURNS INTEGER AS '
DECLARE
  pItemid ALIAS FOR $1;
  pRevisionid ALIAS FOR $2;
  _bomworkid INTEGER;
  _indexid INTEGER;
  _r RECORD;

BEGIN

--  Check on the temporary workspace
--  PERFORM maintainBOMWorkspace();

--  Grab a new index for this bomwork set
  SELECT NEXTVAL(''misc_index_seq'') INTO _indexid;

--  Step through all of the components of the passed pItemid
  FOR _r IN SELECT bomitem_seqnumber,
                   item_id, bomitem_createwo,
                   itemuomtouom(bomitem_item_id, bomitem_uom_id, NULL, bomitem_qtyper) AS qtyper, bomitem_scrap, bomitem_issuemethod,
                   bomitem_effective, bomitem_expires,
                   stdcost(item_id) AS standardcost, actcost(item_id) AS actualcost
  FROM bomitem(pItemId, pRevisionid), item
  WHERE ( (bomitem_item_id=item_id) ) LOOP

--  Insert the component and bomitem parameters
    SELECT NEXTVAL(''bomwork_bomwork_id_seq'') INTO _bomworkid;
    INSERT INTO bomwork
    ( bomwork_id, bomwork_set_id, bomwork_parent_id, bomwork_level,
      bomwork_parent_seqnumber, bomwork_seqnumber,
      bomwork_item_id, bomwork_createwo,
      bomwork_qtyper, bomwork_scrap, bomwork_issuemethod,
      bomwork_effective, bomwork_expires,
      bomwork_stdunitcost, bomwork_actunitcost )
    VALUES
    ( _bomworkid, _indexid, -1, 1,
      0, _r.bomitem_seqnumber,
      _r.item_id, _r.bomitem_createwo,
      _r.qtyper, _r.bomitem_scrap, _r.bomitem_issuemethod,
      _r.bomitem_effective, _r.bomitem_expires,
      _r.standardcost, _r.actualcost );

--  Explode the components of the current component
    PERFORM explodeBOM(_r.item_id, _bomworkid, 1);

  END LOOP;

--  Return a key to the result
  RETURN _indexid;

END;
' LANGUAGE 'plpgsql';

--DROP FUNCTION indentedBOM(INTEGER, INTEGER, INTEGER, INTEGER) ;

CREATE OR REPLACE FUNCTION indentedBOM(INTEGER, INTEGER, INTEGER, INTEGER) RETURNS SETOF indentedbom AS '
DECLARE
  pItemid ALIAS FOR $1;
  pRevisionid ALIAS FOR $2;
  pExpiredDays ALIAS FOR $3;
  pFutureDays ALIAS FOR $4;
  _row indentedbom%ROWTYPE;
  _bomworksetid INTEGER;
  _x RECORD;
  _check CHAR(1);
BEGIN

  --Is this a deactivated revision?
  SELECT rev_status INTO _check
  FROM rev
  WHERE ((rev_id=pRevisionid)
  AND (rev_status=''I''));
  
  IF (NOT FOUND) THEN

    --We can explode this out based on current data
    SELECT indentedBOM(pItemid, pRevisionid) INTO _bomworksetid;  

    FOR _x IN
        SELECT bomwork_id, bomwork_parent_id, bomwork_level,
               bomworkSequence(bomwork_id) AS seq_ord,
               bomwork_seqnumber, item_number, uom_name,
               item_descrip1, item_descrip2,
               (item_descrip1 || '' '' || item_descrip2) AS itemdescription,
               formatQtyPer(bomwork_qtyper) AS f_qtyper,
               formatScrap(bomwork_scrap) AS f_scrap,
       formatBoolYN(bomwork_createwo) as createchild,
       CASE WHEN (bomwork_issuemethod=''S'') THEN ''Push''
            WHEN (bomwork_issuemethod=''L'') THEN ''Pull''
            WHEN (bomwork_issuemethod=''M'') THEN ''Mixed''
            ELSE ''Special''
       END AS issuemethod,
       formatDate(bomwork_effective, ''Always'') AS f_effective,
       formatDate(bomwork_expires,''Never'') AS f_expires,
       CASE WHEN (bomwork_expires <= CURRENT_DATE) THEN TRUE
         ELSE FALSE
       END AS expired,
       CASE WHEN (bomwork_effective > CURRENT_DATE) THEN TRUE
         ELSE FALSE
       END AS future 
       FROM bomwork, item, uom 
       WHERE ( (bomwork_item_id=item_id)
       AND (item_inv_uom_id=uom_id)
       AND (bomwork_set_id=_bomworksetid) )
       AND (bomwork_expires > (CURRENT_DATE - pExpiredDays))
       AND (bomwork_effective <= (CURRENT_DATE + pFutureDays))
       ORDER BY seq_ord
    LOOP
        _row.indentedbom_bomwork_id := _x.bomwork_id;
        _row.indentedbom_bomwork_parent_id := _x.bomwork_parent_id;
        _row.indentedbom_bomwork_level := _x.bomwork_level;
        _row.indentedbom_bomwork_seqnumber := _x.bomwork_seqnumber;
        _row.indentedbom_item_number := _x.item_number;
        _row.indentedbom_uom_name := _x.uom_name;
        _row.indentedbom_item_descrip1 := _x.item_descrip1;
        _row.indentedbom_item_descrip2 := _x.item_descrip2;
        _row.indentedbom_itemdescription := _x.itemdescription;
        _row.indentedbom_qtyper := _x.f_qtyper;
        _row.indentedbom_scrap := _x.f_scrap;
        _row.indentedbom_createchild := _x.createchild;
        _row.indentedbom_issuemethod := _x.issuemethod;
        _row.indentedbom_effective := _x.f_effective;
        _row.indentedbom_expires := _x.f_expires;
        _row.indentedbom_expired := _x.expired;
        _row.indentedbom_future := _x.future;
        RETURN NEXT _row;
    END LOOP;
    
    PERFORM deleteBOMWorkset(_bomworksetid);

  ELSE

  END IF;

    FOR _x IN
        SELECT bomhist_id, bomhist_parent_id, bomhist_level,
               bomhistSequence(bomhist_id) AS seq_ord,
               bomhist_seqnumber, item_number, uom_name,
               item_descrip1, item_descrip2,
               (item_descrip1 || '' '' || item_descrip2) AS itemdescription,
               formatQtyPer(bomhist_qtyper) AS f_qtyper,
               formatScrap(bomhist_scrap) AS f_scrap,
       formatBoolYN(bomhist_createwo) as createchild,
       CASE WHEN (bomhist_issuemethod=''S'') THEN ''Push''
            WHEN (bomhist_issuemethod=''L'') THEN ''Pull''
            WHEN (bomhist_issuemethod=''M'') THEN ''Mixed''
            ELSE ''Special''
       END AS issuemethod,
       formatDate(bomhist_effective, ''Always'') AS f_effective,
       formatDate(bomhist_expires,''Never'') AS f_expires,
       CASE WHEN (bomhist_expires <= CURRENT_DATE) THEN TRUE
         ELSE FALSE
       END AS expired,
       CASE WHEN (bomhist_effective > CURRENT_DATE) THEN TRUE
         ELSE FALSE
       END AS future 
       FROM bomhist, item, uom 
       WHERE ( (bomhist_item_id=item_id)
       AND (item_inv_uom_id=uom_id)
       AND (bomhist_rev_id=pRevisionid) )
       AND (bomhist_expires > (CURRENT_DATE - pExpiredDays))
       AND (bomhist_effective <= (CURRENT_DATE + pFutureDays))
       ORDER BY seq_ord
    LOOP
        _row.indentedbom_bomwork_id := _x.bomhist_id;
        _row.indentedbom_bomwork_parent_id := _x.bomhist_parent_id;
        _row.indentedbom_bomwork_level := _x.bomhist_level;
        _row.indentedbom_bomwork_seqnumber := _x.bomhist_seqnumber;
        _row.indentedbom_item_number := _x.item_number;
        _row.indentedbom_uom_name := _x.uom_name;
        _row.indentedbom_item_descrip1 := _x.item_descrip1;
        _row.indentedbom_item_descrip2 := _x.item_descrip2;
        _row.indentedbom_itemdescription := _x.itemdescription;
        _row.indentedbom_qtyper := _x.f_qtyper;
        _row.indentedbom_scrap := _x.f_scrap;
        _row.indentedbom_createchild := _x.createchild;
        _row.indentedbom_issuemethod := _x.issuemethod;
        _row.indentedbom_effective := _x.f_effective;
        _row.indentedbom_expires := _x.f_expires;
        _row.indentedbom_expired := _x.expired;
        _row.indentedbom_future := _x.future;
        RETURN NEXT _row;
    END LOOP;
    

  RETURN;
END;
' LANGUAGE 'plpgsql';


