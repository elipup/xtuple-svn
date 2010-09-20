include("teglobal");
te.customer = new Object;


if (privileges.check("CanViewRates"))
{
  var _creditGroup = mywindow.findChild("_creditGroup");
  var _layout = toolbox.widgetGetLayout(_creditGroup);
  var _tecustomer = toolbox.loadUi("tecustomer", mywindow);
  var groupBox_3 = mywindow.findChild("groupBox_3");
  var groupBox_4 = mywindow.findChild("groupBox_4");
  var _creditStatusGroup = mywindow.findChild("_creditStatusGroup");

  _layout.addWidget(_tecustomer, 0, 0, 1, 3);
  _layout.addWidget(groupBox_4, 1, 0, 1, 1);
  _layout.addWidget(_creditStatusGroup, 1, 1, 1, 1);
  _layout.addWidget(groupBox_3, 1, 2, 1, 1);
  _layout.addWidget(_creditGroup, 2, 0, 1, 3);

  var _number = mywindow.findChild("_number");
  var _blanketPos = mywindow.findChild("_blanketPos");
  var _billingGroup = mywindow.findChild("_billingGroup");
  var _rate = _tecustomer.findChild("_rate");
  var _tecustrateid = -1; 
  var _basecurrid = _rate.id();

  te.customer.save = function(custId)
  {
    var params = new Object();
    params.tecustrate_id	= _tecustrateid;
    params.cust_id  	= custId;
    params.curr_id	= _rate.id();
    params.rate	= _rate.localValue;

    var query = "updtecustrate";
    if (!_billingGroup.checked)
      query = "deltecustrate";
    else if (_tecustrateid == -1)
      query = "instecustrate";

    var q = toolbox.executeDbQuery("customer", query, params);
    if (q.first())
      _tecustrateid = q.value("tecustrate_id");
    else
      te.errorCheck(q);
  }

  te.customer.populate = function()
  {
    var params = new Object();
    params.cust_id = _number.id();    

    var q = toolbox.executeDbQuery("customer", "seltecustrate", params);

    if (q.first())
    {
      _billingGroup.checked = true;
      _tecustrateid = q.value("tecustrate_id");
      _rate.setId(q.value("tecustrate_curr_id"));
      _rate.localValue = q.value("tecustrate_rate");
      return;
    }
    else
      te.errorCheck(q);
  
    _billingGroup.checked = false;
    _tecustrateid = -1;
    _rate.setId(_basecurrid);
    _rate.localValue = 0;
  }

  // Initialize
  QWidget.setTabOrder(_blanketPos, _rate);

  // Connections
  _number.newId.connect(te.customer.populate);
  mywindow["saved(int)"].connect(te.customer.save);
}
