/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "RelationEditorDialog.h"
#include "kptrelation.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptdurationspinbox.h"
#include "KoIcon.h"

#include <KLocalizedString>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QComboBox>
#include <QTimer>
#include <QHash>
#include <QHashIterator>
#include <QMutableHashIterator>
#include <QMutableListIterator>

#define TASKID_ROLE Qt::UserRole
#define TASKDELETED_ROLE Qt::UserRole+246
#define RELATIONTYPE_ROLE Qt::UserRole

using namespace KPlato;

QList<Task*> sortedTasks(Project *project, Task *task, const QModelIndex &index)
{
    QList<Task*> lst;
    const QList<Node*> tasks = project->allNodes(true);
    int pos = tasks.indexOf(task);
    const QAbstractItemModel *model = index.model();
    const QString name = index.data().toString();
    for (int i = pos-1; i >= 0; --i) {
        Task *t = static_cast<Task*>(tasks.at(i));
        if (t == task) {
            continue;
        }
        if (index.isValid() && name == t->name()) {
            lst << t;
        } else {
            bool match = !model->match(QModelIndex(), Qt::DisplayRole, t->name()).isEmpty();
            if (match || project->legalToLink(t, task)) {
                lst << t;
            }
        }
    }
    for (int i = pos+1; i < tasks.count(); ++i) {
        Task *t = static_cast<Task*>(tasks.at(i));
        if (t == task) {
            continue;
        }
        if (index.isValid() && name == t->name()) {
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

void BaseDelegate::slotEditorDestroyed(QObject*)
{
    Q_EMIT const_cast<BaseDelegate*>(this)->editModeChanged(false);
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
    Q_EMIT const_cast<PredeccessorDelegate*>(this)->editModeChanged(true);
    return editor;
}

void PredeccessorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    if (m_task && m_project) {
        const QList<Task*> tasks = sortedTasks(m_project, static_cast<Task*>(m_task), index);
        for (Task *t : tasks) {
            e->addItem(t->name(), t->id());
        }
    }
    if (index.isValid() && index.data().isValid()) {
        e->setCurrentText(index.data().toString());
    } else {
        e->setCurrentIndex(0);
    }
}

void PredeccessorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    QString name = e->currentText();
    QVariant id = e->currentData();
    model->setData(index, id, TASKID_ROLE);
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
    Q_EMIT const_cast<TypeDelegate*>(this)->editModeChanged(true);
    return editor;
}

void TypeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    const QStringList types = Relation::typeList(true);
    e->addItems(types);
    e->setCurrentText(index.data().toString());
}

void TypeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *e = static_cast<QComboBox*>(editor);
    model->setData(index, e->currentIndex(), RELATIONTYPE_ROLE);
    model->setData(index, e->currentText(), Qt::EditRole);
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
    Q_EMIT const_cast<LagDelegate*>(this)->editModeChanged(true);
    return editor;
}

void LagDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    DurationSpinBox *dsb = static_cast<DurationSpinBox*>(editor);
    dsb->setMinimumUnit((Duration::Unit)(index.data(Role::Minimum).toInt()));
    dsb->setMaximumUnit((Duration::Unit)(index.data(Role::Maximum).toInt()));
    dsb->setUnit((Duration::Unit)(index.model()->data(index, Role::DurationUnit).toInt()));
    dsb->setValue(index.model()->data(index, Qt::EditRole).toDouble());
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
    , m_task(qobject_cast<Task*>(task))

