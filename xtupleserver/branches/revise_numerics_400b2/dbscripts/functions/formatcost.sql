CREATE OR REPLACE FUNCTION formatCost(NUMERIC) RETURNS TEXT AS $$
-- Copyright (c) 1999-2012 by OpenMFG LLC, d/b/a xTuple. 
-- See www.xtuple.com/CPAL for the full text of the software license.
  SELECT formatNumeric($1, 'cost');
$$ LANGUAGE SQL STABLE;

CREATE OR REPLACE FUNCTION formatCost(xcost) RETURNS TEXT AS $$
-- Copyright (c) 1999-2012 by OpenMFG LLC, d/b/a xTuple. 
-- See www.xtuple.com/CPAL for the full text of the software license.
  SELECT formatNumeric($1.amount, 'cost');
$$ LANGUAGE SQL STABLE;
