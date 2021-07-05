/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <calligra-devel@kde.org>
 * SPDX-FileCopyrightText: 2006-2009 Dag Andersen <calligra-devel@kde.org>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef REQUIEREDRESOURCEDELEGATE_H
#define REQUIEREDRESOURCEDELEGATE_H

#include "planmodels_export.h"

#include "kptitemmodelbase.h"

class KUndo2Command;


/// The main namespace
namespace KPlato
{

//------------------------------------
class PLANMODELS_EXPORT RequieredResourceDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit RequieredResourceDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

} // namespace KPlato

#endif
