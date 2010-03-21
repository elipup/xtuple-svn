CREATE OR REPLACE FUNCTION returnShipmentTransaction(INTEGER) RETURNS INTEGER AS '
BEGIN
  RETURN returnShipmentTransaction($1, 0, CURRENT_TIMESTAMP);
END;
' LANGUAGE 'plpgsql';


CREATE OR REPLACE FUNCTION returnShipmentTransaction(INTEGER, INTEGER) RETURNS INTEGER AS '
BEGIN
  RETURN returnShipmentTransaction($1, $2, CURRENT_TIMESTAMP);
END;
' LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION returnShipmentTransaction(INTEGER, INTEGER, TIMESTAMP WITH TIME ZONE) RETURNS INTEGER AS '
DECLARE
  pshipitemid		ALIAS FOR $1;
  _itemlocSeries	INTEGER			 := $2;
  _timestamp		TIMESTAMP WITH TIME ZONE := $3;
  _invhistid		INTEGER;
  _orderitemid		INTEGER;
  _ordertype		TEXT;
  _qty			NUMERIC;
  _ils                 INTEGER;
  _r                   RECORD;
  _m                   RECORD;
  _itemlocdistid       INTEGER;
  _value               NUMERIC;

BEGIN
  SELECT DISTINCT shiphead_order_type, shipitem_orderitem_id, shipitem_qty, shipitem_invhist_id, shipitem_value INTO
		_ordertype, _orderitemid, _qty, _invhistid, _value
  FROM shiphead, shipitem
  WHERE ((shiphead_id=shipitem_shiphead_id)
    AND  (shipitem_id=pshipitemid));

  IF (_itemlocSeries = 0) THEN
    _itemlocSeries := NEXTVAL(''itemloc_series_seq'');
  END IF;
  
  --  If no inventory history, the transaction predates new lot/serial return functionality and have
  --  to handle the old way
  IF (_invhistid IS NULL)  THEN
    _ils := _itemlocSeries;
  ELSE
    _ils := 0;
  END IF;

  IF (_ordertype = ''SO'') THEN

    IF (SELECT (item_type != ''J'')
        FROM coitem, itemsite, item
        WHERE ((coitem_id=_orderitemid)
          AND (coitem_itemsite_id=itemsite_id)
          AND (itemsite_item_id=item_id))) THEN
      SELECT postInvTrans( itemsite_id, ''RS'', _qty * coitem_qty_invuomratio,
			 ''S/R'', _ordertype, formatSoNumber(coitem_id), shiphead_number,
			 ''Return from Shipping'',
			 costcat_asset_accnt_id, costcat_shipasset_accnt_id,
			 _ils, _timestamp, _value ) INTO _invhistid
      FROM coitem, itemsite, costcat, shiphead, shipitem
      WHERE ((_orderitemid=coitem_id)
      AND  (coitem_itemsite_id=itemsite_id)
      AND  (itemsite_costcat_id=costcat_id)
      AND  (shiphead_id=shipitem_shiphead_id)
      AND  (shipitem_id=pshipitemid));
      
    ELSE
      SELECT insertGLTransaction( ''S/R'', ''RS'', formatSoNumber(_orderitemid), ''Return from Shipping'',
                                     costcat_shipasset_accnt_id,
				     costcat_wip_accnt_id,
                                     -1, _value, current_date ) INTO _invhistid
      FROM coitem, itemsite, costcat
      WHERE ( (coitem_itemsite_id=itemsite_id)
       AND (itemsite_costcat_id=costcat_id)
       AND (coitem_id=pshipitemid) )
      GROUP BY costcat_shipasset_accnt_id,costcat_wip_accnt_id;
      
   --  Reverse Backflush eligble material
      FOR _m IN SELECT womatl_id, womatl_qtyper, womatl_scrap, womatl_qtywipscrap,
		     womatl_qtyreq - roundQty(item_fractional, womatl_qtyper * wo_qtyord) AS preAlloc
	      FROM womatl, wo, itemsite, item, shipitem
	      WHERE ((womatl_issuemethod = ''L'')
		AND  (womatl_wo_id=wo_id)
		AND  (womatl_itemsite_id=itemsite_id)
		AND  (itemsite_item_id=item_id)
		AND  (wo_ordtype = ''S'')
		AND  (wo_ordid = shipitem_orderitem_id)
		AND  (shipitem_id=pshipitemid))
      LOOP
        -- CASE says: don''t use scrap % if someone already entered actual scrap
        SELECT returnWoMaterial(_m.womatl_id,
	  CASE WHEN _m.womatl_qtywipscrap > _m.preAlloc THEN
	    _qty * _m.womatl_qtyper + (_m.womatl_qtywipscrap - _m.preAlloc)
	  ELSE
	    _qty * _m.womatl_qtyper * (1 + _m.womatl_scrap)
	  END, _itemlocSeries, now()) INTO _itemlocSeries;

        UPDATE womatl
        SET womatl_issuemethod=''L''
        WHERE ( (womatl_issuemethod=''M'')
          AND (womatl_id=_m.womatl_id) );
    
      END LOOP;


  --  Update the work order about what happened
      UPDATE wo SET 
        wo_qtyrcv = wo_qtyrcv - _qty * coitem_qty_invuomratio,
        wo_wipvalue = wo_wipvalue + _value,
        wo_status =''I''
      FROM coitem
      WHERE ((wo_ordtype = ''S'')
      AND (wo_ordid = _orderitemid)
      AND (coitem_id = _orderitemid));
    END IF;

  ELSEIF (_ordertype = ''TO'') THEN
    SELECT postInvTrans( itemsite_id, ''RS'', _qty,
			''S/R'', _ordertype, tohead_number, '''',
			''Return from Shipping'',
			costcat_asset_accnt_id, costcat_shipasset_accnt_id,
			_itemlocSeries, _timestamp, _value ) INTO _invhistid
    FROM toitem, tohead, itemsite, costcat
    WHERE ((toitem_item_id=itemsite_item_id)
      AND  (toitem_tohead_id=tohead_id)
      AND  (tohead_src_warehous_id=itemsite_warehous_id)
      AND  (itemsite_costcat_id=costcat_id)
      AND  (toitem_id=_orderitemid) );

  END IF;

  --  Find out if there is location or lot/serial detail to undo and handle it 
  FOR _r IN 
    SELECT  itemsite_id, invdetail_ls_id, (invdetail_qty * -1) AS invdetail_qty,
      invdetail_location_id, invdetail_expiration,
      (itemsite_controlmethod IN (''L'', ''S'')) AS lotserial,
      (itemsite_loccntrl) AS loccntrl
    FROM shipitem, invdetail, invhist, itemsite
    WHERE ( (shipitem_invhist_id=invhist_id)
      AND  (invhist_id=invdetail_invhist_id)
      AND  (invhist_itemsite_id=itemsite_id)
      AND  (shipitem_id=pshipitemid) )
  LOOP
    _itemlocdistid := nextval(''itemlocdist_itemlocdist_id_seq'');
          
    IF (( _r.lotserial) AND (NOT _r.loccntrl))  THEN          
      INSERT INTO itemlocdist
        ( itemlocdist_id, itemlocdist_source_type, itemlocdist_source_id,
          itemlocdist_itemsite_id, itemlocdist_ls_id, itemlocdist_expiration,
          itemlocdist_qty, itemlocdist_series, itemlocdist_invhist_id ) 
        VALUES (_itemlocdistid, ''L'', -1,
                _r.itemsite_id, _r.invdetail_ls_id,  COALESCE(_r.invdetail_expiration,startoftime()),
                _r.invdetail_qty, _itemlocSeries, _invhistid );

      INSERT INTO lsdetail 
        ( lsdetail_itemsite_id, lsdetail_ls_id, lsdetail_created,
          lsdetail_source_type, lsdetail_source_id, lsdetail_source_number ) 
        VALUES ( _r.itemsite_id, _r.invdetail_ls_id, CURRENT_TIMESTAMP,
                 ''I'', _itemlocdistid, '''');

      PERFORM distributeitemlocseries(_itemlocSeries);
    ELSE
      INSERT INTO itemlocdist
        ( itemlocdist_id, itemlocdist_source_type, itemlocdist_source_id,
          itemlocdist_itemsite_id, itemlocdist_ls_id, itemlocdist_expiration,
          itemlocdist_qty, itemlocdist_series, itemlocdist_invhist_id ) 
      VALUES (_itemlocdistid, ''O'', -1,
              _r.itemsite_id, _r.invdetail_ls_id, COALESCE(_r.invdetail_expiration,startoftime()),
              _r.invdetail_qty, _itemlocSeries, _invhistid );
 
      INSERT INTO itemlocdist
        ( itemlocdist_itemlocdist_id, itemlocdist_source_type, itemlocdist_source_id,
          itemlocdist_itemsite_id, itemlocdist_ls_id, itemlocdist_expiration,
          itemlocdist_qty) 
      VALUES (_itemlocdistid, ''L'', _r.invdetail_location_id,
              _r.itemsite_id, _r.invdetail_ls_id, COALESCE(_r.invdetail_expiration,startoftime()),
              _r.invdetail_qty);

      PERFORM distributetolocations(_itemlocdistid);
    END IF;
  END LOOP;

  UPDATE shiphead
  SET shiphead_sfstatus=''D''
  FROM shipitem
  WHERE ( (shipitem_shiphead_id=shiphead_id)
   AND (shiphead_sfstatus=''P'')
   AND (shipitem_id=pshipitemid) );

  DELETE FROM shipitem
  WHERE (shipitem_id=pshipitemid);

  RETURN _ils;

END;
' LANGUAGE 'plpgsql';
