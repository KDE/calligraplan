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
    foreach(const QString &f, dir.entryList(QStringList()<<"*.plan")) {
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
