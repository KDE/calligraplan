/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kptloadsharedprojectsdialog.h"

#include "kptproject.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QStringList>
#include <QList>
#include <QUrl>
#include <QFileInfo>
#include <QDir>

#include <KLocalizedString>

using namespace KPlato;

LoadSharedProjectsDialog::LoadSharedProjectsDialog(Project &project, const QUrl &own, QWidget *parent)
    : KoDialog(parent)
    , m_view(new QTreeView(this))
{
    setCaption(xi18nc("@title:window", "Load Resource Assignments"));
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    setMainWidget(m_view);

    QList<QUrl> skip;
    skip << own << project.sharedProjectsUrl();

    QFileInfo fi(project.sharedProjectsUrl().path());
    QDir dir = fi.dir();
    QList<QUrl> paths;
    const QList<QString> files = dir.entryList(QStringList()<<"*.plan");
    for(const QString &f : files) {
        QString path = dir.canonicalPath();
        if (path.isEmpty()) {
            continue;
        }
        path += '/' + f;
        QUrl url = QUrl::fromLocalFile(path);
        if (!skip.contains(url)) {
            paths << url;
        }
    }
    QStandardItemModel *m = new QStandardItemModel(m_view);
    for (int i = 0; i < paths.count(); ++i) {
        QStandardItem *item = new QStandardItem();
        item->setData(paths.at(i).fileName(), Qt::DisplayRole);
        item->setData(paths.at(i), Qt::UserRole);
        item->setData(paths.at(i).toDisplayString(), Qt::ToolTipRole);
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        m->appendRow(item);
    }
    m_view->setModel(m);
    m_view->setHeaderHidden(true);
    m_view->setRootIsDecorated(false);
}

QList<QUrl> LoadSharedProjectsDialog::urls() const
{
    QList<QUrl> urls;
    QStandardItemModel *m = qobject_cast<QStandardItemModel*>(m_view->model());
    if (m) {
        for (int i = 0; i < m->rowCount(); ++i) {
            QStandardItem *item = m->item(i);
            if (item->checkState()) {
                urls << item->data(Qt::UserRole).toUrl();
            }
        }
    }
    return urls;
}
