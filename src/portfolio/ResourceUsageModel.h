/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANPORTFOLIO_RESOURCEUSAGEMODEL_H
#define PLANPORTFOLIO_RESOURCEUSAGEMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QString>
#include <QDate>
#include <QPointF>

class MainDocument;
class KoDocument;


namespace KPlato {
    class Project;
    class Node;
}

#define DOCUMENT_ROLE 100440
#define PROJECT_ROLE 100441
#define ISPORTFOLIO_ROLE 100442

class ResourceUsageModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged)
public:

    explicit ResourceUsageModel(QObject *parent = nullptr);
    virtual ~ResourceUsageModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation o, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    MainDocument *portfolio() const;

    void setCurrentResource(const QString &id);

public Q_SLOTS:
    void setPortfolio(MainDocument *portfolio);
    void setDateRange(const QDate &min, const QDate &max = QDate());

Q_SIGNALS:
    void portfolioChanged();

protected Q_SLOTS:
    void reset();
    void documentChanged(KoDocument *doc, int row);
    void projectChanged(KoDocument *doc);
    void documentAboutToBeInserted(int row);
    void documentInserted();
    void documentAboutToBeRemoved(int row);
    void documentRemoved();

protected:
    void initiateEmptyData();
    void updateData();

protected:
    MainDocument *m_portfolio;
    QString m_currentResourceId;
    // using a qmap within a qmap to get tasks sorted by name in legend
    QMap<QDate, QMap<QString, std::pair<KPlato::Node*, double>>> m_usage; // QMap<date, QMap<taskname, std:pair<Node*, effort (hours)> >
    double m_normalMax;
    double m_stackedMax;
    QMap<QDate, double> m_available;
    QDate m_minimumStartDate;
    QDate m_maximumEndDate;
};

class ResourceAvailableModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ResourceAvailableModel(QObject *parent = nullptr);

//     QModelIndex mapToSource(const QModelIndex & proxyIndex) const override;
//     QModelIndex mapFromSource(const QModelIndex & sourceIndex) const override;
//     QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
//     QModelIndex parent(const QModelIndex &idx) const override;
//     int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

public Q_SLOTS:
    void sourceReset();
};
#endif
