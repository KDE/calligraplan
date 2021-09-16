/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANPORTFOLIO_RESOURCEMODEL_H
#define PLANPORTFOLIO_RESOURCEMODEL_H

#include <QAbstractItemModel>

class MainDocument;
class KoDocument;

namespace KPlato {
    class Project;
}


#define DOCUMENT_ROLE 100440
#define PROJECT_ROLE 100441
#define ISPORTFOLIO_ROLE 100442
#define RESOURCEID_ROLE 100501

class ResourceModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged);
public:

    explicit ResourceModel(QObject *parent = nullptr);
    virtual ~ResourceModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    MainDocument *portfolio() const;

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
    void updateData();
    MainDocument *m_portfolio;
    QMap<QString, QString> m_resourceIds;
};

#endif
