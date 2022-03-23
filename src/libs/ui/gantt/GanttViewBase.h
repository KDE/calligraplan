/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
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

//---------------------------------------
class GanttChartDisplayOptionsPanel : public QWidget, public Ui::GanttChartDisplayOptions
{
    Q_OBJECT
public:
    explicit GanttChartDisplayOptionsPanel(GanttViewBase *gantt, GanttItemDelegate *delegate, QWidget *parent = 0);

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
    GanttPrintingOptions(const GanttPrintingOptions &other);
    GanttPrintingOptions &operator=(const GanttPrintingOptions &other);

    bool loadContext(const KoXmlElement &settings);
    void saveContext(QDomElement &settings) const;

    KGantt::PrintingContext context;
    bool useStartTime;
    QDateTime diagramStart;
    bool useEndTime;
    QDateTime diagramEnd;
};

class PLANUI_EXPORT GanttPrintingOptionsWidget : public QWidget, public Ui::GanttPrintingOptionsWidget
{
    Q_OBJECT
public:
    explicit GanttPrintingOptionsWidget(GanttViewBase *gantt, QWidget *parent = nullptr);

    GanttPrintingOptions options() const;

    void setPrintRowLabels(bool value) { ui_printRowLabels->setChecked(value); }
    bool printRowLabels() const { return ui_printRowLabels->isChecked(); }

    void setPrintColumnLabels(bool value) { ui_printColumnLabels->setChecked(value); }
    bool printColumnLabels() const { return ui_printColumnLabels->isChecked(); }

    void setFitting(const KGantt::PrintingContext::Fitting &fitting)  {
        if (fitting & KGantt::PrintingContext::FitSinglePage) {
            ui_fitSingle->setChecked(true);
        } else if (fitting & KGantt::PrintingContext::FitPageHeight) {
            ui_fitVertical->setChecked(true);
        } else {
            ui_noFitting->setChecked(true);
        }
    }
    KGantt::PrintingContext::Fitting fitting() const {
        KGantt::PrintingContext::Fitting s;
        if (ui_fitSingle->isChecked()) {
            s = KGantt::PrintingContext::FitSinglePage;
        } else if (ui_fitVertical->isChecked()) {
            s = KGantt::PrintingContext::FitPageHeight;
        } else {
            s = KGantt::PrintingContext::NoFitting;
        }
        return s;
    }

public Q_SLOTS:
    void setOptions(const KPlato::GanttPrintingOptions &opt);
};

class GanttPrintingDialog : public PrintingDialog
{
    Q_OBJECT
public:
    PLANUI_EXPORT GanttPrintingDialog(ViewBase *view, GanttViewBase *gantt);

    void startPrinting(RemovePolicy removePolicy) override;
    QList<QWidget*> createOptionWidgets() const override;
    void printPage(int page, QPainter &painter) override;

    int documentLastPage() const override;
    qreal rowLabelsWidth(const QPaintDevice *device) const;
    QRectF calcSceneRect(const QDateTime &startDateTime, const QDateTime &endDateTime) const;

    int lastPageNumber() const;

protected Q_SLOTS:
    void slotPrintRowLabelsToogled(bool on);
    void slotSinglePageToogled(bool on);

protected:
    GanttViewBase *m_gantt;
    GanttPrintingOptionsWidget *m_options;
    QRectF m_sceneRect;
    int m_horPages;
    int m_vertPages;
    qreal m_headerHeight;
    qreal m_rowLabelsWidth;
};

class PLANUI_EXPORT GanttTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit GanttTreeView(QWidget *parent);

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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    friend class GanttPrintingDialog;
    GanttPrintingOptions m_printOptions;

private:
    QPoint m_dragStartPosition;
    Qt::MouseButton m_mouseButton;
};

}  //KPlato namespace

#endif
