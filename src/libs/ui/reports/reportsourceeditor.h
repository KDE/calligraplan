/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATO_REPORTSOURCEEDITORTOR_H
#define KPLATO_REPORTSOURCEEDITORTOR_H


#include "ui_reportsourceeditor.h"

#include <QWidget>

class QAbstractItemModel;
class QDomElement;

namespace KPlato
{

class ReportSourceEditor : public QWidget, public Ui::ReportSourceEditor
{
    Q_OBJECT
public:
    explicit ReportSourceEditor(QWidget *parent);

    void setModel(QAbstractItemModel *model);

    void setSourceData(const QDomElement &element);
    void sourceData(QDomElement &element) const;
    /// Return the tag of the selected model
    QString selectFromTag() const;

Q_SIGNALS:
    void selectFromChanged(const QString &tag);

private Q_SLOTS:
    void slotCurrentIndexChanged();
};

}

#endif

