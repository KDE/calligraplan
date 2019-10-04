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

// clazy:excludeall=qstring-arg
#include "RelationEditorDialog.h"
#include "kptrelation.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptdurationspinbox.h"

#include <KLocalizedString>

#include <QStandardItemModel>
#include <QComboBox>
#include <QTimer>
#include <QHash>
#include <QHashIterator>
#include <QMutableHashIterator>

using namespace KPlato;

QList<Task*> sortedTasks(Project *project, Task *task, const QModelIndex &index)
{
    QList<Task*> lst;
    const QList<Task*> tasks = project->allTasks();
    int pos = tasks.indexOf(task);
    const QAbstractItemModel *model = index.model();
    const QString name = index.data().toString();
    for (int i = pos-1; i >= 0; --i) {
        Task *t = tasks.at(i);
        if (t == task) {
            continue;
        }
        if (name == t->name()) {
            lst << t;
        } else {
            bool match = !model->match(QModelIndex(), Qt::DisplayRole, t->name()).isEmpty();
            if (match || project->legalToLink(t, task)) {
                lst << t;
            }
        }
    }
    for (int i = pos+1; i < tasks.count(); ++i) {
        Task *t = tasks.at(i);
        if (t == task) {
            continue;
        }
        if (name == t->name()) {
            lst << t;
        } else {
            bool match = !model->match(QModelIndex(), Qt::DisplayRole, t->name()).isEmpty();
            if (match || project->legalToLink(t, task)) {
                lst << t;
            }
        }
    }
    return lst;
}

BaseDelegate::BaseDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{        
}

void BaseDelegate::slotEditorDestroyed(QObject*o)
{
    qInfo()<<Q_FUNC_INFO<<o;
    emit const_cast<BaseDelegate*>(this)->editModeChanged(false);
}

PredeccessorDelegate::PredeccessorDelegate(Project *project, Node *task, QObject *parent)
: BaseDelegate(parent)
, m_project(project)
, m_task(task)
{
}

QWidget *PredeccessorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QComboBox *editor = new QComboBox(parent);
    editor->installEventFilter(const_cast<PredeccessorDelegate*>(this));
    connect(editor, &QComboBox::destroyed, this, &BaseDelegate::slotEditorDestroyed);
    emit const_cast<PredeccessorDelegate*>(this)->editModeChanged(true);
    return editor;
}

void PredeccessorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    if (m_task && m_project) {
        QList<Task*> tasks = sortedTasks(m_project, static_cast<Task*>(m_task), index);
        for (Task *t : tasks) {
            e->addItem(t->name(), t->id());
        }
    }
    e->setCurrentText(index.data().toString());
}

void PredeccessorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    QString name = e->currentText();
    QVariant id = e->currentData();
    model->setData(index, id, Qt::UserRole);
    model->setData(index, name, Qt::EditRole);
}

void PredeccessorDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QRect r = option.rect;
    editor->setGeometry(r);
}

TypeDelegate::TypeDelegate(QObject *parent)
: BaseDelegate(parent)
{
}

QWidget *TypeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QComboBox *editor = new QComboBox(parent);
    editor->installEventFilter(const_cast<TypeDelegate*>(this));
    connect(editor, &QComboBox::destroyed, this, &BaseDelegate::slotEditorDestroyed);
    emit const_cast<TypeDelegate*>(this)->editModeChanged(true);
    return editor;
}

void TypeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    QStandardItemModel *comboModel = new QStandardItemModel(e);
    const QStringList types = Relation::typeList(true);
    e->addItems(types);
    e->setCurrentText(index.data().toString());
}

void TypeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    QString name = e->currentText();
    model->setData(index, index.row(), Qt::UserRole);
    model->setData(index, name, Qt::EditRole);
}

void TypeDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QRect r = option.rect;
    editor->setGeometry(r);
}

LagDelegate::LagDelegate(QObject *parent)
: BaseDelegate(parent)
{
}

