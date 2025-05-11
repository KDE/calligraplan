/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2012 C. Boemann <cbo@kogmbh.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptpart.h"

#include "config/ConfigDialog.h"
#include "kptview.h"
#include "kptmaindocument.h"
#include "kptfactory.h"
#include "Help.h"
#include "calligraplansettings.h"
#include "kptcommand.h"
#include <kptmainprojectdialog.h>
#include "kptdebug.h"

#include <KoComponentData.h>
#include <WelcomeView.h>
#include <KoDocumentEntry.h>
#include <KoIcon.h>
#include <KoNetAccess.h>

#include <KRecentFilesAction>
#include <KXMLGUIFactory>
#include <KConfigGroup>
#include <KHelpClient>
#include <KDesktopFile>
#include <KAboutData>
#include <KActionCollection>

#include <QStackedWidget>
#include <QDesktopServices>
#include <QPointer>
#include <QUrl>
#include <QProcess>

using namespace KPlato;

Part::Part(QObject *parent)
    : KoPart(Factory::global(), parent)
{
    setTemplatesResourcePath(QStringLiteral("calligraplan/templates/"));
}

Part::~Part()
{
}

void Part::setDocument(KPlato::MainDocument *document)
{
    KoPart::setDocument(document);
    m_document = document;
}

KoDocument *Part::createDocument(KoPart *part) const
{
    return new MainDocument(part);
}

KoView *Part::createViewInstance(KoDocument *document, QWidget *parent)
{
    // synchronize view selector
    View *view = dynamic_cast<View*>(views().value(0));
    /*FIXME
    if (view && m_context) {
        QDomDocument doc = m_context->save(view);
        m_context->setContent(doc.toString());
    }*/
    view = new View(this, qobject_cast<MainDocument*>(document), parent);
//    connect(view, SIGNAL(destroyed()), this, SLOT(slotViewDestroyed()));
//    connect(document, SIGNAL(viewListItemAdded(const ViewListItem*,const ViewListItem*,int)), view, SLOT(addViewListItem(const ViewListItem*,const ViewListItem*,int)));
//    connect(document, SIGNAL(viewListItemRemoved(const ViewListItem*)), view, SLOT(removeViewListItem(const ViewListItem*)));
    return view;
}

KoMainWindow *Part::createMainWindow()
{
    KoMainWindow *w = new KoMainWindow(PLAN_MIME_TYPE.latin1(), componentData());
    w->setRecentFilesGroupName(QStringLiteral("Recent Projects"));

    KoDocumentEntry entry = KoDocumentEntry::queryByMimeType(PLAN_MIME_TYPE);
    QJsonObject json = entry.metaData();
    auto docs = json.value(QStringLiteral("X-PLAN-Documentation")).toVariant().toString().split(QLatin1Char(';'), Qt::SkipEmptyParts);

    auto help = Help::instance();
    help->setDocs(docs);
    help->initiate();
    qApp->installEventFilter(help);

    auto a = w->actionCollection()->action(QStringLiteral("configure"));
    if (a) {
        a->setText(i18n("Configure Plan..."));
    }
    connect(w, &KoMainWindow::configure, this, &Part::configure);
    return w;
}

void Part::slotOpenTemplate(const QUrl &url)
{
    openTemplate(url);
}

bool Part::openTemplate(const QUrl &url)
{
    debugPlan<<"Open shared resources template:"<<url;
    m_document->setLoadingTemplate(true);
    m_document->setLoadingSharedResourcesTemplate(url.fileName() == QStringLiteral("SharedResources.plant"));
    bool res = KoPart::openTemplate(url);
    m_document->setLoadingTemplate(false);
    if (res) {
        finish();
    }
    return res;
}

bool Part::openProjectTemplate(const QUrl &url)
{
    QApplication::setOverrideCursor(Qt::BusyCursor);
    m_document->setLoadingTemplate(true);
    bool ok = m_document->loadNativeFormat(url.path());
    m_document->setModified(false);
    m_document->undoStack()->clear();

    if (ok) {
        m_document->resetURL();
        m_document->setEmpty();
    } else {
        m_document->showLoadingErrorDialog();
        m_document->initEmpty();
    }
    m_document->setLoadingTemplate(false);
    QApplication::restoreOverrideCursor();
    return ok;
}

void Part::openTaskModule(const QUrl &url)
{
    Part *part = new Part(nullptr);
    MainDocument *doc = new MainDocument(part);
    part->setDocument(doc);
    doc->setIsTaskModule(true);
    mainWindows().first()->openDocument(part, url);
}

void Part::finish()
{
    mainWindows().first()->setRootDocument(document(), this);
}

void Part::configure(KoMainWindow *mw)
{
    Q_ASSERT(mw == currentMainwindow());
    if(KConfigDialog::showDialog(QStringLiteral("Plan Settings"))) {
        return;
    }
    ConfigDialog *dialog = new ConfigDialog(this, QStringLiteral("Plan Settings"), KPlatoSettings::self());
    connect(dialog, &ConfigDialog::settingsUpdated, this, &Part::slotSettingsUpdated, Qt::QueuedConnection);
    dialog->open();
}

void Part::slotSettingsUpdated()
{
//     new Help(KPlatoSettings::contextPath(), KPlatoSettings::contextLanguage());
}

bool Part::editProject()
{
    MainDocument *doc = qobject_cast<MainDocument*>(document());
    Q_ASSERT(doc);
    QPointer<MainProjectDialog> dia = new MainProjectDialog(*doc->project());
    int res = dia->exec();
    if (res == QDialog::Accepted) {
        MacroCommand *cmd = dia->buildCommand();
        if (cmd) {
            cmd->execute();
            delete cmd;
            document()->setModified(true);
        }
        doc->slotProjectCreated();
        finish();
    }
    dia->deleteLater();
    return res == QDialog::Accepted;
}

void Part::slotLoadSharedResources(const QString &file, const QUrl &projects, bool loadProjectsAtStartup)
{
    Q_UNUSED(projects)
    Q_UNUSED(loadProjectsAtStartup)
    MainDocument *doc = qobject_cast<MainDocument*>(document());
    Q_ASSERT(doc);
    QUrl url(file);
    if (url.scheme().isEmpty()) {
        url.setScheme(QStringLiteral("file"));
    }
    if (url.isValid()) {
        doc->insertResourcesFile(url);
    }
}

QWidget *Part::createWelcomeView(KoMainWindow *parent) const
{
    return new WelcomeView(parent);
}

void Part::addRecentURLToAllMainWindows()
{
    // Add to recent actions list in our mainWindows
    for (KoMainWindow *mainWindow : qAsConst(mainWindows())) {
        mainWindow->addRecentURL(document()->project()->name(), document()->url());
    }
}
