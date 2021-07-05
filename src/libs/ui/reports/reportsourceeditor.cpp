/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "reportsourceeditor.h"
#include "report.h"

#include "kptnodeitemmodel.h"

#include <QDomElement>
#include <QTimer>

#include "kptdebug.h"

namespace KPlato
{

ReportSourceEditor::ReportSourceEditor(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    connect(ui_source, SIGNAL(currentIndexChanged(int)), SLOT(slotCurrentIndexChanged()));
}

void ReportSourceEditor::setModel(QAbstractItemModel *model)
{
    ui_source->setModel(model);
    ui_source->setCurrentIndex(0);
}

void ReportSourceEditor::slotCurrentIndexChanged()
{
    Q_EMIT selectFromChanged(selectFromTag());
}

QString ReportSourceEditor::selectFromTag() const
{
    QString tag;
    if (ui_source->currentIndex() >= 0) {
        QAbstractItemModel *m = ui_source->model();
        tag = m->index(ui_source->currentIndex(), 0).data(Reports::TagRole).toString();
    }
    return tag;
}

void ReportSourceEditor::setSourceData(const QDomElement &element)
{
    if (element.tagName() != "data-source") {
        debugPlan<<"no source element";
        ui_source->setCurrentIndex(0);
        return;
    }
    QString selectfrom = element.attribute("select-from");
    QAbstractItemModel *m = ui_source->model();
    for (int row = 0; row < m->rowCount(); ++row) {
        QString name = m->index(row, 0).data(Reports::TagRole).toString();
        if (! name.isEmpty() && name == selectfrom) {
            ui_source->setCurrentIndex(row);
            return;
        }
    }
    debugPlan<<"no source";
    ui_source->setCurrentIndex(0);
}

void ReportSourceEditor::sourceData(QDomElement &element) const
{
    QDomElement e = element.ownerDocument().createElement("data-source");
    element.appendChild(e);
    int row = ui_source->currentIndex();
    QAbstractItemModel *m = ui_source->model();
    e.setAttribute("select-from", m->index(row, 0).data(Reports::TagRole).toString());
}

} //namespace KPlato

