/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
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
#include "Part.h"

#include "View.h"
#include "MainDocument.h"
#include "Factory.h"

#include <KoMainWindow.h>
#include <KoComponentData.h>

#include <QAction>

Part::Part(QObject *parent)
    : KoPart(Factory::global(), parent)
{
}

Part::~Part()
{
}

KoDocument *Part::createDocument(KoPart *part) const
{
    return new MainDocument(part);
}

KoView *Part::createViewInstance(KoDocument *document, QWidget *parent)
{
    // synchronize view selector
    View *view = new View(this, qobject_cast<MainDocument*>(document), parent);
    return view;
}

KoMainWindow *Part::createMainWindow()
{
    KoMainWindow *w = new KoMainWindow(PLANPORTFOLIO_MIME_TYPE, componentData());
    QAction *handbookAction = w->action("help_contents");
    if (handbookAction) {
        disconnect(handbookAction, nullptr, nullptr, nullptr);
        connect(handbookAction, &QAction::triggered, this, &Part::slotHelpContents);
    }
    return w;
}

QString Part::recentFilesGroupName() const
{
    return QStringLiteral("Recent Portfolios");
}

void Part::configure(KoMainWindow *mw)
{
    qInfo()<<Q_FUNC_INFO;
//     if (KConfigDialog::showDialog(xi18nc("@info:title", "Portfolio Settings"))) {
//         return;
//     }
//     ConfigDialog *dialog = new ConfigDialog(mw, xi18nc("@info:title", "Portfolio Settings"), KPlatoSettings::self());
//     connect(dialog, &ConfigDialog::settingsUpdated, this, &Part::slotSettingsUpdated, Qt::QueuedConnection);
//     dialog->open();
}

void Part::slotSettingsUpdated()
{
//     new Help(KPlatoSettings::contextPath(), KPlatoSettings::contextLanguage());
//     if (startUpWidget) {
//         static_cast<WelcomeView*>(startUpWidget->widget(0))->setProjectTemplatesModel();
//     }
}
