/*  -*- C++ -*-
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1997 Tim D. Gilman (tdgilman@best.org)
              SPDX-FileCopyrightText: 1998-2001 Mirko Boehm (mirko@kde.org)
              SPDX-FileCopyrightText: 2007 John Layt <john@layt.net>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KP_KDATETABLE_H
#define KP_KDATETABLE_H

#include "planmodels_export.h"

#include <QWidget>
#include <QDate>
#include <QList>
#include <QStyleOptionViewItem>
#include <QStyleOptionHeader>

class QMenu;

namespace KPlato
{

class KDateTableDataModel;
class KDateTableDateDelegate;
class KDateTableWeekDayDelegate;
class KDateTableWeekNumberDelegate;

class StyleOptionHeader;
class StyleOptionViewItem;

/**
 * Date selection table.
 * This is a support class for the KDatePicker class.  It just
 * draws the calendar table without titles, but could theoretically
 * be used as a standalone.
 *
 * When a date is selected by the user, it emits a signal:
 * dateSelected(QDate)
 *
 * @internal
 * @author Tim Gilman, Mirko Boehm
 */
class PLANMODELS_EXPORT KDateTable : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QDate date READ date WRITE setDate) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(bool popupMenu READ popupMenuEnabled WRITE setPopupMenuEnabled) // clazy:exclude=qproperty-without-notify

public:
    /**
     * The constructor.
     */
    explicit KDateTable(QWidget* parent = nullptr);

    /**
     * The constructor.
     */
    explicit KDateTable(const QDate&, QWidget* parent = nullptr);

    /**
     * The destructor.
     */
    ~KDateTable() override;

    /**
     * Returns a recommended size for the widget.
     * To save some time, the size of the largest used cell content is
     * calculated in each paintCell() call, since all calculations have
     * to be done there anyway. The size is stored in maxCell. The
     * sizeHint() simply returns a multiple of maxCell.
     */
    QSize sizeHint() const override;
    /**
     * Set the font size of the date table.
     */
    void setFontSize(int size);
    /**
     * Select and display this date.
     */
    bool setDate(const QDate &date);

    // KDE5 remove the const & from the returned QDate
    /**
     * @returns the selected date.
     */
    const QDate& date() const;

    /**
     * Enables a popup menu when right clicking on a date.
     *
     * When it's enabled, this object emits a aboutToShowContextMenu signal
     * where you can fill in the menu items.
     */
    void setPopupMenuEnabled(bool enable);

    /**
     * Returns if the popup menu is enabled or not
     */
    bool popupMenuEnabled() const;

    enum BackgroundMode { NoBgMode=0, RectangleMode, CircleMode };

    /**
     * Makes a given date be painted with a given foregroundColor, and background
     * (a rectangle, or a circle/ellipse) in a given color.
     */
    void setCustomDatePainting(const QDate &date, const QColor &fgColor,
                                BackgroundMode bgMode=NoBgMode, const QColor &bgColor = QColor());

    /**
     * Unsets the custom painting of a date so that the date is painted as usual.
     */
    void unsetCustomDatePainting(const QDate &date);

    //----->
    enum ItemDataRole { DisplayRole_1 = Qt::UserRole + 1 };
    
    enum SelectionMode { SingleSelection, ExtendedSelection };
    void setSelectionMode(SelectionMode mode);
    
    void setModel(KDateTableDataModel *model);
    KDateTableDataModel *model() const;
    
    // datetable takes ownership of delegate
    void setDateDelegate(KDateTableDateDelegate *delegate);
    // datetable takes ownership of delegate
    void setDateDelegate(const QDate &date, KDateTableDateDelegate *delegate);
    // datetable takes ownership of delegate
    void setWeekDayDelegate(KDateTableWeekDayDelegate *delegate);
    // datetable takes ownership of delegate
    void setWeekNumberDelegate(KDateTableWeekNumberDelegate *delegate);
    void setWeekNumbersEnabled(bool enable);
    
    void setStyleOptionDate(const StyleOptionViewItem &so);
    void setStyleOptionWeekDay(const StyleOptionHeader &so);
    void setStyleOptionWeekNumber(const StyleOptionHeader &so);
    
    void setGridEnabled(bool enable);
    //<-----
    
protected:
    /**
     * calculate the position of the cell in the matrix for the given date.
     * The result is the 0-based index.
     */
    virtual int posFromDate(const QDate &date); 
    /**
     * calculate the date that is displayed at a given cell in the matrix. pos is the
     * 0-based index in the matrix. Inverse function to posForDate().
     */
    virtual QDate dateFromPos(int pos); 

    void paintEvent(QPaintEvent *e) override;
    /**
     * React on mouse clicks that select a date.
     */
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

    /**
     * Cell highlight on mouse hovering
     */
    bool event(QEvent *e) override;

