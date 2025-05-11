/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007, 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTUSEDEFFORTEDITOR_H
#define KPTUSEDEFFORTEDITOR_H

#include "planui_export.h"

#include <QTableView>
#include <QAbstractItemModel>
#include <QMultiMap>

#include "kpttask.h"

class QDate;

namespace KPlato
{

class Completion;
class Resource;
class Project;

class PLANUI_EXPORT UsedEffortItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit UsedEffortItemModel(QWidget *parent = nullptr);

    void setProject(Project *project) { m_project = project; }

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant & value, int role = Qt::EditRole) override;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }
    QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex()) const override;

    void setCompletion(Completion *completion);
    const Resource *resource(const QModelIndex &index) const;
    Completion::UsedEffort *usedEffort(const QModelIndex &index) const;
    void setCurrentMonday(const QDate &date);

    QModelIndex addRow();
    QMultiMap<QString, const Resource*> freeResources() const;

    void setReadOnly(bool ro) { m_readonly = ro; }
    bool readOnly() const { return m_readonly; }

Q_SIGNALS:
    void rowInserted(const QModelIndex&);
    void changed();
    void effortChanged(const QDate &date);

public Q_SLOTS:
    bool submit() override;
    void revert() override;
    void addResource(const QString &name);

private:
    Project *m_project;
    Completion *m_completion;
    QList<QDate> m_dates;
    QStringList m_headers;
    QList<const Resource*> m_resourcelist;
    QMultiMap<QString, const Resource*> m_editlist;
    bool m_readonly;
};

class PLANUI_EXPORT UsedEffortEditor : public QTableView
{
    Q_OBJECT
public:
    explicit UsedEffortEditor(QWidget *parent);
    void setProject(Project *project);
    void setCompletion(Completion *completion);
    void setCurrentMonday(const QDate &date);
    void addResource();

    bool hasFreeResources() const;

    UsedEffortItemModel *model() const { return static_cast<UsedEffortItemModel*>(QTableView::model()); }

    QSize sizeHint() const override { return m_size.isValid() ? m_size : QTableView::sizeHint(); }
    void setSizeHint(const QSize &size) { m_size = size; }

Q_SIGNALS:
    void changed();
    void resourceAdded();

private:
    QSize m_size;
};

//--------------------------------------------
class PLANUI_EXPORT CompletionEntryItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Properties {
            Property_Date,            /// Date of entry
            Property_Completion,      /// % Completed
            Property_UsedEffort,      /// Used Effort
            Property_RemainingEffort, /// Remaining Effort
            Property_PlannedEffort    /// Planned Effort
    };

    explicit CompletionEntryItemModel(QObject *parent = nullptr);

    void setTask(Task *t);

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant & value, int role = Qt::EditRole) override;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &) const override { return QModelIndex(); }
    QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex()) const override;

    void setCompletion(Completion *completion);
    const Resource *resource(const QModelIndex &index) const;
    Completion::UsedEffort *usedEffort(const QModelIndex &index) const;
    void setCurrentMonday(const QDate &date);

    QModelIndex addRow();
    void removeRow(int row);

    /// These flags are in addition to flags return from QAbstractItemModel::flags()
    void setFlags(int col, Qt::ItemFlags flags) { m_flags[ col ] = flags; }

    long id() const { return m_manager == nullptr ? -1 : m_manager->scheduleId(); }

    void addRow(const QDate &date);

Q_SIGNALS:
    void rowInserted(const QDate&);
    void rowRemoved(const QDate&);
    void changed();

public Q_SLOTS:
    bool submit() override;
    void revert() override;
    void slotDataChanged();
    void setManager(KPlato::ScheduleManager *sm);

protected:
    QVariant date (int row, int role = Qt::DisplayRole) const;
    QVariant percentFinished (int row, int role) const;
    QVariant remainingEffort (int row, int role) const;
    virtual QVariant actualEffort (int row, int role) const;
    QVariant plannedEffort (int row, int role) const;

    void removeEntry(const QDate& date);
    void addEntry(const QDate& date);
    void refresh();

    QList<qint64> scales() const;

protected:
    Task *m_node;
    Project *m_project;
    ScheduleManager *m_manager;
    Completion *m_completion;
    QList<QDate> m_dates;
    QStringList m_headers;
    QList<QDate> m_datelist;
    QList<Qt::ItemFlags> m_flags;
};

class PLANUI_EXPORT CompletionEntryEditor : public QTableView
{
    Q_OBJECT
public:
    explicit CompletionEntryEditor(QWidget *parent);
    void setCompletion(Completion *completion);

    CompletionEntryItemModel *model() const { return static_cast<CompletionEntryItemModel*>(QTableView::model()); }
    void setCompletionModel(CompletionEntryItemModel *m);

    void insertEntry(const QDate &date);

Q_SIGNALS:
    void changed();
    void rowInserted(const QDate&);
    void rowRemoved(const QDate&);
    void selectedItemsChanged(const QItemSelection&, const QItemSelection&);

public Q_SLOTS:
    void addEntry();
    void removeEntry();

private:
    
};


}  //KPlato namespace

#endif // KPTUSEDEFFORTEDITOR_H
