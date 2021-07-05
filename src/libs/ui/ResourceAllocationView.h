/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/


#ifndef ResourceAllocationView_h
#define ResourceAllocationView_h

#include <QTreeView>
#include <QPersistentModelIndex>

class KoDocument;

class QAction;

namespace KPlato
{

class Resource;

// class to use in resources docker
class ResourceAllocationView : public QTreeView
{
    Q_OBJECT
public:
    ResourceAllocationView(KoDocument *doc, QWidget *parent = nullptr);

    QList<Resource*> selectedResources() const;

public Q_SLOTS:
    void setSelectedTasks(const QItemSelection &selected, const QItemSelection &deselected);

protected Q_SLOTS:
    void slotAllocate();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    KoDocument *m_doc;
    QList<QPersistentModelIndex> m_tasks;

    QAction *m_allocateAction;
};


} // namespace KPlato

#endif
