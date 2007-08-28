CREATE OR REPLACE FUNCTION copyItem(INTEGER, TEXT) RETURNS INTEGER AS '
DECLARE
  pSItemid ALIAS FOR $1;
  pTItemNumber ALIAS FOR $2;
  _itemid INTEGER;

BEGIN

  SELECT NEXTVAL(''item_item_id_seq'') INTO _itemid;
  INSERT INTO item
  ( item_id, item_number, item_descrip1, item_descrip2,
    item_classcode_id, item_type,
    item_active, item_picklist, item_sold, item_fractional,
    item_invuom, item_capuom, item_capinvrat, item_altcapuom, item_altcapinvrat,
    item_maxcost, item_prodweight, item_packweight,
    item_prodcat_id, item_priceuom, item_invpricerat,
    item_shipuom, item_shipinvrat,
    item_taxable, item_exclusive, item_listprice,
    item_config, item_comments, item_extdescrip,
    item_upccode, item_planning_type )
  SELECT _itemid, pTItemNumber, item_descrip1, item_descrip2,
         item_classcode_id, item_type,
         item_active, item_picklist, item_sold, item_fractional,
         item_invuom, item_capuom, item_capinvrat, item_altcapuom, item_altcapinvrat,
         item_maxcost, item_prodweight, item_packweight,
         item_prodcat_id, item_priceuom, item_invpricerat,
         item_shipuom, item_shipinvrat,
         item_taxable, item_exclusive, item_listprice,
         item_config, item_comments, item_extdescrip,
         item_upccode, item_planning_type
  FROM item
  WHERE (item_id=pSItemid);

  INSERT INTO itemimage
  (itemimage_item_id, itemimage_image_id, itemimage_purpose)
  SELECT _itemid, itemimage_image_id, itemimage_purpose
  FROM itemimage
  WHERE (itemimage_item_id=pSItemid);

  INSERT INTO charass
  ( charass_target_type, charass_target_id,
    charass_char_id, charass_value )
  SELECT ''I'', _itemid, charass_char_id, charass_value
  FROM charass
  WHERE ( (charass_target_type=''I'')
   AND (charass_target_id=pSItemid) );

  RETURN _itemid;

END;
' LANGUAGE 'plpgsql';


CREATE OR REPLACE FUNCTION copyItem(integer, text, boolean, boolean, boolean) RETURNS integer AS '
DECLARE
  pSItemid ALIAS FOR $1;
  pTItemNumber ALIAS FOR $2;
  pCopyBOM ALIAS FOR $3;
  pCopyBOO ALIAS FOR $4;
  pCopyCosts ALIAS FOR $5;
BEGIN
  RETURN copyItem(pSItemid, pTItemNumber, pCopyBOM, pCopyBOO, pCopyCosts, FALSE);
END;
' LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION copyItem(integer, text, boolean, boolean, boolean, boolean) RETURNS integer AS '
DECLARE
  pSItemid ALIAS FOR $1;
  pTItemNumber ALIAS FOR $2;
  pCopyBOM ALIAS FOR $3;
  pCopyBOO ALIAS FOR $4;
  pCopyCosts ALIAS FOR $5;
  pCopyUsedAt ALIAS FOR $6;
  _itemid INTEGER;

BEGIN

  SELECT copyItem(pSItemid, pTItemNumber) INTO _itemid;

  IF (pCopyBOM) THEN
    PERFORM copyBOM(pSItemid, _itemid);
  END IF;

  IF (pCopyBOO) THEN
    PERFORM copyBOO(pSItemid, _itemid);
  END IF;

  IF (pCopyBOM AND pCopyBOO AND pCopyUsedAt) THEN
    UPDATE bomitem
       SET bomitem_booitem_id=Tbomitem_booitem_id,
           bomitem_schedatwooper=Tbomitem_schedatwooper
      FROM (SELECT tb.booitem_id AS Tbomitem_booitem_id,
                   sm.bomitem_schedatwooper AS Tbomitem_schedatwooper,
                   tm.bomitem_id AS Tbomitem_id
              FROM booitem tb, booitem sb, bomitem tm, bomitem sm, item
             WHERE ((item_id=pSItemid)
               AND  (sb.booitem_item_id=item_id)
               AND  (sm.bomitem_parent_item_id=item_id)
               AND  (sm.bomitem_booitem_id=sb.booitem_id)
               AND  (tb.booitem_item_id=_itemid)
               AND  (tm.bomitem_parent_item_id=_itemid)
               AND  (sb.booitem_seqnumber=tb.booitem_seqnumber)
               AND  (sm.bomitem_seqnumber=tm.bomitem_seqnumber))
           ) AS data
     WHERE (bomitem_id=Tbomitem_id);
  END IF;

  IF (pCopyCosts) THEN
    INSERT INTO itemcost
    ( itemcost_item_id, itemcost_costelem_id, itemcost_lowlevel,
      itemcost_stdcost, itemcost_posted,
      itemcost_actcost, itemcost_curr_id, itemcost_updated )
    SELECT _itemid, itemcost_costelem_id, itemcost_lowlevel,
      itemcost_stdcost, CURRENT_DATE,
      itemcost_actcost, itemcost_curr_id, CURRENT_DATE
    FROM itemcost
    WHERE (itemcost_item_id=pSItemid);
  END IF;

  RETURN _itemid;

END;
' LANGUAGE 'plpgsql';
