/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <calligra-devel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASKCOMPLETEDELEGATE_H
#define KPTTASKCOMPLETEDELEGATE_H

#include "kptitemmodelbase.h"

class QModelIndex;

namespace KPlato
{

class PLANMODELS_EXPORT TaskCompleteDelegate : public ProgressBarDelegate
{
  Q_OBJECT
public:
    explicit TaskCompleteDelegate(QObject *parent = nullptr);

    ~TaskCompleteDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

};

} //namespace KPlato

#endif
