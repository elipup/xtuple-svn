
function query()
{
  var params = new Object;
  params.search = mywindow.findChild("_search").text;

  var qry = toolbox.executeQuery("SELECT cntct_first_name, cntct_last_name, cntct_phone"
                                +"  FROM cntct"
                                +" WHERE((cntct_phone = <? value(\"search\") ?>)"
                                +"    OR (cntct_phone2 = <? value(\"search\") ?>))", params);
  if(!qry.first())
  {
    mywindow.findChild("_firstname").text = qsTr("n/a");
    mywindow.findChild("_lastname").text = qsTr("n/a");
    mywindow.findChild("_number").text = qsTr("n/a");

    toolbox.messageBox("warning", mywindow, qsTr("No Results"), qsTr("No results were found matching your criteria."));
    return;
  }

  mywindow.findChild("_firstname").text = qry.value("cntct_first_name");
  mywindow.findChild("_lastname").text = qry.value("cntct_last_name");
  mywindow.findChild("_number").text = qry.value("cntct_phone");
}

mywindow.findChild("_close").clicked.connect(mywindow, "close");
mywindow.findChild("_query").clicked.connect(query);