QWidget *LagDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    DurationSpinBox *editor = new DurationSpinBox(parent);
    editor->installEventFilter(const_cast<LagDelegate*>(this));
    connect(editor, &QComboBox::destroyed, this, &BaseDelegate::slotEditorDestroyed);
    emit const_cast<LagDelegate*>(this)->editModeChanged(true);
    return editor;
}

void LagDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    DurationSpinBox *dsb = static_cast<DurationSpinBox*>(editor);
    dsb->setMinimumUnit((Duration::Unit)(index.data( Role::Minimum ).toInt()));
    dsb->setMaximumUnit((Duration::Unit)(index.data( Role::Maximum ).toInt()));
    dsb->setUnit((Duration::Unit)(index.model()->data(index, Role::DurationUnit).toInt()));
    dsb->setValue(index.model()->data( index, Qt::EditRole).toDouble() );
}

void LagDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    DurationSpinBox *dsb = static_cast<DurationSpinBox*>(editor);
    model->setData(index, dsb->value(), Qt::EditRole);
    model->setData(index, dsb->unit(), Role::DurationUnit);
}

void LagDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QRect r = option.rect;
    editor->setGeometry(r);
}

void LagDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());
    
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    Duration::Unit unit = static_cast<Duration::Unit>(index.data(Role::DurationUnit).toInt());
    Duration duration(index.data().toDouble(), unit);
    opt.text = duration.format(unit, 2);

    QStyle *style = QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, nullptr);

}

RelationEditorDialog::RelationEditorDialog(Project *project, Node *task, QWidget *parent)
    : KoDialog(parent)
    , m_project(project)
    , m_task(task)
      
{
    ui.setupUi(this);

    setCaption(xi18nc("@title:window", "Edit Dependency"));
    setButtons(KoDialog::Ok|KoDialog::Cancel );
    showButtonSeparator( true );

//     m_relationModel.setProject(project);
//     m_relationModel.setNode(task);

    ui.taskName->setText(task->name());

    QStandardItemModel *m = new QStandardItemModel(0, 3, ui.view);
    QStringList headers;
    headers << xi18nc("@title:column", "Predecessor");
    headers << xi18nc("@title:column", "Type");
    headers << xi18nc("@title:column", "Lag");
    m->setHorizontalHeaderLabels(headers);
    const QList<Relation*> relations = task->dependParentNodes();
    for (Relation *r : relations) {
        m->insertRow(m->rowCount());
        QModelIndex idx = m->index(m->rowCount()-1, 0);
        m->setData(idx, r->parent()->name());
        m->setData(idx, r->parent()->id(), Qt::UserRole);
        idx = idx.sibling(idx.row(), 1);
        m->setData(idx, r->typeToString(true), Qt::EditRole);
        m->setData(idx, r->type(), Qt::UserRole);
        idx = idx.sibling(idx.row(), 2);
        Duration lag = r->lag();
        m->setData(idx, lag.toDouble(Duration::Unit_h), Qt::EditRole);
        m->setData(idx, Duration::Unit_m, Role::Minimum);
        m->setData(idx, Duration::Unit_M, Role::Maximum);
        m->setData(idx, Duration::Unit_h, Role::DurationUnit);
    }
    ui.view->setModel(m);

    BaseDelegate *del = new PredeccessorDelegate(project, task, ui.view);
    connect(del, &BaseDelegate::editModeChanged, ui.addBtn, &QAbstractButton::setDisabled);
    connect(del, &BaseDelegate::editModeChanged, ui.removeBtn, &QAbstractButton::setDisabled);
    ui.view->setItemDelegateForColumn(0, del);
    del = new TypeDelegate(ui.view);
    connect(del, &BaseDelegate::editModeChanged, ui.addBtn, &QAbstractButton::setDisabled);
    connect(del, &BaseDelegate::editModeChanged, ui.removeBtn, &QAbstractButton::setDisabled);
    ui.view->setItemDelegateForColumn(1, del);
    del = new LagDelegate(ui.view);
    connect(del, &BaseDelegate::editModeChanged, ui.addBtn, &QAbstractButton::setDisabled);
    connect(del, &BaseDelegate::editModeChanged, ui.removeBtn, &QAbstractButton::setDisabled);
    ui.view->setItemDelegateForColumn(2, del);
    ui.view->setColumnWidth(0, 280);

    setMainWidget(ui.mainWidget);

    connect(ui.addBtn, &QToolButton::clicked, this, &RelationEditorDialog::addRelation);
    connect(ui.removeBtn, &QToolButton::clicked, this, &RelationEditorDialog::removeRelation);

    if (relations.isEmpty()) {
        QTimer::singleShot(0, this, &RelationEditorDialog::addRelation);
    }
//     connect(ui.view, &QAbstractItemView::editorDestroyed, this, &RelationEditorDialog::slotEditModeChanged);
}

