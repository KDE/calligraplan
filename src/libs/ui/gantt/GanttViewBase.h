/* This file is part of the KDE project
 *  Copyright (C) 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KPTGANTTVIEWBASE_H
#define KPTGANTTVIEWBASE_H

#include "planui_export.h"

#include "kptitemviewsettup.h"
#include "kptviewbase.h"

#include "ui_kptganttprintingoptions.h"
#include "ui_kptganttchartdisplayoptions.h"

#include <KGanttGlobal>
#include <KGanttView>

class QDomElement;

class KoPrintJob;
class KoXmlElement;

namespace KGantt {
    class DateTimeGrid;
}

namespace KPlato
{
class ViewBase;
class DateTimeTimeLine;
class GanttViewBase;
class GanttItemDelegate;
class GanttPrintingOptionsWidget;

class GanttChartDisplayOptionsPanel : public QWidget, public Ui::GanttChartDisplayOptions
{
    Q_OBJECT
public:
    explicit GanttChartDisplayOptionsPanel(GanttViewBase *gantt, GanttItemDelegate *delegate, QWidget *parent = nullptr);

    void setValues(const GanttItemDelegate &del);

public Q_SLOTS:
    void slotOk();
    void setDefault();

Q_SIGNALS:
    void changed();

private:
    GanttItemDelegate *m_delegate;
    GanttViewBase *m_gantt;
};

class GanttViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    explicit GanttViewSettingsDialog(GanttViewBase *gantt, GanttItemDelegate *delegate, ViewBase *view, bool selectPrint = false);

protected Q_SLOTS:
    void slotOk() override;

private:
    GanttViewBase *m_gantt;
    GanttPrintingOptionsWidget *m_printingoptions;
};

//--------------------
class GanttPrintingOptions
{
public:
    GanttPrintingOptions();

    bool loadContext(const KoXmlElement &settings);
    void saveContext(QDomElement &settings) const;

    bool printRowLabels;
    bool singlePage;
};

class PLANUI_EXPORT GanttPrintingOptionsWidget : public QWidget, public Ui::GanttPrintingOptionsWidget
{
    Q_OBJECT
public:
    explicit GanttPrintingOptionsWidget(QWidget *parent = nullptr);

    GanttPrintingOptions options() const;

    void setPrintRowLabels(bool value) { ui_printRowLabels->setChecked(value); }
    bool printRowLabels() const { return ui_printRowLabels->isChecked(); }

    void setSinglePage(bool value)  { value ? ui_singlePage->setChecked(false) : ui_multiplePages->setChecked(true); }
    bool singlePage() const { return ui_singlePage->isChecked(); }

public Q_SLOTS:
    void setOptions(const KPlato::GanttPrintingOptions &opt);
};

class GanttPrintingDialog : public PrintingDialog
{
    Q_OBJECT
public:
    GanttPrintingDialog(ViewBase *view, GanttViewBase *gantt);

    void startPrinting(RemovePolicy removePolicy) override;
    QList<QWidget*> createOptionWidgets() const override;
    void printPage(int page, QPainter &painter) override;

    int documentLastPage() const override;

protected Q_SLOTS:
    void slotPrintRowLabelsToogled(bool on);
    void slotSinglePageToogled(bool on);

protected:
    GanttViewBase *m_gantt;
    QRectF m_sceneRect;
    int m_horPages;
    int m_vertPages;
    double m_headerHeight;
    GanttPrintingOptionsWidget *m_options;
    QImage m_image;
};

class PLANUI_EXPORT GanttTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit GanttTreeView(QWidget *parent);

};

class GanttZoomWidget : public QSlider {
    Q_OBJECT
public:
    explicit GanttZoomWidget(QWidget *parent);

    void setGrid(KGantt::DateTimeGrid *grid);

    void setEnableHideOnLeave(bool hide);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void sliderValueChanged(int value);

private:
    bool m_hide;
    KGantt::DateTimeGrid *m_grid;
};

class PLANUI_EXPORT GanttViewBase : public KGantt::View
{
    Q_OBJECT
public:
    explicit GanttViewBase(QWidget *parent);
    ~GanttViewBase() override;

    GanttTreeView *treeView() const;
    GanttPrintingOptions printingOptions() const { return m_printOptions; }

    virtual bool loadContext(const KoXmlElement &settings);
    virtual void saveContext(QDomElement &settings) const;

    DateTimeTimeLine *timeLine() const;

    void editCopy();

    void handleContextMenuEvent(const QModelIndex &idx, const QPoint &pos);

    void setShowRowSeparators(bool enable);
    bool showRowSeparators() const;

public Q_SLOTS:
    void setPrintingOptions(const KPlato::GanttPrintingOptions &opt) { m_printOptions = opt; }

Q_SIGNALS:
    void contextMenuRequested(const QModelIndex &idx, const QPoint &pos);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    friend class GanttPrintingDialog;
    GanttPrintingOptions m_printOptions;

private:
    QPoint m_dragStartPosition;
    Qt::MouseButton m_mouseButton;
    GanttZoomWidget *m_zoomwidget;
};

}  //KPlato namespace

#endif