Q_SIGNALS:
    /**
     * The selected date changed.
     * @param cur The current date
     * @param old The date before the date was changed
     */
    void dateChanged(const QDate& cur, const QDate& old = QDate());
    /**
     * A date has been selected by clicking on the table.
     */
    void tableClicked();

    /**
     * A popup menu for selected dates is about to be shown.
     * Connect the slot where you fill the menu to this signal.
     */
    void aboutToShowContextMenu(QMenu * menu, const QList<QDate>&);

    //----->
    void selectionChanged(const QList<QDate>&);

protected Q_SLOTS:
    void slotReset();
    void slotDataChanged(const QDate &start, const QDate &end);
    //<------
    
private:
    Q_PRIVATE_SLOT(d, void nextMonth())
    Q_PRIVATE_SLOT(d, void previousMonth())
    Q_PRIVATE_SLOT(d, void beginningOfMonth())
    Q_PRIVATE_SLOT(d, void endOfMonth())
    Q_PRIVATE_SLOT(d, void beginningOfWeek())
    Q_PRIVATE_SLOT(d, void endOfWeek())

private:
    class KDateTablePrivate;
    friend class KDateTablePrivate;
    KDateTablePrivate * const d;

    void initWidget(const QDate &date);
    void initAccels();
    void paintCell(QPainter *painter, int row, int col);
    
    Q_DISABLE_COPY(KDateTable)
};

//----->
class PLANMODELS_EXPORT KDateTableDataModel : public QObject
{
    Q_OBJECT
public:
    KDateTableDataModel(QObject *parent);
    ~KDateTableDataModel() override;
    
    /// Fetch data for @p date, @p dataType specifies the type of data
    virtual QVariant data(const QDate &date, int role = Qt::DisplayRole,  int dataType = -1) const;
    virtual QVariant weekDayData(int day, int role = Qt::DisplayRole) const;
    virtual QVariant weekNumberData(int week, int role = Qt::DisplayRole) const;
    
Q_SIGNALS:
    void reset();
    void dataChanged(const QDate &start, const QDate &end);

};

//-------
class PLANMODELS_EXPORT KDateTableDateDelegate : public QObject
{
    Q_OBJECT
public:
    KDateTableDateDelegate(QObject *parent = nullptr);
    ~KDateTableDateDelegate() override {}

    virtual QRectF paint(QPainter *painter, const StyleOptionViewItem &option, const QDate &date,  KDateTableDataModel *model);
    
    virtual QVariant data(const QDate &date, int role, KDateTableDataModel *model);
};

class PLANMODELS_EXPORT KDateTableCustomDateDelegate : public KDateTableDateDelegate
{
    Q_OBJECT
public:
    KDateTableCustomDateDelegate(QObject *parent = nullptr);
    ~KDateTableCustomDateDelegate() override {}

    QRectF paint(QPainter *painter, const StyleOptionViewItem &option, const QDate &date,  KDateTableDataModel *model) override;
    
private:
    friend class KDateTable;
    QColor fgColor;
    QColor bgColor;
    KDateTable::BackgroundMode bgMode;
    
};

class PLANMODELS_EXPORT KDateTableWeekDayDelegate : public QObject
{
    Q_OBJECT
public:
    KDateTableWeekDayDelegate(QObject *parent = nullptr);
    ~KDateTableWeekDayDelegate() override {}

    virtual QRectF paint(QPainter *painter, const StyleOptionHeader &option, int weekday,  KDateTableDataModel *model);

    virtual QVariant data(int day, int role, KDateTableDataModel *model);
};

class PLANMODELS_EXPORT KDateTableWeekNumberDelegate : public QObject
{
    Q_OBJECT
public:
    KDateTableWeekNumberDelegate(QObject *parent = nullptr);
    ~KDateTableWeekNumberDelegate() override {}

    virtual QRectF paint(QPainter *painter, const StyleOptionHeader &option, int week,  KDateTableDataModel *model);
    
    virtual QVariant data(int week, int role, KDateTableDataModel *model);
};

class StyleOptionHeader : public QStyleOptionHeader
{
public:
    StyleOptionHeader() : QStyleOptionHeader() {}

    StyleOptionHeader &operator=(const StyleOptionHeader &) = default;

    QRectF rectF;
};

class StyleOptionViewItem : public QStyleOptionViewItem
{
public:
    StyleOptionViewItem() 
    : QStyleOptionViewItem()
    {}
    StyleOptionViewItem(const StyleOptionViewItem &style) 
    : QStyleOptionViewItem(style)
    {
        rectF = style.rectF;
    }

    StyleOptionViewItem &operator=(const StyleOptionViewItem &) = default;

    QRectF rectF;
};

} //namespace KPlato

#endif // KP_KDATETABLE_H
