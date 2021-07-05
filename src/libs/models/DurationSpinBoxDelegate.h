/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <calligra-devel@kde.org>
 * SPDX-FileCopyrightText: 2006-2009 Dag Andersen <calligra-devel@kde.org>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTITEMMODELBASE_H
#define KPTITEMMODELBASE_H

#include "planmodels_export.h"

#include "kptglobal.h"
#include "kpttreecombobox.h"

#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QMetaEnum>
#include <QMimeData>
#include <QSortFilterProxyModel>

#include <KoXmlReaderForward.h>

class KUndo2Command;


/// The main namespace
namespace KPlato
{

class PLANMODELS_EXPORT DurationSpinBoxDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit DurationSpinBoxDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

} // namespace KPlato

#endif
