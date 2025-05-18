/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTREECOMBOBOX_H
#define KPTTREECOMBOBOX_H

#include "planmodels_export.h"

#include <KComboBox>

#include <QAbstractItemView>

class QModelIndex;
class QPersistentModelIndex;
class QPaintEvent;
class QTreeView;
class QAbstractItemModel;

namespace KPlato
{

class PLANMODELS_EXPORT TreeComboBox : public KComboBox
{
    Q_OBJECT
public:
    explicit TreeComboBox(QWidget *parent = nullptr);

    QTreeView *view() const;
    void setModel(QAbstractItemModel *model) override;
    QAbstractItemModel *model() const;

    QList<QPersistentModelIndex> currentIndexes() const { return m_currentIndexes; }

    void setSelectionMode(QAbstractItemView::SelectionMode mode);

    void showPopup() override;

Q_SIGNALS:
    void changed();

public Q_SLOTS:
    void setCurrentIndexes(const QModelIndexList &lst);
    void setCurrentIndexes(const QList<QPersistentModelIndex> &lst);

protected:
    void paintEvent(QPaintEvent *event) override;
    
protected Q_SLOTS:
    void updateCurrentIndexes(const QModelIndexList &lst);
    void slotSelectionChanged();

private:
    void updateView();
    
private:
    QAbstractItemView::SelectionMode m_selectionmode;
    QList<int> m_showcolumns;
    bool m_showheader;
    QList<QPersistentModelIndex> m_currentIndexes;
};


} //namespace KPlato

#endif