{
    Q_ASSERT(m_task);
    ui.setupUi(this);

    // set actions to get shortcuts
    QAction *a = new QAction(ui.addBtn->icon(), ui.addBtn->text(), ui.addBtn);
    a->setShortcut(Qt::Key_Insert);
    ui.addBtn->setDefaultAction(a);
    a = new QAction(ui.removeBtn);
    a->setIcon(koIcon("edit-delete"));
    a->setText(i18n("Delete"));
    a->setShortcut(Qt::Key_Delete);
    ui.removeBtn->setDefaultAction(a);

    setCaption(xi18nc("@title:window", "Edit Dependency"));
    setButtons(KoDialog::Ok|KoDialog::Cancel);
    showButtonSeparator(true);

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
        m->setData(idx, r->parent()->id(), TASKID_ROLE);
        idx = idx.sibling(idx.row(), 1);
        m->setData(idx, r->typeToString(true), Qt::EditRole);
        m->setData(idx, r->type(), RELATIONTYPE_ROLE);
        idx = idx.sibling(idx.row(), 2);
        Duration lag = r->lag();
        m->setData(idx, lag.toDouble(Duration::Unit_h), Qt::EditRole);
        m->setData(idx, Duration::Unit_m, Role::Minimum);
        m->setData(idx, Duration::Unit_M, Role::Maximum);
        m->setData(idx, Duration::Unit_h, Role::DurationUnit);
    }
    ui.view->setModel(m);

    BaseDelegate *del = new PredeccessorDelegate(project, task, ui.view);
    connect(del, &BaseDelegate::editModeChanged, this, &RelationEditorDialog::slotDisableInsert);
    connect(del, &BaseDelegate::editModeChanged, this, &RelationEditorDialog::slotDisableRemove);
    ui.view->setItemDelegateForColumn(0, del);
    del = new TypeDelegate(ui.view);
    connect(del, &BaseDelegate::editModeChanged, ui.addBtn, &QAbstractButton::setDisabled);
    connect(del, &BaseDelegate::editModeChanged, this, &RelationEditorDialog::slotDisableRemove);
    ui.view->setItemDelegateForColumn(1, del);
    del = new LagDelegate(ui.view);
    connect(del, &BaseDelegate::editModeChanged, ui.addBtn, &QAbstractButton::setDisabled);
    connect(del, &BaseDelegate::editModeChanged, this, &RelationEditorDialog::slotDisableRemove);
    ui.view->setItemDelegateForColumn(2, del);
    ui.view->setColumnWidth(0, 280);

    setMainWidget(ui.mainWidget);

    connect(ui.addBtn, &QToolButton::triggered, this, &RelationEditorDialog::addRelation);
    connect(ui.removeBtn, &QToolButton::triggered, this, &RelationEditorDialog::removeRelation);

    if (relations.isEmpty()) {
        QTimer::singleShot(0, this, &RelationEditorDialog::addRelation);
    }
    slotDisableInsert(false);
    connect(ui.view->selectionModel(), &QItemSelectionModel::currentChanged, this, &RelationEditorDialog::slotCurrentChanged);
    slotCurrentChanged(ui.view->selectionModel()->currentIndex());

    setWhatsThis( xi18nc("@info:whatsthis",
                           "<title>Task Dependency Dialog</title>"
                           "<para>"
                           "The task dependency dialog enables you to easily add a predecessor to the selected task."
                           "<nl/>When opened, it presents the previous task as the default predecessor."
                           " This is often the desired predecessor, and can be added by just pressing <interface>OK</interface>."
                           "<nl/>The dropdown list of possible predecessors is sorted with the most likely tasks first."
                           "</para>"
                          ));
}

void RelationEditorDialog::slotCurrentChanged(const QModelIndex &idx)
{
    QStandardItemModel *m = static_cast<QStandardItemModel*>(ui.view->model());
    QStandardItem *item = m->itemFromIndex(idx);
    if (!item) {
        ui.removeBtn->setEnabled(false);
        return;
    } else {
        ui.removeBtn->setEnabled(true);
    }

    if (item->data().toBool()) {
        ui.removeBtn->defaultAction()->setIcon(koIcon("edit-undo"));
        ui.removeBtn->defaultAction()->setText(i18n("Un-Delete"));
    } else {
        ui.removeBtn->defaultAction()->setIcon(koIcon("edit-delete"));
        ui.removeBtn->defaultAction()->setText(i18n("Delete"));
    }
}

