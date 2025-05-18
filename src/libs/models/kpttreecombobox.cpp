/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttreecombobox.h"

#include <KLocalizedString>

#include <QModelIndex>
#include <QTreeView>
#include <QHeaderView>
#include <QStylePainter>

namespace KPlato
{


//----------------------
TreeComboBox::TreeComboBox(QWidget *parent)
    : KComboBox(parent),
    m_selectionmode(QAbstractItemView::ExtendedSelection)
{
    m_showcolumns << 0;
    m_showheader = false;

    updateView();

    connect(this, SIGNAL(activated(int)), SLOT(slotSelectionChanged()));
}

void TreeComboBox::updateView()
{
    QTreeView *v = new QTreeView();
    setView(v);
    v->setSelectionMode(m_selectionmode);
    // don't want to have mouseover select an item
    v->disconnect(SIGNAL(entered(QModelIndex)));

    QHeaderView *h = v->header();
    for (int i = 0; i < h->count(); ++i) {
        h->setSectionHidden(i, ! m_showcolumns.contains(i));
    }
    h->setVisible(m_showheader);
    v->setRootIsDecorated(false);
}

QTreeView *TreeComboBox::view() const
{
    return static_cast<QTreeView*>(KComboBox::view());
}

void TreeComboBox::setModel(QAbstractItemModel *model)
{
    KComboBox::setModel(model);
    updateView();
}

QAbstractItemModel *TreeComboBox::model() const
{
    return KComboBox::model();
}

void TreeComboBox::setSelectionMode(QAbstractItemView::SelectionMode mode)
{
    m_selectionmode = mode;
    view()->setSelectionMode(mode);
}

void TreeComboBox::slotSelectionChanged()
{
    updateCurrentIndexes(view()->selectionModel()->selectedRows());
}

void TreeComboBox::showPopup()
{
    QComboBox::showPopup();
    // now clean up things we want different
    QItemSelectionModel *sm = view()->selectionModel();
    sm->clearSelection();
    view()->setSelectionMode(m_selectionmode);
    view()->setSelectionBehavior(QAbstractItemView::SelectRows);
    for (const QModelIndex i : std::as_const(m_currentIndexes)) {
        if (i.isValid()) {
            sm->select(i, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
    if (! sm->selectedRows().contains(sm->currentIndex())) {
        sm->setCurrentIndex(sm->selectedRows().value(0), QItemSelectionModel::NoUpdate);
    }
}

void TreeComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    QStringList lst;
    for (const QPersistentModelIndex &idx : std::as_const(m_currentIndexes)) {
        if (idx.isValid()) {
            lst << idx.data().toString();
        }
    }
    opt.currentText = lst.isEmpty() ? i18n("None") : lst.join(QStringLiteral(","));
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

void TreeComboBox::setCurrentIndexes(const QModelIndexList &lst)
{
    m_currentIndexes.clear();
    for (const QModelIndex &idx : lst) {
        m_currentIndexes << QPersistentModelIndex(idx);
    }
}

void TreeComboBox::setCurrentIndexes(const QList<QPersistentModelIndex> &lst)
{
    m_currentIndexes = lst;
}

void TreeComboBox::updateCurrentIndexes(const QModelIndexList &lst)
{
    QList<QPersistentModelIndex> x;
    for (const QModelIndex &idx : lst) {
        x << QPersistentModelIndex(idx);
    }
    if (x == m_currentIndexes) {
        return;
    }
    m_currentIndexes = x;
    Q_EMIT changed();
}

} //namespace KPlato
