/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "Part.h"

#include "View.h"
#include "MainWindow.h"
#include "MainDocument.h"
#include "PortfolioFactory.h"
#include "config/ConfigDialog.h"
#include "portfoliosettings.h"

#include <KoComponentData.h>
#include <KoDocumentEntry.h>
#include <KActionCollection>
#include <Help.h>

#include <QAction>

Part::Part(QObject *parent)
    : KoPart(PortfolioFactory::global(), parent)
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
    MainWindow *w = new MainWindow(PLANPORTFOLIO_MIME_TYPE.latin1(), componentData());
    KoDocumentEntry entry = KoDocumentEntry::queryByMimeType(PLANPORTFOLIO_MIME_TYPE);
    QJsonObject json = entry.metaData();
    auto docs = json.value(QStringLiteral("X-PLAN-Documentation")).toVariant().toString().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    auto help = KPlato::Help::instance();
    help->setDocs(docs);
//     help->setContentsUrl(QUrl(KPlatoSettings::documentationPath()));
//     help->setContextUrl(QUrl(KPlatoSettings::contextPath()));
    qApp->installEventFilter(help); // this must go after filter installed by KMainWindow, so it will be called before

    auto a = w->actionCollection()->action(QStringLiteral("configure"));
    if (a) {
        a->setText(i18n("Configure Portfolio..."));
    }
    return w;
}

QString Part::recentFilesGroupName() const
{
    return QStringLiteral("Recent Portfolios");
}

void Part::configure(KoMainWindow *mw)
{
    Q_ASSERT(mw == currentMainwindow());
    if (KConfigDialog::showDialog(xi18nc("title:window", "Portfolio Settings"))) {
        return;
    }
    ConfigDialog *dialog = new ConfigDialog(this, xi18nc("@title:window", "Portfolio Settings"), PortfolioSettings::self());
    connect(dialog, &ConfigDialog::settingsUpdated, this, &Part::slotSettingsUpdated, Qt::QueuedConnection);
    dialog->open();
}

void Part::slotSettingsUpdated()
{
//     new Help(KPlatoSettings::contextPath(), KPlatoSettings::contextLanguage());
//     if (startUpWidget) {
//         static_cast<WelcomeView*>(startUpWidget->widget(0))->setProjectTemplatesModel();
//     }
}
