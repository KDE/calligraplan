/* This file is part of the KDE project
* Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef PLANPORTFOLIO_PROJECTSMODEL_H
#define PLANPORTFOLIO_PROJECTSMODEL_H

#include <kptnodeitemmodel.h>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QString>
#include <QUrl>

class MainDocument;
class KoDocument;

namespace KPlato {
    class Project;
}

class ProjectsModel;

#define DOCUMENT_ROLE 100440
#define PROJECT_ROLE 100441
#define ISPORTFOLIO_ROLE 100442

class ProjectsFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged);

public:
    explicit ProjectsFilterModel(QObject *parent = nullptr);
    ~ProjectsFilterModel();

    void setAcceptedRows(const QList<int> rows);
    void setAcceptedColumns(const QList<int> columns);

    MainDocument *portfolio() const;
    KoDocument *documentFromIndex(const QModelIndex &idx) const;

public Q_SLOTS:
    void setPortfolio(MainDocument *portfolio);

Q_SIGNALS:
    void portfolioChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;

private:
    ProjectsModel *m_baseModel;
    QList<int> m_acceptedRows;
    QList<int> m_acceptedColumns;
};

class ProjectsModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged);
public:
    enum ExtraColumns { ScheduleManagerColumn = 0 };

    explicit ProjectsModel(QObject *parent = nullptr);
    virtual ~ProjectsModel();

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;

    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    MainDocument *portfolio() const;

    KoDocument *documentFromIndex(const QModelIndex &idx) const;
    KoDocument *documentFromRow(int row) const;
    KPlato::Project *projectFromIndex(const QModelIndex &idx) const;

public Q_SLOTS:
    void setPortfolio(MainDocument *portfolio);

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
    MainDocument *m_portfolio;
    KPlato::NodeModel m_nodeModel;
};

#endif
