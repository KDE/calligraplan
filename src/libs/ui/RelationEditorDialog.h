/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTRELATIONEDITORDIALOG_H
#define KPTRELATIONEDITORDIALOG_H

#include "planui_export.h"

#include "ui_RelationEditorDialog.h"
#include <KoDialog.h>

#include <QStyledItemDelegate>

namespace KPlato
{

class Project;
class Node;
class Task;
class MacroCommand;


class PLANUI_EXPORT RelationEditorDialog : public KoDialog
{
    Q_OBJECT
public:
    RelationEditorDialog(Project *project, Node *task, QWidget *parent = nullptr);

    virtual MacroCommand *buildCommand();

public Q_SLOTS:
    void addRelation();
    void removeRelation();

private Q_SLOTS:
    void slotDisableRemove(bool disable);
    void slotDisableInsert(bool disable);
    void slotCurrentChanged(const QModelIndex &idx);

private:
    Project *m_project;
    Task *m_task;
    Ui::RelationEditorDialog ui;
};

class BaseDelegate : public  QStyledItemDelegate
{
    Q_OBJECT
public:
    BaseDelegate(QObject *parent);

Q_SIGNALS:
    void editModeChanged(bool);

public Q_SLOTS:
    void slotEditorDestroyed(QObject*);    
};

class PredeccessorDelegate : public BaseDelegate
{
    Q_OBJECT
public:
    PredeccessorDelegate(Project *project, Node *task, QObject *parent);
    
protected:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    Project *m_project;
    Node *m_task;
};

class TypeDelegate : public BaseDelegate
{
public:
    TypeDelegate(QObject *parent);
    
protected:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class LagDelegate : public BaseDelegate
{
public:
    LagDelegate(QObject *parent);
    
protected:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

}

#endif
