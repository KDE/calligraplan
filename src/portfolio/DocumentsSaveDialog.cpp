/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "DocumentsSaveDialog.h"

#include <MainDocument.h>

#include <KoDocument.h>

#include <QTreeView>
#include <QStandardItemModel>
#include <QUrl>

DocumentsSaveModel::DocumentsSaveModel(MainDocument *doc, QObject *parent)
    : QStandardItemModel(parent)
    , m_doc(doc)
{
    const auto labels = QStringList()
        << i18nc("@info:title", "Portfolio")
        << i18nc("@info:title", "Modified")
        << i18nc("@info:title", "Save")
        << i18nc("@info:title", "Internal")
        << i18nc("@info:title", "Url");
    setHorizontalHeaderLabels(labels);
//     if (true/*doc->isModified()*/) {
//         QList<QStandardItem*> items;
//         QStandardItem *item;
//         item = new QStandardItem();
//         item->setEnabled(false);
//         items << item;
//         const QString text = doc->isModified() ? i18n("Yes") : i18n("No");
//         item = new QStandardItem(text);
//         item->setEditable(false);
//         items << item;
//         item = new QStandardItem();
//         item->setEnabled(false);
//         items << item;
//         item =  new QStandardItem();
//         item->setEnabled(false);
//         items << item;
//         item = new QStandardItem(doc->url().toDisplayString());
//         items << item;
//         appendRow(items);
//     }
    for (auto subdoc : doc->documents()) {
        QList<QStandardItem*> items;
        QStandardItem *item;

        QString text = subdoc->property(ISPORTFOLIO).toBool() ? i18n("Yes") : i18n("No");
        item = new QStandardItem(text);
        item->setData(QVariant::fromValue<QObject*>(subdoc));
        item->setEditable(false);
        items << item;

        text = subdoc->isModified() ? i18n("Yes") : i18n("No");
        item = new QStandardItem(text);
        item->setEditable(false);
        items << item;

        item =  new QStandardItem();
        item->setCheckable(true);
        item->setCheckState(subdoc->isModified() ? Qt::Checked : Qt::Unchecked);
        items << item;

        item =  new QStandardItem();
        item->setCheckable(true);
        items << item;

        item = new QStandardItem(subdoc->url().toDisplayString());
        items << item;
        appendRow(items);
    }
}

DocumentsSaveDialog::DocumentsSaveDialog(MainDocument *doc, QWidget *parent)
    : KoDialog(parent)
    , m_doc(doc)
{
    setCaption(i18nc("@title:window", "Save Documents"));
    setMinimumSize(650, 500);

    ui.setupUi(this);

    ui.mainUrl->setText(doc->url().toDisplayString());
    ui.saveMain->setChecked(doc->isModified());

    auto model = new DocumentsSaveModel(doc, this);
    ui.subdocsView->setModel(model);
    ui.subdocsView->setAcceptDrops(false);
    ui.subdocsView->setRootIsDecorated(false);

    setMainWidget(ui.mainWidget);
}

bool DocumentsSaveDialog::saveMain() const
{
    return ui.saveMain->isChecked();
}

QUrl DocumentsSaveDialog::mainUrl() const
{
    return ui.mainUrl->url();
}

QList<KoDocument*> DocumentsSaveDialog::modifiedDocuments() const
{
    QList<KoDocument*> docs;
    const auto model = qobject_cast<QStandardItemModel*>(ui.subdocsView->model());
    const int count = model->rowCount();
    for (int row = 0; row < count; ++row) {
        const auto item = model->item(row);
        const auto doc = qobject_cast<KoDocument*>(item->data().value<QObject*>());
        if (doc) {
            docs << doc;
        }
    }
    return docs;
}
