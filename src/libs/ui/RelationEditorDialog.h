/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
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
    QWidget *createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
    void setEditorData( QWidget *editor, const QModelIndex &index ) const override;
    void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const override;
    void updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;

private:
    Project *m_project;
    Node *m_task;
};

class TypeDelegate : public BaseDelegate
{
public:
    TypeDelegate(QObject *parent);
    
protected:
    QWidget *createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
    void setEditorData( QWidget *editor, const QModelIndex &index ) const override;
    void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const override;
    void updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
};

class LagDelegate : public BaseDelegate
{
public:
    LagDelegate(QObject *parent);
    
protected:
    QWidget *createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
    void setEditorData( QWidget *editor, const QModelIndex &index ) const override;
    void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const override;
    void updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

}

#endif
