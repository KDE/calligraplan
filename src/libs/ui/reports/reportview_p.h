/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOREPORTVIEW_P_H
#define KPLATOREPORTVIEW_P_H


#include "planui_export.h"

#include "ui_reportgroupsectionswidget.h"

#include <QSplitter>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QMap>
#include <QAction>

class KReportDesignerSectionDetailGroup;
class KReportDesigner;

class KToolBar;

class QDomElement;
class QActionGroup;

namespace KPlato
{

class ReportData;


class GroupSectionEditor : public QObject
{
    Q_OBJECT
public:
    explicit GroupSectionEditor(QObject *parent);

    void setupUi(QWidget *widget);
    void clear();
    void setData(KReportDesigner *designer, ReportData *rd);

protected Q_SLOTS:
    void slotSelectionChanged(const QItemSelection &sel);
    void slotAddRow();
    void slotRemoveRows();
    void slotMoveRowUp();
    void slotMoveRowDown();

private:
    Ui::ReportGroupSectionsWidget gsw;
    KReportDesigner *designer;
    ReportData *reportdata;
    QStandardItemModel model;

    class Item : public QStandardItem
    {
    public:
        explicit Item(KReportDesignerSectionDetailGroup *g) : QStandardItem(), group(g) {}
        KReportDesignerSectionDetailGroup *group;

        QStringList names;
        QStringList keys;
    };

    class ColumnItem : public Item
    {
    public:
        explicit ColumnItem(KReportDesignerSectionDetailGroup *g);
        QVariant data(int role = Qt::DisplayRole) const;
        void setData(const QVariant &value, int role = Qt::EditRole);
    };
    class SortItem : public Item
    {
    public:
        explicit SortItem(KReportDesignerSectionDetailGroup *g);
        QVariant data(int role = Qt::DisplayRole) const;
        void setData(const QVariant &value, int role = Qt::EditRole);
    };
    class HeaderItem : public Item
    {
    public:
        explicit HeaderItem(KReportDesignerSectionDetailGroup *g);
        QVariant data(int role = Qt::DisplayRole) const;
        void setData(const QVariant &value, int role = Qt::EditRole);
    };
    class FooterItem : public Item
    {
    public:
        explicit FooterItem(KReportDesignerSectionDetailGroup *g);
        QVariant data(int role = Qt::DisplayRole) const;
        void setData(const QVariant &value, int role = Qt::EditRole);
    };
    class PageBreakItem : public Item
    {
    public:
        explicit PageBreakItem(KReportDesignerSectionDetailGroup *g);
        QVariant data(int role = Qt::DisplayRole) const;
        void setData(const QVariant &value, int role = Qt::EditRole);
    };
};


} // namespace KPlato

#endif