void RelationEditorDialog::slotEditModeChanged(QObject *e)
{
    qInfo()<<Q_FUNC_INFO<<e;
}

void RelationEditorDialog::addRelation()
{
    QStandardItemModel *m = static_cast<QStandardItemModel*>(ui.view->model());
    m->setRowCount(m->rowCount()+1);
    QModelIndex idx = m->index(m->rowCount()-1, 1);
    m->setData(idx, Relation::typeList().at(0));
    m->setData(idx, Relation::FinishStart, Qt::UserRole);

    idx = idx.sibling(idx.row(), 2);
    m->setData(idx, 0.0);
    m->setData(idx, Duration::Unit_m, Role::Minimum);
    m->setData(idx, Duration::Unit_M, Role::Maximum);
    m->setData(idx, Duration::Unit_h, Role::DurationUnit);

    idx = idx.sibling(idx.row(), 0);
    
    ui.view->edit(idx);
}

void RelationEditorDialog::removeRelation()
{
    QModelIndex idx = ui.view->selectionModel()->currentIndex();
    qInfo()<<Q_FUNC_INFO<<idx;
    if (idx.isValid()) {
        ui.view->model()->removeRow(idx.row());
    }
}

MacroCommand *RelationEditorDialog::buildCommand() {
    MacroCommand *c = new MacroCommand( kundo2_i18n("Modify dependency") );
    enum State {Skip, Deleted};
    QHash<Relation*, State> relations;
    for (int j = 0; j < m_task->numDependParentNodes(); ++j) {
        relations.insert(m_task->getDependParentNode(j), Deleted);
    }
    const QAbstractItemModel *m = ui.view->model();
    for (int i = 0; i < m->rowCount(); ++i) {
        QModelIndex idx = m->index(i, 0);
        Node *pred = m_project->findNode(idx.data(Qt::UserRole).toString());
        Q_ASSERT(pred);
        idx = idx.sibling(i, 1);
        Relation::Type type = (Relation::Type)idx.data(Qt::UserRole).toInt();
        idx = idx.sibling(i, 2);
        Duration lag = Duration(idx.data().toDouble(), (Duration::Unit)idx.data(Role::DurationUnit).toInt());
        bool found = false;
        QMutableHashIterator<Relation*, State> it(relations);
        while (it.hasNext()) {
            it.next();
            Relation *r = it.key();
            if (pred == r->parent()) {
                it.setValue(Skip);
                if (type == r->type()) {
                    c->addCommand(new ModifyRelationTypeCmd(r, type));
                }
                if (lag == r->lag()) {
                    c->addCommand(new ModifyRelationLagCmd(r, lag));
                }
                found = true;
                break;
            }
        }
        if (!found) {
            Relation *rel = new Relation(pred, m_task, type, lag);
            c->addCommand(new AddRelationCmd(*m_project, rel));
        }
    }
    QHashIterator<Relation*, State> it(relations);
    while (it.hasNext()) {
        it.next();
        switch (it.value()) {
            case Deleted:
                c->addCommand(new DeleteRelationCmd(*m_project, it.key()));
                break;
            default:
                break;
        }
    }
    if (c->isEmpty()) {
        delete c;
        c = nullptr;
    }
    return c;
}