void RelationEditorDialog::slotDisableInsert(bool _disable)
{
    bool disable = _disable;
    if (!disable) {
        QList<Task*> tasks = m_project->allTasks();
        tasks.removeAll(m_task);
        QMutableListIterator<Task*> it(tasks);
        while (it.hasNext()) {
            Task *t = it.next();
            if (!m_project->legalToLink(t, m_task)) {
                it.remove();
            }
        }
        for(int i = 0; i < ui.view->model()->rowCount(); ++i) {
            QModelIndex idx = ui.view->model()->index(i, 0);
            tasks.removeAll(static_cast<Task*>(m_project->findNode(idx.data(TASKID_ROLE).toString())));
        }
        disable = tasks.isEmpty();
    }
    ui.addBtn->setDisabled(disable);
}

void RelationEditorDialog::slotDisableRemove(bool disable)
{
    QModelIndex cidx = ui.view->selectionModel()->currentIndex();
    ui.removeBtn->setDisabled(disable || !cidx.isValid());
}

void RelationEditorDialog::addRelation()
{
    QStandardItemModel *m = static_cast<QStandardItemModel*>(ui.view->model());
    m->setRowCount(m->rowCount()+1);
    QModelIndex idx = m->index(m->rowCount()-1, 1);
    m->setData(idx, Relation::typeList().at(0));
    m->setData(idx, Relation::FinishStart, RELATIONTYPE_ROLE);

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
    QModelIndex cidx = ui.view->selectionModel()->currentIndex();
    if (!cidx.isValid()) return;

    cidx = cidx.sibling(cidx.row(), 0);
    if (cidx.isValid()) {
        QModelIndex idx = cidx;
        QStandardItemModel *m = static_cast<QStandardItemModel*>(ui.view->model());
        QStandardItem *item = m->itemFromIndex(idx);
        if (item->data().toBool()) {
            // undelete
            item->setIcon(QIcon());
            item->setData(false, TASKDELETED_ROLE); // deleted
            idx = idx.sibling(idx.row(), 1);
            m->itemFromIndex(idx)->setEnabled(true);
            idx = idx.sibling(idx.row(), 2);
            m->itemFromIndex(idx)->setEnabled(true);
        } else {
            item->setIcon(koIcon("edit-delete"));
            item->setData(true, TASKDELETED_ROLE); // deleted
            idx = idx.sibling(idx.row(), 1);
            m->itemFromIndex(idx)->setEnabled(false);
            idx = idx.sibling(idx.row(), 2);
            m->itemFromIndex(idx)->setEnabled(false);
        }
    }
    slotDisableInsert(false);
    slotCurrentChanged(cidx);
}

MacroCommand *RelationEditorDialog::buildCommand() {
    MacroCommand *c = new MacroCommand(kundo2_i18n("Modify dependency"));
    enum State {Skip, Deleted};
    QHash<Relation*, State> relations;
    for (int j = 0; j < m_task->numDependParentNodes(); ++j) {
        relations.insert(m_task->getDependParentNode(j), Deleted);
    }
    const QAbstractItemModel *m = ui.view->model();
    for (int i = 0; i < m->rowCount(); ++i) {
        QModelIndex idx = m->index(i, 0);
        if (idx.data(TASKDELETED_ROLE).toBool()) {
            continue; // deleted
        }
        Node *pred = m_project->findNode(idx.data(TASKID_ROLE).toString());
        Q_ASSERT(pred);
        idx = idx.sibling(i, 1);
        Relation::Type type = (Relation::Type)idx.data(RELATIONTYPE_ROLE).toInt();
        idx = idx.sibling(i, 2);
        Duration lag = Duration(idx.data().toDouble(), (Duration::Unit)idx.data(Role::DurationUnit).toInt());
        bool found = false;
        QMutableHashIterator<Relation*, State> it(relations);
        while (it.hasNext()) {
            it.next();
            Relation *r = it.key();
            if (pred == r->parent()) {
                it.setValue(Skip);
                if (type != r->type()) {
                    c->addCommand(new ModifyRelationTypeCmd(r, type));
                }
                if (lag != r->lag()) {
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

