/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_PROJECTSDASHBOARD_H
#define PLANPORTFOLIO_PROJECTSDASHBOARD_H

#include "planportfolio_export.h"

#include <KoView.h>

#include <QAbstractTableModel>
#include <QDate>

class MainDocument;
class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;

namespace KPlato {
    class ChartItemModel;
}

class PLANPORTFOLIO_EXPORT ProjectsDashboard : public KoView
{
    Q_OBJECT

public:
    explicit ProjectsDashboard(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~ProjectsDashboard() override;

    QMenu *popupMenu(const QString& name);

    KoPrintJob *createPrintJob() override;

protected:
    void updateReadWrite(bool readwrite) override;

private:
    void setupGui();

private:
    bool m_readWrite;

};

class ProjectsPerformanceModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ProjectsPerformanceModel(QObject *parent = nullptr);
    ~ProjectsPerformanceModel() override;

    void setPortfolio(KoDocument *doc);
    void setValueToRetreive(int value);

    int rowCount(const QModelIndex &idx = QModelIndex()) const override;
    int columnCount(const QModelIndex &idx = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role =  Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

public Q_SLOTS:
    void slotChanged();

private:
    MainDocument *m_portfolio;
    int m_valueToRetreive;
    QHash<KoDocument*, KPlato::ChartItemModel*> m_chartModels;
    QDate m_startDate;
    QDate m_endDate;
};

#endif
