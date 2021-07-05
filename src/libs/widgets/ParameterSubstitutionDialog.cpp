/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ParameterSubstitutionDialog.h"

#include <KLocalizedString>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QDebug>

using namespace KPlato;

ParameterSubstitutionDialog::ParameterSubstitutionDialog(const QStringList &parameters, QWidget *parent)
: KoDialog(parent)
{
    m_panel = new ParameterSubstitutionPanel(parameters, this);
    setMainWidget(m_panel);
}

QMap<QString, QString> ParameterSubstitutionDialog::parameters() const
{
    QMap<QString, QString> parameters;
    QStandardItemModel *m = static_cast<QStandardItemModel*>(m_panel->ui.treeView->model());
    for (int r = 0; r < m->rowCount(); ++r) {
        QStandardItem *item1 = m->item(r, 1);
        if (item1->checkState() == Qt::Checked) {
            QStandardItem *item0 = m->item(r, 0);
            parameters.insert(item0->text(), item1->text());
        }
    }
    return parameters;
}

ParameterSubstitutionPanel::ParameterSubstitutionPanel(const QStringList &parameters, QWidget *parent)
: QWidget(parent)
{
    ui.setupUi(this);

    QStandardItemModel *m = new QStandardItemModel(ui.treeView);
    QStringList labels;
    labels << xi18nc("title@column", "Parameter");
    labels << xi18nc("title@column", "Substitution");
    m->setHorizontalHeaderLabels(labels);
    ui.treeView->setModel(m);
    ui.treeView->setFocus();
    for (const QString &parameter : parameters) {
        QStandardItem *item = new QStandardItem(parameter);
        item->setEditable(false);
        item->setSelectable(false);
        QStandardItem *item1 = new QStandardItem();
        item1->setCheckable(true);
        item1->setCheckState(Qt::Checked);
        m->appendRow(QList<QStandardItem*>()<<item<<item1);
    }
    ui.treeView->selectionModel()->setCurrentIndex(m->index(0, 1), QItemSelectionModel::ClearAndSelect);
}

