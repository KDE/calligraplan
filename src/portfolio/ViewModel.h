/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANPORTFOLIO_VIEWMODEL_H
#define PLANPORTFOLIO_VIEWMODEL_H

#include <KPageModel>
#include <QString>

class MainDocument;

class ViewModel : public KPageModel
{
    Q_OBJECT
public:
    explicit ViewModel(QObject *parent = nullptr);
    ~ViewModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QVariant data(const QModelIndex &idx, int role) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    void setWidgets(const QList<QWidget*> &views);

private:
    QList<QWidget*> m_views;
};

#endif
