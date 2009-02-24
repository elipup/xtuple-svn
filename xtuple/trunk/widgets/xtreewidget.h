/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef __XTREEWIDGET_H__
#define __XTREEWIDGET_H__

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QVector>

#include "widgets.h"


//  Table Column Widths
#define _itemColumn        100
#define _whsColumn         35
#define _userColumn        80
#define _dateColumn        80
#define _timeDateColumn    160
#define _timeColumn        80
#define _qtyColumn         80
#define _priceColumn       60
#define _moneyColumn       60
#define _bigMoneyColumn    100
#define _costColumn        70
#define _prcntColumn       55
#define _transColumn       35
#define _uomColumn         45
#define _orderColumn       60
#define _statusColumn      45
#define _seqColumn         40
#define _ynColumn          45
#define _docTypeColumn     80
#define _currencyColumn    80


#include "xsqlquery.h"

class XTreeWidget;
class QMenu;
class QAction;

class XTUPLEWIDGETS_EXPORT XTreeWidgetItem : public QTreeWidgetItem
{
  friend class XTreeWidget;

  public:
    XTreeWidgetItem(XTreeWidgetItem *, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );
    XTreeWidgetItem(XTreeWidgetItem *, int, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );
    XTreeWidgetItem(XTreeWidget *, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );
    XTreeWidgetItem(XTreeWidget *, int, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );
    XTreeWidgetItem(XTreeWidget *, XTreeWidgetItem *, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );
    XTreeWidgetItem(XTreeWidget *, XTreeWidgetItem *, int, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );
    XTreeWidgetItem(XTreeWidgetItem *, XTreeWidgetItem *, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );
    XTreeWidgetItem(XTreeWidgetItem *, XTreeWidgetItem *, int, int, QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant(),
                     QVariant = QVariant(), QVariant = QVariant() );

    void setText(int, const QVariant &);
    virtual QString text(int p) const { return QTreeWidgetItem::text(p); }
    virtual QString text(const QString &) const;
    inline void setTextColor(int column, const QColor & color) { QTreeWidgetItem::setTextColor(column, color); }
    void setTextColor(const QColor &);

    Q_INVOKABLE inline int id() const         { return _id;    }
    Q_INVOKABLE inline int altId() const      { return _altId; }
    Q_INVOKABLE inline void setId(int pId)    { _id = pId;     }
    Q_INVOKABLE inline void setAltId(int pId) { _altId = pId;  }

    virtual QVariant rawValue(const QString);
    Q_INVOKABLE virtual int id(const QString);

    inline XTreeWidgetItem *child(int idx) const
    {
      QTreeWidgetItem * item = QTreeWidgetItem::child(idx);
      return ((XTreeWidgetItem*)item);
    }    

  protected:
    virtual double totalForItem(const int, const int) const;

  private:
    void constructor( int, int, QVariant, QVariant, QVariant,
                      QVariant, QVariant, QVariant, QVariant,
                      QVariant, QVariant, QVariant, QVariant );

    int                _id;
    int                _altId;
};

class XTUPLEWIDGETS_EXPORT XTreeWidget : public QTreeWidget
{
  Q_OBJECT
  Q_PROPERTY(QString dragString READ dragString WRITE setDragString)
  Q_PROPERTY(QString altDragString READ altDragString WRITE setAltDragString)

  public:
    XTreeWidget(QWidget *);
    ~XTreeWidget();

    Q_INVOKABLE void populate(XSqlQuery, bool = FALSE);
    Q_INVOKABLE void populate(XSqlQuery, int, bool = FALSE);
    void populate(const QString &, bool = FALSE);
    void populate(const QString &, int, bool = FALSE);

    QString dragString() const;
    void setDragString(QString);
    QString altDragString() const;
    void setAltDragString(QString);

    Q_INVOKABLE int  altId() const;
    Q_INVOKABLE int  id()    const;
    Q_INVOKABLE int  id(const QString)     const;
    Q_INVOKABLE void setId(int);

    Q_INVOKABLE virtual int              column(const QString) const;
    Q_INVOKABLE virtual XTreeWidgetItem *currentItem()         const;
    Q_INVOKABLE virtual void             setColumnCount(int);
    Q_INVOKABLE virtual void             setColumnLocked(int, bool);
    Q_INVOKABLE virtual void             setColumnVisible(int, bool);
    Q_INVOKABLE virtual void             sortItems(int, Qt::SortOrder);
    Q_INVOKABLE virtual XTreeWidgetItem *topLevelItem(int idx) const;

    Q_INVOKABLE XTreeWidgetItem *findXTreeWidgetItemWithId(const XTreeWidget *ptree, const int pid);
    Q_INVOKABLE XTreeWidgetItem *findXTreeWidgetItemWithId(const XTreeWidgetItem *ptreeitem, const int pid);

    static  bool itemAsc(const QVariant &, const QVariant &);
    static  bool itemDesc(const QVariant &, const QVariant &);

  public slots:
    void addColumn(const QString &, int, int, bool = true, const QString = QString(), const QString = QString());
    void clear();
    void hideColumn(int colnum) { QTreeWidget::hideColumn(colnum); };
    void hideColumn(const QString &);
    void showColumn(int colnum) { QTreeWidget::showColumn(colnum); };
    void showColumn(const QString &);

  signals:
    void  valid(bool);
    void  newId(int);
    void  itemSelected(int);
    void  populateMenu(QMenu *, QTreeWidgetItem *);
    void  populateMenu(QMenu *, QTreeWidgetItem *, int);
    void  resorted();
    void  populated();

  protected slots:
    void sHeaderClicked(int);
    void populateCalculatedColumns();

  protected:
    QPoint dragStartPosition;

    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void resizeEvent(QResizeEvent*);

  private:
    QString     _dragString;
    QString     _altDragString;
    QMenu       *_menu;
    QMap<int, int>      _defaultColumnWidths;
    QMap<int, int>      _savedColumnWidths;
    QMap<int, bool>     _savedVisibleColumns;
    QMap<int, QVariantMap*>	_roles;
    QList<int>          _lockedColumns;
    QVector<int>        _stretch;
    bool        _resizingInProcess;
    bool        _forgetful;
    bool        _forgetfulOrder;
    bool        _settingsLoaded;
    QString     _settingsName;
    int         _resetWhichWidth;
    int         _scol;
    Qt::SortOrder _sord;
    static void loadLocale();

  private slots:
    void sSelectionChanged();
    void sItemSelected(QTreeWidgetItem *, int);
    void sShowMenu(const QPoint &);
    void sShowHeaderMenu(const QPoint &);
    void sExport();
    //void sStartDrag(QTreeWidgetItem *, int);
    void sColumnSizeChanged(int, int, int);
    void sResetWidth();
    void sResetAllWidths();
    void sToggleForgetfulness();
    void sToggleForgetfulnessOrder();
    void popupMenuActionTriggered(QAction*);
};

#endif

