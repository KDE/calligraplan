/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
