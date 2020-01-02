/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2006 - 2007, 2012 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "kptitemmodelbase.h"

#include "kptdebug.h"

#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QModelIndex>
#include <QItemSelection>
#include <QStyleOptionViewItem>
#include <QTimeEdit>
#include <QPainter>
#include <QToolTip>
#include <QTreeView>
#include <QStylePainter>
#include <QMimeData>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTableFormat>
#include <QVector>
#include <QTextLength>
#include <QTextTable>

#include <QBuffer>
#include <KoStore.h>
#include <KoXmlWriter.h>
#include <KoOdfWriteStore.h>

#include <kcombobox.h>
#include <klineedit.h>


using namespace KPlato;

DurationSpinBoxDelegate::DurationSpinBoxDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *DurationSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    DurationSpinBox *editor = new DurationSpinBox(parent);
    editor->installEventFilter(const_cast<DurationSpinBoxDelegate*>(this));
    return editor;
}

void DurationSpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    DurationSpinBox *dsb = static_cast<DurationSpinBox*>(editor);
    //    dsb->setScales(index.model()->data(index, Role::DurationScales));
    dsb->setMinimumUnit((Duration::Unit)(index.data(Role::Minimum).toInt()));
    dsb->setMaximumUnit((Duration::Unit)(index.data(Role::Maximum).toInt()));
    dsb->setUnit((Duration::Unit)(index.model()->data(index, Role::DurationUnit).toInt()));
    dsb->setValue(index.model()->data(index, Qt::EditRole).toDouble());
}

void DurationSpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                           const QModelIndex &index) const
{
    DurationSpinBox *dsb = static_cast<DurationSpinBox*>(editor);
    QVariantList lst;
    lst << QVariant(dsb->value()) << QVariant((int)(dsb->unit()));
    model->setData(index, QVariant(lst), Qt::EditRole);
}

void DurationSpinBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    //r.setHeight(r.height() + 50);
    editor->setGeometry(r);
}
