/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2000-2006 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007, 2009 Thomas zander <zander@kde.org>
   SPDX-FileCopyrightText: 2010 Benjamin Port <port.benjamin@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoMainWindow.h"

#include "KoView.h"
#include "KoDocument.h"
#include "KoFilterManager.h"
#include "KoDocumentInfo.h"
#include "KoDocumentInfoDlg.h"
#include "KoFileDialog.h"
#include "KoDockFactoryBase.h"
#include "KoDockWidgetTitleBar.h"
#include "KoPrintJob.h"
#include "KoDocumentEntry.h"
#include "KoPart.h"
#include "WelcomeView.h"
#include <KoPageLayoutDialog.h>
#include <KoPageLayout.h>
#include "KoApplication.h"
#include <KoIcon.h>
#include "KoResourcePaths.h"
#include "KoComponentData.h"
#include <config.h>
#include <KoDockRegistry.h>

#include <KRecentDirs>
#include <khelpmenu.h>
#include <KRecentFilesAction>
#include <KAboutData>
#include <KToggleAction>
#include <KMessageBox>
#include <KoNetAccess.h>
#include <kedittoolbar.h>
#include <QTemporaryFile>
#include <KRecentDocument>
#include <KLocalizedString>
#include <KXMLGUIFactory>
#include <KFileItem>
#include <KToolBar>
#include <KActionMenu>
#include <KActionCollection>
#include <KWindowConfig>
#include <KEMailClientLauncherJob>
#include <KDialogJobUiDelegate>

#ifdef HAVE_KACTIVITIES
#include <PlasmaActivities/ResourceInstance>
#endif

//   // qt includes
#include <QDockWidget>
#include <QApplication>
#include <QLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTabBar>
#include <QPrinter>
#include <QPrintDialog>
#include <QScreen>
#include <QPrintPreviewDialog>
#include <QCloseEvent>
#include <QPointer>
#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>
#include <QFontDatabase>
#include <QMimeDatabase>
#include <QStatusBar>
#include <QMenuBar>
#include <QDesktopServices>
#include <QScreen>

#include "MainDebug.h"

class KoMainWindowPrivate //NOLINT
{
public:
    KoMainWindowPrivate(const QByteArray &_nativeMimeType, const KoComponentData &componentData_, KoMainWindow *w)
        : componentData(componentData_)
    {
        nativeMimeType = _nativeMimeType;
        parent = w;
        rootDocument = nullptr;
        rootPart = nullptr;
        partToOpen = nullptr;
        mainWindowGuiIsBuilt = false;
        forQuit = false;
        activePart = nullptr;
        activeView = nullptr;
        firstTime = true;
        progress = nullptr;
        showDocumentInfo = nullptr;
        saveAction = nullptr;
        saveActionAs = nullptr;
        printAction = nullptr;
        printActionPreview = nullptr;
        sendFileAction = nullptr;
        exportPdf = nullptr;
        closeFile = nullptr;
        reloadFile = nullptr;
        importFile = nullptr;
        exportFile = nullptr;
        encryptDocument = nullptr;
#ifndef NDEBUG
        uncompressToDir = nullptr;
#endif
        isImporting = false;
        isExporting = false;
        windowSizeDirty = false;
        lastExportSpecialOutputFlag = 0;
        readOnly = false;
        dockWidgetMenu = nullptr;
        deferredClosingEvent = nullptr;
        blockClose = false;

        m_helpMenu = nullptr;

        // PartManager
        m_activeWidget = nullptr;
        m_activePart = nullptr;

        noCleanup = false;
        openingDocument = false;
        printPreviewJob = nullptr;

        tbIsVisible = true;
    }

    ~KoMainWindowPrivate() {
        qDeleteAll(toolbarList);
    }

    void applyDefaultSettings(QPrinter &printer) {
        QString title = rootDocument->documentInfo()->aboutInfo("title");
        if (title.isEmpty()) {
            title = rootDocument->url().fileName();
            // strip off the native extension (I don't want foobar.kwd.ps when printing into a file)
            QMimeType mime = QMimeDatabase().mimeTypeForName(QString::fromLatin1(rootDocument->outputMimeType()));
            if (mime.isValid()) {
                const QString extension = mime.preferredSuffix();

                if (title.endsWith(extension))
                    title.chop(extension.length());
            }
        }

        if (title.isEmpty()) {
            // #139905
            title = i18n("%1 unsaved document (%2)", parent->componentData().componentDisplayName(),
                         QLocale().toString(QDate::currentDate(), QLocale::ShortFormat));
        }
        printer.setDocName(title);
    }

    QByteArray nativeMimeType;

    KoMainWindow *parent;
    KoDocument *rootDocument;
    QList<KoView*> rootViews;

    // PartManager
    QPointer<KoPart> rootPart;
    QPointer<KoPart> partToOpen;
    QPointer<KoPart> activePart;
    QPointer<KoPart> m_activePart;
    QPointer<KoPart> m_registeredPart;

    KoView *activeView;
    QWidget *m_activeWidget;

    QPointer<QProgressBar> progress;
    QMutex progressMutex;

    QList<QAction *> toolbarList;

    bool mainWindowGuiIsBuilt;
    bool forQuit;
    bool firstTime;
    bool windowSizeDirty;
    bool readOnly;

    QAction *showDocumentInfo;
    QAction *saveAction;
    QAction *saveActionAs;
    QAction *printAction;
    QAction *printActionPreview;
    QAction *sendFileAction;
    QAction *exportPdf;
    QAction *closeFile;
    QAction *reloadFile;
    QAction *importFile;
    QAction *exportFile;
    QAction *encryptDocument;
#ifndef NDEBUG
    QAction *uncompressToDir;
#endif
    KToggleAction *toggleDockers;
    KToggleAction *toggleDockerTitleBars;
    KRecentFilesAction *recent;

    bool isImporting;
    bool isExporting;

    QUrl lastExportUrl;
    QByteArray lastExportedFormat;
    int lastExportSpecialOutputFlag;

    QMap<QString, QDockWidget *> dockWidgetsMap;
    KActionMenu *dockWidgetMenu;
    QMap<QDockWidget *, bool> dockWidgetVisibilityMap;
    QList<QDockWidget *> dockWidgets;
    QByteArray m_dockerStateBeforeHiding;

    QCloseEvent *deferredClosingEvent;
    bool blockClose;


    KoComponentData componentData;

    KHelpMenu *m_helpMenu;

    bool noCleanup;
    bool openingDocument;
    KoPrintJob *printPreviewJob;

    QAction *configureAction;

    bool tbIsVisible;

    QString recentFilesGroupName = QStringLiteral("RecentFiles");
};

KoMainWindow::KoMainWindow(const QByteArray &nativeMimeType, const KoComponentData &componentData)
    : KXmlGuiWindow()
    , d(new KoMainWindowPrivate(nativeMimeType, componentData, this))
{
#ifdef Q_OS_DARWIN
    setUnifiedTitleAndToolBarOnMac(true);
#endif
    setStandardToolBarMenuEnabled(true);

    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    connect(this, &KoMainWindow::restoringDone, this, &KoMainWindow::forceDockTabFonts);

    // PartManager
    // End

    QString xmlFileName = QStringLiteral("%1_shell.rc").arg(qApp->applicationName());
    setXMLFile(xmlFileName);

    actionCollection()->addAction(KStandardAction::New, QStringLiteral("file_new"), this, SLOT(slotFileNew()));
    actionCollection()->addAction(KStandardAction::Open, QStringLiteral("file_open"), this, SLOT(slotFileOpen()));
    d->recent = KStandardAction::openRecent(this, SLOT(slotFileOpenRecent(QUrl)), actionCollection());
    connect(d->recent, &KRecentFilesAction::recentListCleared, this, &KoMainWindow::saveRecentFiles);
    d->saveAction = actionCollection()->addAction(KStandardAction::Save,  QStringLiteral("file_save"), this, SLOT(slotFileSave()));
    d->saveActionAs = actionCollection()->addAction(KStandardAction::SaveAs,  QStringLiteral("file_save_as"), this, SLOT(slotFileSaveAs()));
    d->printAction = actionCollection()->addAction(KStandardAction::Print,  QStringLiteral("file_print"), this, SLOT(slotFilePrint()));
    d->printActionPreview = actionCollection()->addAction(KStandardAction::PrintPreview,  QStringLiteral("file_print_preview"), this, SLOT(slotFilePrintPreview()));

    d->exportPdf  = new QAction(i18n("Print to PDF..."), this);
    d->exportPdf->setIcon(koIcon("application-pdf"));
    actionCollection()->addAction(QStringLiteral("file_export_pdf"), d->exportPdf);
    connect(d->exportPdf, &QAction::triggered, this, static_cast<KoPrintJob* (KoMainWindow::*)(void)>(&KoMainWindow::exportToPdf));

    d->sendFileAction = actionCollection()->addAction(KStandardAction::Mail,  QStringLiteral("file_send_file"), this, SLOT(slotEmailFile()));

    d->closeFile = actionCollection()->addAction(KStandardAction::Close,  QStringLiteral("file_close"), this, SLOT(slotFileClose()));
    actionCollection()->addAction(KStandardAction::Quit,  QStringLiteral("file_quit"), this, SLOT(slotFileQuit()));

    d->reloadFile  = new QAction(i18n("Reload"), this);
    actionCollection()->addAction(QStringLiteral("file_reload_file"), d->reloadFile);
    connect(d->reloadFile, &QAction::triggered, this, &KoMainWindow::slotReloadFile);

    d->importFile  = new QAction(koIcon("document-import"), i18n("Import..."), this);
    actionCollection()->addAction(QStringLiteral("file_import_file"), d->importFile);
    connect(d->importFile, &QAction::triggered, this, &KoMainWindow::slotImportFile);

    d->exportFile  = new QAction(koIcon("document-export"), i18n("E&xport..."), this);
    actionCollection()->addAction(QStringLiteral("file_export_file"), d->exportFile);
    connect(d->exportFile, &QAction::triggered, this, &KoMainWindow::slotExportFile);

    d->encryptDocument = new QAction(i18n("En&crypt Document"), this);
    actionCollection()->addAction(QStringLiteral("file_encrypt_doc"), d->encryptDocument);
    connect(d->encryptDocument, SIGNAL(triggered(bool)), this, SLOT(slotEncryptDocument()));

#ifndef NDEBUG
    d->uncompressToDir = new QAction(i18n("&Uncompress to Directory"), this);
    actionCollection()->addAction(QStringLiteral("file_uncompress_doc"), d->uncompressToDir);
    connect(d->uncompressToDir, SIGNAL(triggered(bool)), this, SLOT(slotUncompressToDir()));
#endif

    QAction *actionNewView  = new QAction(koIcon("window-new"), i18n("&New View"), this);
    actionNewView->setMenuRole(QAction::NoRole);
    actionCollection()->addAction(QStringLiteral("view_newview"), actionNewView);
    connect(actionNewView, &QAction::triggered, this, &KoMainWindow::newView);

    /* The following entry opens the document information dialog.  Since the action is named so it
        intends to show data this entry should not have a trailing ellipses (...).  */
    d->showDocumentInfo  = new QAction(koIcon("document-properties"), i18n("Document Information"), this);
    actionCollection()->addAction(QStringLiteral("file_documentinfo"), d->showDocumentInfo);
    connect(d->showDocumentInfo, &QAction::triggered, this, &KoMainWindow::slotDocumentInfo);

    KStandardAction::keyBindings(this, SLOT(slotConfigureKeys()), actionCollection());
    auto a = KStandardAction::configureToolbars(this, SLOT(slotConfigureToolbars()), actionCollection());
    a->setEnabled(false);

    actionCollection()->action(QStringLiteral("file_new"))->setEnabled(false);
    d->showDocumentInfo->setEnabled(false);
    d->saveActionAs->setEnabled(false);
    d->reloadFile->setEnabled(false);
    d->importFile->setEnabled(false);
    d->exportFile->setEnabled(false);
    d->saveAction->setEnabled(false);
    d->printAction->setEnabled(false);
    d->printActionPreview->setEnabled(false);
    d->sendFileAction->setEnabled(false);
    d->exportPdf->setEnabled(false);
    d->closeFile->setEnabled(false);
#if 0
    d->encryptDocument->setEnabled(false);
#ifndef NDEBUG
    d->uncompressToDir->setEnabled(false);
#endif
#endif
    KToggleAction *fullscreenAction  = new KToggleAction(koIcon("view-fullscreen"), i18n("Full Screen Mode"), this);
    actionCollection()->addAction(QStringLiteral("view_fullscreen"), fullscreenAction);
    actionCollection()->setDefaultShortcut(fullscreenAction, QKeySequence::FullScreen);
    connect(fullscreenAction, &QAction::toggled, this, &KoMainWindow::viewFullscreen);

    d->toggleDockers = new KToggleAction(i18n("Show Dockers"), this);
    d->toggleDockers->setChecked(true);
    actionCollection()->addAction(QStringLiteral("view_toggledockers"), d->toggleDockers);
    connect(d->toggleDockers, &QAction::toggled, this, &KoMainWindow::toggleDockersVisibility);

    d->toggleDockerTitleBars = new KToggleAction(i18nc("@action:inmenu", "Show Docker Titlebars"), this);
    KConfigGroup configGroupInterface =  KSharedConfig::openConfig()->group("Interface");
    d->toggleDockerTitleBars->setChecked(configGroupInterface.readEntry("ShowDockerTitleBars", true));
    d->toggleDockerTitleBars->setVisible(false);
    actionCollection()->addAction(QStringLiteral("view_toggledockertitlebars"), d->toggleDockerTitleBars);
    connect(d->toggleDockerTitleBars, &QAction::toggled, this, &KoMainWindow::showDockerTitleBars);

    d->dockWidgetMenu  = new KActionMenu(i18n("Dockers"), this);
    actionCollection()->addAction(QStringLiteral("settings_dockers_menu"), d->dockWidgetMenu);
    d->dockWidgetMenu->setVisible(false);
    d->dockWidgetMenu->setPopupMode(QToolButton::MenuButtonPopup);

    createMainwindowGUI();
    d->mainWindowGuiIsBuilt = true;

    // we first figure out some good default size and restore the x,y position. See bug 285804Z.
    KConfigGroup cfg(KSharedConfig::openConfig(), "MainWindow");
    QByteArray geom = QByteArray::fromBase64(cfg.readEntry("ko_geometry", QByteArray()));
    if (!restoreGeometry(geom)) {
        // TODO: Handle multiple monitors
        const auto screen = qApp->primaryScreen();
        QRect desk = screen->geometry();

        quint32 x = desk.x();
        quint32 y = desk.y();
        quint32 w = 0;
        quint32 h = 0;

        // Default size -- maximize on small screens, something useful on big screens
        const int deskWidth = desk.width();
        if (deskWidth > 1024) {
            // a nice width, and slightly less than total available
            // height to componensate for the window decs
            w = (deskWidth / 3) * 2;
            h = (desk.height() / 3) * 2;
        }
        else {
            w = desk.width();
            h = desk.height();
        }

        x += (desk.width() - w) / 2;
        y += (desk.height() - h) / 2;

        move(x,y);
        setGeometry(geometry().x(), geometry().y(), w, h);
    }
    restoreState(QByteArray::fromBase64(cfg.readEntry("ko_windowstate", QByteArray())));
}

void KoMainWindow::setNoCleanup(bool noCleanup)
{
    d->noCleanup = noCleanup;
}

KoMainWindow::~KoMainWindow()
{
    KConfigGroup cfg(KSharedConfig::openConfig(), "MainWindow");
    cfg.writeEntry("ko_geometry", saveGeometry().toBase64());
    cfg.writeEntry("ko_windowstate", saveState().toBase64());

    // The doc and view might still exist (this is the case when closing the window)
    if (d->rootPart)
        d->rootPart->removeMainWindow(this);

    if (d->partToOpen) {
        d->partToOpen->removeMainWindow(this);
        delete d->partToOpen;
    }

    // safety first ;)
    setActivePart(nullptr, nullptr);

    if (d->rootViews.indexOf(d->activeView) == -1) {
        delete d->activeView;
        d->activeView = nullptr;
    }
    while (!d->rootViews.isEmpty()) {
        delete d->rootViews.takeFirst();
    }

    if(d->noCleanup)
        return;
    // We have to check if this was a root document.
    // This has to be checked from queryClose, too :)
    if (d->rootPart && d->rootPart->viewCount() == 0) {
        //debugMain <<"Destructor. No more views, deleting old doc" << d->rootDocument;
        delete d->rootDocument; // FIXME: Why delete here and not only in KoPart?
    }

    delete d;
}

void KoMainWindow::setRootDocument(KoDocument *doc, KoPart *part, bool deletePrevious)
{
    if (d->rootDocument == doc)
        return;

    if (d->partToOpen && d->partToOpen->document() != doc) {
        d->partToOpen->removeMainWindow(this);
        if (deletePrevious) {
            delete d->partToOpen;
        }
    }
    d->partToOpen = nullptr;

    //debugMain <<"KoMainWindow::setRootDocument this =" << this <<" doc =" << doc;
    QList<KoView*> oldRootViews = d->rootViews;
    d->rootViews.clear();
    KoDocument *oldRootDoc = d->rootDocument;
    KoPart *oldRootPart = d->rootPart;
    // De-activate gui or else menus may be messed up
    for (auto v : oldRootViews) {
        v->guiActivateEvent(false);
    }
    if (oldRootDoc) {
        oldRootDoc->disconnect(this);
        oldRootPart->removeMainWindow(this);

        // Hide all dockwidgets and remember their old state
        d->dockWidgetVisibilityMap.clear();

        for (QDockWidget* dockWidget : qAsConst(d->dockWidgetsMap)) {
            d->dockWidgetVisibilityMap.insert(dockWidget, dockWidget->isVisible());
            dockWidget->setVisible(false);
        }

        d->toggleDockerTitleBars->setVisible(false);
        d->dockWidgetMenu->setVisible(false);
    }

    d->rootDocument = doc;
    // XXX remove this after the splitting
    if (!part && doc) {
        d->rootPart = doc->documentPart();
    }
    else {
        d->rootPart = part;
    }

    if (doc) {
        d->toggleDockerTitleBars->setVisible(true);
        d->dockWidgetMenu->setVisible(true);
        d->m_registeredPart = d->rootPart.data();

        KoView *view = d->rootPart->createView(doc, this);
        setCentralWidget(view);
        d->rootViews.append(view);

        view->show();
        view->setFocus();

        // The addMainWindow has been done already if using openUrl
        if (!d->rootPart->mainWindows().contains(this)) {
            d->rootPart->addMainWindow(this);
        }
    }

    bool enable = d->rootDocument != nullptr ? true : false;
    actionCollection()->action(QStringLiteral("file_new"))->setEnabled(true);
    d->showDocumentInfo->setEnabled(enable);
    d->saveAction->setEnabled(enable);
    d->saveActionAs->setEnabled(enable);
    d->importFile->setEnabled(enable);
    d->importFile->setEnabled(enable);
    d->exportFile->setEnabled(enable);
#if 0
    d->encryptDocument->setEnabled(enable);
#ifndef NDEBUG
    d->uncompressToDir->setEnabled(enable);
#endif
#endif
    d->printAction->setEnabled(enable);
    d->printActionPreview->setEnabled(enable);
    d->sendFileAction->setEnabled(enable);
    d->exportPdf->setEnabled(enable);
    d->closeFile->setEnabled(enable);
    updateCaption();

    setActivePart(d->rootPart, doc ? d->rootViews.first() : nullptr);
    if (d->rootPart) {
        reloadRecentFileList();
    }
    Q_EMIT restoringDone();

    while(!oldRootViews.isEmpty()) {
        delete oldRootViews.takeFirst();
    }
    if (oldRootPart && oldRootPart->viewCount() == 0) {
        //debugMain <<"No more views, deleting old doc" << oldRootDoc;
        oldRootDoc->clearUndoHistory();
        if(deletePrevious)
            delete oldRootDoc;
    }

    if (doc && !d->dockWidgetVisibilityMap.isEmpty()) {
        for (QDockWidget* dockWidget : qAsConst(d->dockWidgetsMap)) {
            dockWidget->setVisible(d->dockWidgetVisibilityMap.value(dockWidget));
        }
    }

    if (!d->rootDocument) {
        statusBar()->setVisible(false);
    }
    else {
#ifdef Q_OS_DARWIN
        statusBar()->setMaximumHeight(28);
#endif
        connect(d->rootDocument, &KoDocument::titleModified, this, &KoMainWindow::slotDocumentTitleModified);
    }
}

void KoMainWindow::updateReloadFileAction(KoDocument *doc)
{
    d->reloadFile->setEnabled(doc && !doc->url().isEmpty());
}

void KoMainWindow::setReadWrite(bool readwrite)
{
    d->saveAction->setEnabled(readwrite);
    d->importFile->setEnabled(readwrite);
    d->readOnly =  !readwrite;
    updateCaption();
}

void KoMainWindow::setRecentFilesGroupName(const QString &name)
{
    d->recentFilesGroupName = name;
}

QString KoMainWindow::recentFilesGroupName() const
{
    return d->recentFilesGroupName;
}

void KoMainWindow::addRecentURL(const QString &projectName, const QUrl &url)
{
    debugMain << "url=" << url.toDisplayString();
    // Add entry to recent documents list
    if (!url.isEmpty()) {
        bool ok = true;
        if (url.isLocalFile()) {
            QString path = url.adjusted(QUrl::StripTrailingSlash).toLocalFile();
            const QStringList tmpDirs = QStandardPaths::standardLocations(QStandardPaths::TempLocation);
            for (const QString &tmpDir : tmpDirs) {
                if (path.startsWith(tmpDir)) {
                    ok = false; // it's in the tmp resource
                    break;
                }
            }
            if (ok) {
                KRecentDocument::add(QUrl::fromLocalFile(path));
                KRecentDirs::add(QStringLiteral(":OpenDialog"), QFileInfo(path).dir().canonicalPath());
            }
        } else {
            KRecentDocument::add(url.adjusted(QUrl::StripTrailingSlash));
        }
        if (ok) {
            d->recent->addUrl(url, projectName);
        }
        saveRecentFiles();

#ifdef HAVE_KACTIVITIES
        KActivities::ResourceInstance::notifyAccessed(url);
#endif
    }
}

void KoMainWindow::saveRecentFiles()
{
    const QString group = recentFilesGroupName();
    // Save list of recent files
    KSharedConfigPtr config = componentData().config();
    debugMain << this << " Saving recent files list into config. componentData()=" << componentData().componentName();
    d->recent->saveEntries(config->group(group));
    config->sync();

    // Tell all windows to reload their list, after saving
    // Doesn't work multi-process, but it's a start
    const auto windows = KMainWindow::memberList();
    for (KMainWindow* window : windows) {
        static_cast<KoMainWindow *>(window)->reloadRecentFileList();
    }
}

void KoMainWindow::reloadRecentFileList()
{
    const QString group = recentFilesGroupName();
    KSharedConfigPtr config = componentData().config();
    d->recent->loadEntries(config->group(group));
}

bool KoMainWindow::isDocumentModified()
{
    return d->rootDocument ? d->rootDocument->isModified() : false;
}

KoPart* KoMainWindow::createPart() const
{
    KoDocumentEntry entry = KoDocumentEntry::queryByMimeType(QString::fromLatin1(d->nativeMimeType));
    QString errorMsg;
    KoPart *part = entry.createKoPart(&errorMsg);

    if (!part || !errorMsg.isEmpty()) {
        return nullptr;
    }
    return part;
}

void KoMainWindow::updateCaption()
{
    debugMain;
    if (!d->rootDocument) {
        updateCaption(QString(), false);
    }
    else {
        QString caption(d->rootDocument->caption());
        if (d->readOnly) {
            caption += QLatin1Char(' ') + i18n("(write protected)");
        }

        updateCaption(caption, isDocumentModified());
        if (!rootDocument()->url().fileName().isEmpty())
            d->saveAction->setToolTip(i18n("Save as %1", d->rootDocument->url().fileName()));
        else
            d->saveAction->setToolTip(i18n("Save"));
    }
}

void KoMainWindow::updateCaption(const QString & caption, bool mod)
{
    debugMain  << caption << "," << mod;
#ifdef PLAN_ALPHA
    setCaption(QStringLiteral("ALPHA %1: %2").arg(QStringLiteral(PLAN_ALPHA)).arg(caption), mod);
    return;
#endif
#ifdef PLAN_BETA
    setCaption(QStringLiteral("BETA %1: %2").arg(QStringLiteral(PLAN_BETA)).arg(caption), mod);
    return;
#endif
#ifdef PLAN_RC
    setCaption(QStringLiteral("RELEASE CANDIDATE %1: %2").arg(QStringLiteral(PLAN_RC)).arg(caption), mod);
    return;
#endif

    setCaption(caption, mod);
}

KoDocument *KoMainWindow::rootDocument() const
{
    return d->rootDocument;
}

KoView *KoMainWindow::rootView() const
{
    if (d->rootViews.indexOf(d->activeView) != -1)
        return d->activeView;
    return d->rootViews.first();
}

bool KoMainWindow::openDocument(const QUrl &url)
{
    if (url.fileName().endsWith(QStringLiteral(".plant"))) {
        KMessageBox::error(nullptr, xi18nc("@info", "Cannot open a template file.<nl/>"
                                    "If you want to modify the template, create a new project using this template"
                                    " and save it using <interface>File->Create Project Template...</interface>."));
        return false;
    }
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, nullptr)) {
        KMessageBox::error(nullptr, i18n("The file %1 does not exist.", url.url()));
        d->recent->removeUrl(url); //remove the file from the recent-opened-file-list
        saveRecentFiles();
        return false;
    }
    if (d->rootDocument && !d->rootDocument->isEmpty()) {
        // newdoc must be opened in a new mainwindow
        auto newpart = createPart();
        if (!newpart) {
            return false;
        }
        KoMainWindow *s = newpart->createMainWindow();
        newpart->addMainWindow(s);
        s->show();
        return s->openDocument(newpart, url);
    }
    return openDocumentInternal(url);
}

bool KoMainWindow::openDocument(KoPart *newPart, const QUrl &url)
{
    if (url.fileName().endsWith(QStringLiteral(".plant"))) {
        KMessageBox::error(nullptr, xi18nc("@info", "Cannot open a template file.<nl/>"
                                    "If you want to modify the template, create a new project using this template"
                                    " and save it using <interface>File->Create Project Template...</interface>."));
        return false;
    }
    if (!newPart) {
        return openDocument(url);
    }
    // the part always has a document; the document doesn't know about the part.
    KoDocument *newdoc = newPart->document();
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, nullptr)) {
        newdoc->initEmpty(); //create an empty document
        setRootDocument(newdoc, newPart);
        newdoc->setUrl(url);
        QMimeType mime = QMimeDatabase().mimeTypeForUrl(url);
        QString mimetype = (!mime.isValid() || mime.isDefault()) ? QLatin1String(newdoc->nativeFormatMimeType()) : mime.name();
        newdoc->setMimeTypeAfterLoading(mimetype);
        updateCaption();
        return true;
    }
    return openDocumentInternal(url, newPart, newdoc);
}

bool KoMainWindow::openDocumentInternal(const QUrl &url, KoPart *newpart, KoDocument *newdoc)
{
    debugMain << newpart << newdoc << url.url();
    bool keepPart = true;
    if (!newpart) {
        newpart = createPart();
        keepPart = false;
    }
    if (!newpart)
        return false;

    if (!newdoc)
        newdoc = newpart->document();

    KFileItem file(url, QLatin1String(newdoc->mimeType()), KFileItem::Unknown);
    if (!file.isWritable()) {
        newdoc->setReadWrite(false);
    }

    d->firstTime = true;
    connect(newdoc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    connect(newdoc, &KoDocument::completed, this, &KoMainWindow::slotLoadCompleted);
    connect(newdoc, &KoDocument::canceled, this, &KoMainWindow::slotLoadCanceled);
    d->openingDocument = true;
    newpart->addMainWindow(this);   // used by openUrl
    bool openRet = (!isImporting()) ? newdoc->openUrl(url) : newdoc->importDocument(url);
    if (!openRet) {
        if (!keepPart) {
            newpart->removeMainWindow(this);
            delete newdoc;
            delete newpart;
            newpart = nullptr;
            d->openingDocument = false;
        }
        return false;
    }
    updateReloadFileAction(newdoc);
    // Delete welcomeview, it does not have part and document
    // so must be explicitly deleted when a document is opened
    WelcomeView* w = findChild<WelcomeView*>();
    if (w) {
        w->deleteLater();
    }
    return true;
}

// Separate from openDocument to handle async loading (remote URLs)
void KoMainWindow::slotLoadCompleted()
{
    debugMain;
    KoDocument *newdoc = qobject_cast<KoDocument*>(sender());
    KoPart *newpart = newdoc->documentPart();

    newpart->addRecentURLToAllMainWindows();

    if (d->rootDocument && d->rootDocument->isEmpty()) {
        // Replace current empty document
        setRootDocument(newdoc);
    } else if (d->rootDocument && !d->rootDocument->isEmpty()) {
        // Open in a new main window
        // (Note : could create the main window first and the doc next for this
        // particular case, that would give a better user feedback...)
        KoMainWindow *s = newpart->createMainWindow();
        s->show();
        newpart->removeMainWindow(this);
        s->setRootDocument(newdoc, newpart);
    } else {
        // We had no document, set the new one
        setRootDocument(newdoc);
    }
    slotProgress(-1);
    disconnect(newdoc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    disconnect(newdoc, &KoDocument::completed, this, &KoMainWindow::slotLoadCompleted);
    disconnect(newdoc, &KoDocument::canceled, this, &KoMainWindow::slotLoadCanceled);
    d->openingDocument = false;
    Q_EMIT loadCompleted();
}

void KoMainWindow::slotLoadCanceled(const QString & errMsg)
{
    debugMain;
    KoDocument* doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    disconnect(doc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    disconnect(doc, &KoDocument::completed, this, &KoMainWindow::slotLoadCompleted);
    disconnect(doc, &KoDocument::canceled, this, &KoMainWindow::slotLoadCanceled);
    d->openingDocument = false;
    if (!errMsg.isEmpty()) {  // empty when canceled by user
        KMessageBox::error(this, errMsg);
    }
    // open welcome view
    auto part = doc->documentPart();
    Q_ASSERT(part);
    auto w = part->createWelcomeView(this);
    if (w) {
        setPartToOpen(part);
        setCentralWidget(w);
    }
    Q_EMIT loadCanceled();
}

void KoMainWindow::slotSaveCanceled(const QString &errMsg)
{
    debugMain;
    if (!errMsg.isEmpty())   // empty when canceled by user
        KMessageBox::error(this, errMsg);
    slotSaveCompleted();
}

void KoMainWindow::slotSaveCompleted()
{
    debugMain;
    KoDocument* doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    disconnect(doc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    disconnect(doc, &KoDocument::completed, this, &KoMainWindow::slotSaveCompleted);
    disconnect(doc, &KoDocument::canceled, this, &KoMainWindow::slotSaveCanceled);

    if (d->deferredClosingEvent) {
        KXmlGuiWindow::closeEvent(d->deferredClosingEvent);
    }
}

// returns true if we should save, false otherwise.
bool KoMainWindow::exportConfirmation(const QByteArray &outputFormat)
{
    KConfigGroup group =  KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
    if (!group.readEntry("WantExportConfirmation", true)) {
        return true;
    }

    QMimeType mime = QMimeDatabase().mimeTypeForName(QString::fromLatin1(outputFormat));
    QString comment = mime.isValid() ? mime.comment() : i18n("%1 (unknown file type)", QString::fromLatin1(outputFormat));

    // Warn the user
    int ret;
    if (!isExporting()) { // File --> Save
        ret = KMessageBox::warningContinueCancel
                (
                    this,
                    i18n("<qt>Saving as a %1 may result in some loss of formatting."
                         "<p>Do you still want to save in this format?</qt>",
                         QStringLiteral("<b>%1</b>").arg(comment)),      // in case we want to remove the bold later
                    i18n("Confirm Save"),
                    KStandardGuiItem::save(),
                    KStandardGuiItem::cancel(),
                    QStringLiteral("NonNativeSaveConfirmation")
                    );
    } else { // File --> Export
        ret = KMessageBox::warningContinueCancel
                (
                    this,
                    i18n("<qt>Exporting as a %1 may result in some loss of formatting."
                         "<p>Do you still want to export to this format?</qt>",
                         QStringLiteral("<b>%1</b>").arg(comment)),      // in case we want to remove the bold later
                    i18n("Confirm Export"),
                    KGuiItem(i18n("Export")),
                    KStandardGuiItem::cancel(),
                    QStringLiteral("NonNativeExportConfirmation") // different to the one used for Save (above)
                    );
    }

    return (ret == KMessageBox::Continue);
}

bool KoMainWindow::saveDocument(bool saveas, bool silent, int specialOutputFlag)
{
    return saveDocumentInternal(saveas, silent, specialOutputFlag);
}

bool KoMainWindow::saveDocumentInternal(bool saveas, bool silent, int specialOutputFlag)
{
    if (!d->rootDocument || !d->rootPart) {
        warnMain<<"Failed to save, no root document";
        return true;
    }

    bool reset_url;

    if (d->rootDocument->url().isEmpty()) {
        Q_EMIT saveDialogShown();
        reset_url = true;
        saveas = true;
    } else {
        reset_url = false;
    }

    connect(d->rootDocument, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    connect(d->rootDocument, &KoDocument::completed, this, &KoMainWindow::slotSaveCompleted);
    connect(d->rootDocument, &KoDocument::canceled, this, &KoMainWindow::slotSaveCanceled);

    QUrl oldURL = d->rootDocument->url();
    QString oldFile = d->rootDocument->localFilePath();

    QByteArray _native_format = d->rootDocument->nativeFormatMimeType();
    QByteArray oldOutputFormat = d->rootDocument->outputMimeType();

    int oldSpecialOutputFlag = d->rootDocument->specialOutputFlag();

    QUrl suggestedURL = d->rootDocument->url();

    QStringList mimeFilter;
    QMimeType mime = QMimeDatabase().mimeTypeForName(QString::fromLatin1(_native_format));
    if (!mime.isValid())
        // QT5TODO: find if there is no better way to get an object for the default type
        mime = QMimeDatabase().mimeTypeForName(QStringLiteral("application/octet-stream"));
    if (specialOutputFlag == 0) {
        mimeFilter = KoFilterManager::mimeFilter(_native_format, KoFilterManager::Export, d->rootDocument->extraNativeMimeTypes());
    } else if (specialOutputFlag & KoDocumentBase::SaveEncrypted) {
        // we don't support exporting encrypted files
        mimeFilter << QLatin1String(_native_format);
    } else {
        // FIXME: This should be mimetypes not patterns, or?
        mimeFilter = mime.globPatterns();
    }
    debugMain << d->rootDocument->url() << "old format" << oldOutputFormat<< "special output flag" << specialOutputFlag << "mime filter" << mimeFilter<< "saveas" << saveas;
    if (oldOutputFormat.isEmpty() && !d->rootDocument->url().isEmpty()) {
        // Not been saved yet, but there is a default url so open dialog with this url
        if (suggestedURL.path() == suggestedURL.fileName()) {
            // only a filename has been given, so add the default dir
            KConfigGroup group =  KSharedConfig::openConfig()->group("File Dialogs");
            QString path = group.readEntry("SaveDocument");
            path += QLatin1Char('/') + suggestedURL.fileName();
            suggestedURL.setPath(path);
            suggestedURL.setScheme(QStringLiteral("file"));
        }
        saveas = true;
        debugMain << "newly created doc, default file name:" << d->rootDocument->url() << "save to:" << suggestedURL;
    } else if (!mimeFilter.contains(QLatin1String(oldOutputFormat)) && !isExporting()) {
        debugMain << "no export filter for" << oldOutputFormat;

        // --- don't setOutputMimeType in case the user cancels the Save As
        // dialog and then tries to just plain Save ---

        // suggest a different filename extension (yes, we fortunately don't all live in a world of magic :))
        QString suggestedFilename = suggestedURL.fileName();
        if (!suggestedFilename.isEmpty()) {  // ".kra" looks strange for a name
            int c = suggestedFilename.lastIndexOf(QLatin1Char('.'));

            QString ext = mime.preferredSuffix();
            if (!ext.isEmpty()) {
                ext.prepend(QLatin1Char('.'));
                if (c < 0)
                    suggestedFilename += ext;
                else
                    suggestedFilename = suggestedFilename.left(c) + ext;
            } else { // current filename extension wrong anyway
                if (c > 0) {
                    // this assumes that a . signifies an extension, not just a .
                    suggestedFilename = suggestedFilename.left(c);
                }
            }

            suggestedURL = suggestedURL.adjusted(QUrl::RemoveFilename);
            suggestedURL.setPath(suggestedURL.path() + suggestedFilename);
        }

        // force the user to choose outputMimeType
        saveas = true;
    }

    bool ret = false;

    if (saveas || d->rootDocument->url().isEmpty()) {
        debugMain<<"save as:"<<"doc:"<<d->rootDocument->url()<<"suggested:"<<suggestedURL<<"export:"<<isExporting()<<d->lastExportUrl;

        if (isExporting() && !d->lastExportUrl.isEmpty()) {
            suggestedURL = d->lastExportUrl;
        }

        const auto localfile = suggestedURL.toLocalFile();
        const bool overrideDefaultDir = !suggestedURL.adjusted(QUrl::RemoveFilename).isEmpty();
        debugMain<<localfile<<overrideDefaultDir;
        KoFileDialog dialog(this, KoFileDialog::SaveFile, QStringLiteral("SaveDocument"));
        dialog.setCaption(i18n("untitled"));
        dialog.setDefaultDir(localfile, overrideDefaultDir);
        dialog.setMimeTypeFilters(mimeFilter);
        QUrl newURL = QUrl::fromUserInput(dialog.filename());
        debugMain<<newURL;
        ret = !newURL.isEmpty();
        if (ret) {
            QByteArray outputFormat = _native_format;

            if (!specialOutputFlag) {
                QMimeType mime = QMimeDatabase().mimeTypeForUrl(newURL);
                outputFormat = mime.name().toLatin1();
            }

            // if just File/Save As'ing to change filter options you
            // don't want to be reminded about overwriting files etc.
            bool justChangingFilterOptions = false;
            if (!isExporting()) {
                justChangingFilterOptions = (newURL == d->rootDocument->url()) &&
                        (outputFormat == d->rootDocument->mimeType()) &&
                        (specialOutputFlag == oldSpecialOutputFlag);
            } else {
                justChangingFilterOptions = (newURL == d->lastExportUrl) &&
                        (outputFormat == d->lastExportedFormat) &&
                        (specialOutputFlag == d->lastExportSpecialOutputFlag);
            }
            bool wantToSave = true;

            // don't change this line unless you know what you're doing :)
            if (!justChangingFilterOptions || d->rootDocument->confirmNonNativeSave(isExporting())) {
                if (!d->rootDocument->isNativeFormat(outputFormat)) {
                    wantToSave = exportConfirmation(outputFormat);
                }
            }

            if (wantToSave) {
                //
                // Note:
                // If the user is stupid enough to Export to the current URL,
                // we do _not_ change this operation into a Save As.  Reasons
                // follow:
                //
                // 1. A check like "isExporting() && oldURL == newURL"
                //    doesn't _always_ work on case-insensitive filesystems
                //    and inconsistent behaviour is bad.
                // 2. It is probably not a good idea to change d->rootDocument->mimeType
                //    and friends because the next time the user File/Save's,
                //    (not Save As) they won't be expecting that they are
                //    using their File/Export settings
                //
                // As a bad side-effect of this, the modified flag will not
                // be updated and it is possible that what is currently on
                // their screen is not what is stored on disk (through loss
                // of formatting).  But if you are dumb enough to change
                // mimetype but not the filename, then arguably, _you_ are
                // the "bug" :)
                //
                // - Clarence
                //


                d->rootDocument->setOutputMimeType(outputFormat, specialOutputFlag);
                if (!isExporting()) {  // Save As
                    ret = d->rootDocument->saveAs(newURL);

                    if (ret) {
                        debugMain << "Successful Save As!";
                        addRecentURL(d->rootDocument->projectName(), newURL);
                        setReadWrite(true);
                    } else {
                        warnMain << "Failed Save As!";
                        d->rootDocument->setUrl(oldURL);
                        d->rootDocument->setLocalFilePath(oldFile);
                        d->rootDocument->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                    }
                } else { // Export
                    ret = d->rootDocument->exportDocument(newURL);

                    if (ret) {
                        // a few file dialog convenience things
                        d->lastExportUrl = newURL;
                        d->lastExportedFormat = outputFormat;
                        d->lastExportSpecialOutputFlag = specialOutputFlag;
                    }

                    // always restore output format
                    d->rootDocument->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                }

                if (silent) // don't let the document change the window caption
                    d->rootDocument->setTitleModified();
            }   // if (wantToSave)  {
            else {
                ret = false;
            }
        }
    } else { // saving

        bool needConfirm = d->rootDocument->confirmNonNativeSave(false) && !d->rootDocument->isNativeFormat(oldOutputFormat);

        if (!needConfirm || (needConfirm && exportConfirmation(oldOutputFormat /* not so old :) */))) {
            // be sure d->rootDocument has the correct outputMimeType!
            if (isExporting() || d->rootDocument->isModified() || d->rootDocument->alwaysAllowSaving()) {
                ret = d->rootDocument->save();
            }

            if (!ret) {
                warnMain << d->rootDocument << "Failed Save!";
                d->rootDocument->setUrl(oldURL);
                d->rootDocument->setLocalFilePath(oldFile);
            }
        } else {
            ret = false;
        }
    }

    if (!ret && reset_url) {
        d->rootDocument->resetURL(); //clean the suggested filename as the save dialog was rejected
    }
    updateReloadFileAction(d->rootDocument);
    updateCaption();

    return ret;
}

void KoMainWindow::closeEvent(QCloseEvent *e)
{
    // If we are in the process of opening a new document, rootDocument() may not have been set yet,
    // so we must prevent closing to avoid crash.
    if(d->blockClose || d->openingDocument || (rootDocument() && rootDocument()->isLoading())) {
        e->setAccepted(false);
        return;
    }
    if (queryClose()) {
        d->deferredClosingEvent = e;
        if (!d->m_dockerStateBeforeHiding.isEmpty()) {
            restoreState(d->m_dockerStateBeforeHiding);
        }
        statusBar()->setVisible(true);
        menuBar()->setVisible(true);

        saveWindowSettings();
        if(d->noCleanup)
            return;
        setRootDocument(nullptr);
        if (!d->dockWidgetVisibilityMap.isEmpty()) { // re-enable dockers for persistency
            for (QDockWidget* dockWidget : qAsConst(d->dockWidgetsMap))
                dockWidget->setVisible(d->dockWidgetVisibilityMap.value(dockWidget));
        }
    } else {
        e->setAccepted(false);
    }
}

void KoMainWindow::saveWindowSettings()
{
    KSharedConfigPtr config = componentData().config();

    if (d->windowSizeDirty) {

        // Save window size into the config file of our componentData
        // TODO: check if this is ever read again, seems lost over the years
        debugMain;
        KConfigGroup mainWindowConfigGroup = config->group("MainWindow");
        KWindowConfig::saveWindowSize(windowHandle(), mainWindowConfigGroup);
        config->sync();
        d->windowSizeDirty = false;
    }

    if (rootDocument() && d->rootPart) {

        // Save toolbar position into the config file of the app, under the doc's component name
        KConfigGroup group =  KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
        //debugMain <<"KoMainWindow::closeEvent -> saveMainWindowSettings rootdoc's componentData=" << d->rootPart->componentData().componentName();
        saveMainWindowSettings(group);

        // Save collapsable state of dock widgets
        for (QMap<QString, QDockWidget*>::const_iterator i = d->dockWidgetsMap.constBegin();
             i != d->dockWidgetsMap.constEnd(); ++i) {
            if (i.value()->widget()) {
                KConfigGroup dockGroup = group.group(QStringLiteral("DockWidget ") + i.key());
                dockGroup.writeEntry("Collapsed", i.value()->widget()->isHidden());
                dockGroup.writeEntry("Locked", i.value()->property("Locked").toBool());
                dockGroup.writeEntry("DockArea", (int) dockWidgetArea(i.value()));
            }
        }

    }

     KSharedConfig::openConfig()->sync();
    resetAutoSaveSettings(); // Don't let KMainWindow override the good stuff we wrote down

}

void KoMainWindow::resizeEvent(QResizeEvent * e)
{
    d->windowSizeDirty = true;
    KXmlGuiWindow::resizeEvent(e);
}

bool KoMainWindow::queryClose()
{
    if (rootDocument() == nullptr)
        return true;
    //debugMain <<"KoMainWindow::queryClose() viewcount=" << rootDocument()->viewCount()
    //               << " mainWindowCount=" << rootDocument()->mainWindowCount() << '\n';
    if (!d->forQuit && d->rootPart && d->rootPart->mainwindowCount() > 1)
        // there are more open, and we are closing just one, so no problem for closing
        return true;

    // main doc + internally stored child documents
    if (isDocumentModified()) {
        QString name;
        if (rootDocument()->documentInfo()) {
            name = rootDocument()->documentInfo()->aboutInfo("title");
        }
        if (name.isEmpty())
            name = rootDocument()->url().fileName();

        if (name.isEmpty())
            name = i18n("Untitled");

        int res = KMessageBox::warningTwoActionsCancel(this,
                                                  i18n("<p>The document <b>'%1'</b> has been modified.</p><p>Do you want to save it?</p>", name),
                                                  QString(),
                                                  KStandardGuiItem::save(),
                                                  KStandardGuiItem::discard(),
                                                  KStandardGuiItem::cancel());

        switch (res) {
        case KMessageBox::PrimaryAction : {
            bool isNative = (d->rootDocument->outputMimeType() == d->rootDocument->nativeFormatMimeType());
            if (!saveDocument(!isNative))
                return false;
            break;
        }
        case KMessageBox::SecondaryAction :
            rootDocument()->removeAutoSaveFiles();
            rootDocument()->setModified(false);   // Now when queryClose() is called by closeEvent it won't do anything.
            break;
        default : // case KMessageBox::Cancel :
            return false;
        }
    }

    return true;
}

// Helper method for slotFileNew and slotFileClose
KoPart *KoMainWindow::chooseNewDocument(InitDocFlags init)
{
    KoDocument* doc = rootDocument();
    KoPart *newpart = koApp->getPart(koApp->applicationName(), QString::fromLatin1(d->nativeMimeType));
    KoDocument *newdoc = newpart->document();
    if (!newdoc) {
        delete newpart;
        return nullptr;
    }
    disconnect(newdoc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);

    if ((!doc && init == InitOnFileNew) || (doc && !doc->isEmpty())) {
        KoMainWindow *s = newpart->createMainWindow();
        s->show();
        newpart->addMainWindow(s);
        return newpart;
    }
    // InitOnFileClose
    if (doc) {
        setRootDocument(nullptr);
    }
    newpart->addMainWindow(this);
    return newpart;
}

void KoMainWindow::slotFileNew()
{
    auto part = chooseNewDocument(InitOnFileNew);
    auto mainWindow = part->mainWindows().last();
    auto w = part->createWelcomeView(mainWindow);
    if (w) {
        mainWindow->setPartToOpen(part);
        mainWindow->setCentralWidget(w);
    } else {
        mainWindow->setRootDocument(part->document(), part);
    }
    mainWindow->show();
}

void KoMainWindow::slotFileOpen()
{
    QUrl url;
    if (!isImporting()) {
        KoFileDialog dialog(this, KoFileDialog::OpenFile, QStringLiteral("OpenDocument"));
        dialog.setCaption(i18n("Open Document"));
        dialog.setDefaultDir(qApp->applicationName().contains(QStringLiteral("karbon"))
                               ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                               : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        dialog.setMimeTypeFilters(koApp->mimeFilter(KoFilterManager::Import));
        dialog.setHideNameFilterDetailsOption();
        url = QUrl::fromUserInput(dialog.filename());
    } else {
        KoFileDialog dialog(this, KoFileDialog::ImportFile, QStringLiteral("OpenDocument"));
        dialog.setCaption(i18n("Import Document"));
        dialog.setDefaultDir(qApp->applicationName().contains(QStringLiteral("karbon"))
                                ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                                : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        dialog.setMimeTypeFilters(koApp->mimeFilter(KoFilterManager::Import));
        dialog.setHideNameFilterDetailsOption();
        url = QUrl::fromUserInput(dialog.filename());
    }

    if (url.isEmpty())
        return;

    (void) openDocument(url);
}

void KoMainWindow::slotFileOpenRecent(const QUrl & url, KoPart *part)
{
    // Create a copy, because the original QUrl in the map of recent files in
    // KRecentFilesAction may get deleted.
    (void) openDocument(part, QUrl(url));
}

void KoMainWindow::slotFileSave()
{
    if (saveDocument())
        Q_EMIT documentSaved();
}

void KoMainWindow::slotFileSaveAs()
{
    int specialOutputFlag = d->rootDocument->specialOutputFlag() == KoDocumentBase::SaveEncrypted ? KoDocumentBase::SaveEncrypted : 0;
    if (saveDocument(true, false, specialOutputFlag)) {
        Q_EMIT documentSaved();
    }
}

void KoMainWindow::slotEncryptDocument()
{
    if (saveDocument(false, false, KoDocument::SaveEncrypted))
        Q_EMIT documentSaved();
}

void KoMainWindow::slotUncompressToDir()
{
    if (saveDocument(true, false, KoDocument::SaveAsDirectoryStore))
        Q_EMIT documentSaved();
}

void KoMainWindow::slotDocumentInfo()
{
    if (!rootDocument())
        return;

    KoDocumentInfo *docInfo = rootDocument()->documentInfo();

    if (!docInfo)
        return;

    KoDocumentInfoDlg *dlg = d->rootDocument->createDocumentInfoDialog(this, docInfo);

    if (dlg->exec()) {
        if (dlg->isDocumentSaved()) {
            rootDocument()->setModified(false);
        } else {
            rootDocument()->setModified(true);
        }
        rootDocument()->setTitleModified();
    }

    delete dlg;
}

void KoMainWindow::slotFileClose()
{
    if (queryClose()) {
        saveWindowSettings();
        setRootDocument(nullptr);   // don't delete this main window when deleting the document
        if(d->rootDocument) {
            d->rootDocument->clearUndoHistory();
        }
        delete d->rootDocument;
        d->rootDocument = nullptr;
        auto part = chooseNewDocument(InitOnFileClose);
        auto mainWindow = part->mainWindows().last();
        auto w = part->createWelcomeView(mainWindow);
        if (w) {
            mainWindow->setPartToOpen(part);
            mainWindow->setCentralWidget(w);
        } else {
            mainWindow->setRootDocument(part->document(), part);
        }
        mainWindow->show();
    }
}

void KoMainWindow::slotFileQuit()
{
    close();
}

void KoMainWindow::slotFilePrint()
{
    if (!rootView())
        return;
    KoPrintJob *printJob = rootView()->createPrintJob();
    if (printJob == nullptr)
        return;
    d->applyDefaultSettings(printJob->printer());
    // Must be blocking as we change the palette while printing
    printJob->setProperty("blocking", true);
    QPrintDialog *printDialog = rootView()->createPrintDialog(printJob, this);
    if (printDialog && printDialog->exec() == QDialog::Accepted) {
        printJob->startPrinting(KoPrintJob::DeleteWhenDone);
    } else {
        delete printJob;
    }
    delete printDialog;
}

void KoMainWindow::slotFilePrintPreview()
{
#if 0
    // QPrintPreviewDialog quickly uses gigabytes of memory when printing even moderately large gantt chart
    if (!rootView())
        return;
    KoPrintJob *printJob = rootView()->createPrintJob();
    if (printJob == nullptr)
        return;

    /* Sets the startPrinting() slot to be blocking.
     The Qt print-preview dialog requires the printing to be completely blocking
     and only return when the full document has been printed.
     By default the KoPrintingDialog is non-blocking and
     multithreading, setting blocking to true will allow it to be used in the preview dialog */
    printJob->setProperty("blocking", true);
    QPrintPreviewDialog *preview = new QPrintPreviewDialog(&printJob->printer(), this);
    printJob->setParent(preview); // will take care of deleting the job
    d->printPreviewJob = printJob;
    connect(preview, SIGNAL(paintRequested(QPrinter*)), this, SLOT(slotPrintPreviewPaintRequest(QPrinter*))); // clazy:exclude=old-style-connect
    preview->exec();
    delete preview;
    d->printPreviewJob = nullptr;
#else
    printPreviewToPdf();
#endif
}

void KoMainWindow::slotPrintPreviewPaintRequest(QPrinter *printer)
{
    KoPageLayout pl = rootView()->pageLayout();
    pl.updatePageLayout(printer);
    rootView()->setPageLayout(pl);
    d->printPreviewJob->startPrinting();
}

KoPrintJob* KoMainWindow::printPreviewToPdf()
{
    if (!rootView()) {
        return nullptr;
    }
    KoDocument* pDoc = rootDocument();
    if (!pDoc) {
        return nullptr;
    }
    KoPrintJob *printJob = rootView()->createPdfPrintJob();
    if (printJob == nullptr) {
        return nullptr;
    }
    KoPageLayout pageLayout;
    pageLayout = rootView()->pageLayout();

    QTemporaryDir dir;
    dir.setAutoRemove(false);
    QString pdfFileName;

    if (pdfFileName.isEmpty()) {
        /** if document has a file name, take file name and replace extension with .pdf */
        if (pDoc && pDoc->url().isValid()) {
            auto startUrl = pDoc->url();
            QString fileName = startUrl.fileName();
            pdfFileName = fileName.replace(QRegularExpression(QStringLiteral("\\.\\w{2,5}$"), QRegularExpression::CaseInsensitiveOption), QStringLiteral(".pdf"));
        }
    }
    pdfFileName = QStringLiteral("%1/%2").arg(dir.path()).arg(pdfFileName);

    if (isHidden()) {
        printJob->setProperty("noprogressdialog", true);
    }

    d->applyDefaultSettings(printJob->printer());
    printJob->printer().setOutputFileName(pdfFileName);
    printJob->printer().setColorMode(QPrinter::Color);

    if (pageLayout.format == KoPageFormat::CustomSize) {
        printJob->printer().setPageSize(QPageSize(QSizeF(pageLayout.width, pageLayout.height), QPageSize::Millimeter));
    } else {
        printJob->printer().setPageSize(KoPageFormat::qPageSize(pageLayout.format));
    }

    switch (pageLayout.orientation) {
    case KoPageFormat::Portrait: printJob->printer().setPageOrientation(QPageLayout::Portrait); break;
    case KoPageFormat::Landscape: printJob->printer().setPageOrientation(QPageLayout::Landscape); break;
    }

    printJob->printer().setPageMargins(pageLayout.pageMargins(), QPageLayout::Millimeter);

    //before printing check if the printer can handle printing
    if (!printJob->canPrint()) {
        KMessageBox::error(this, i18n("Cannot preview the document"));
        return printJob;
    }
    printJob->setProperty("blocking", true);
    printJob->startPrinting(KoPrintJob::DeleteWhenDone);
    QUrl url(pdfFileName);
    url.setScheme(QStringLiteral("file"));
    if (!QDesktopServices::openUrl(url)) {
        KMessageBox::error(this, i18n("Cannot open pdf viewer"));
    }

    rootView()->setPageLayout(pageLayout);

    return printJob;
}

KoPrintJob* KoMainWindow::exportToPdf()
{
    return exportToPdf(QString());
}

KoPrintJob* KoMainWindow::exportToPdf(const QString &_pdfFileName)
{
    if (!rootView())
        return nullptr;
    KoPageLayout pageLayout;
    pageLayout = rootView()->pageLayout();

    QString pdfFileName = _pdfFileName;

    if (pdfFileName.isEmpty()) {
        KConfigGroup group =  KSharedConfig::openConfig()->group("File Dialogs");
        QString defaultDir = group.readEntry("SavePdfDialog");
        if (defaultDir.isEmpty())
            defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QUrl startUrl = QUrl::fromLocalFile(defaultDir);
        KoDocument* pDoc = rootDocument();
        /** if document has a file name, take file name and replace extension with .pdf */
        if (pDoc && pDoc->url().isValid()) {
            startUrl = pDoc->url();
            QString fileName = startUrl.fileName();
            fileName = fileName.replace(QRegularExpression(QStringLiteral("\\.\\w{2,5}$"), QRegularExpression::CaseInsensitiveOption), QStringLiteral(".pdf"));
            startUrl = startUrl.adjusted(QUrl::RemoveFilename);
            startUrl.setPath(startUrl.path() +  fileName);
        }

        QPointer<KoPageLayoutDialog> layoutDlg(new KoPageLayoutDialog(this, pageLayout));
        layoutDlg->setWindowModality(Qt::WindowModal);
        if (layoutDlg->exec() != QDialog::Accepted || !layoutDlg) {
            delete layoutDlg;
            return nullptr;
        }
        pageLayout = layoutDlg->pageLayout();
        delete layoutDlg;

        KoFileDialog dialog(this, KoFileDialog::SaveFile, QStringLiteral("SaveDocument"));
        dialog.setCaption(i18n("Export as PDF"));
        dialog.setDefaultDir(startUrl.toLocalFile());
        dialog.setMimeTypeFilters(QStringList() << QStringLiteral("application/pdf"));
        QUrl url = QUrl::fromUserInput(dialog.filename());

        pdfFileName = url.toLocalFile();
        if (pdfFileName.isEmpty())
            return nullptr;
    }

    KoPrintJob *printJob = rootView()->createPdfPrintJob();
    if (printJob == nullptr)
        return nullptr;
    if (isHidden()) {
        printJob->setProperty("noprogressdialog", true);
    }

    d->applyDefaultSettings(printJob->printer());
    // TODO for remote files we have to first save locally and then upload.
    printJob->printer().setOutputFileName(pdfFileName);
    printJob->printer().setColorMode(QPrinter::Color);

    if (pageLayout.format == KoPageFormat::CustomSize) {
        printJob->printer().setPageSize(QPageSize(QSizeF(pageLayout.width, pageLayout.height), QPageSize::Millimeter));
    } else {
        printJob->printer().setPageSize(KoPageFormat::qPageSize(pageLayout.format));
    }

    switch (pageLayout.orientation) {
    case KoPageFormat::Portrait: printJob->printer().setPageOrientation(QPageLayout::Portrait); break;
    case KoPageFormat::Landscape: printJob->printer().setPageOrientation(QPageLayout::Landscape); break;
    }

    printJob->printer().setPageMargins(pageLayout.pageMargins(), QPageLayout::Millimeter);

    //before printing check if the printer can handle printing
    if (!printJob->canPrint()) {
        KMessageBox::error(this, i18n("Cannot export to the specified file"));
    }

    printJob->startPrinting(KoPrintJob::DeleteWhenDone);

    rootView()->setPageLayout(pageLayout);

    return printJob;
}

void KoMainWindow::slotConfigureKeys()
{
    QAction* undoAction=nullptr;
    QAction* redoAction=nullptr;
    QString oldUndoText;
    QString oldRedoText;
    if(currentView()) {
        //The undo/redo action text is "undo" + command, replace by simple text while inside editor
        undoAction = currentView()->actionCollection()->action(QStringLiteral("edit_undo"));
        redoAction = currentView()->actionCollection()->action(QStringLiteral("edit_redo"));
        oldUndoText = undoAction->text();
        oldRedoText = redoAction->text();
        undoAction->setText(i18n("Undo"));
        redoAction->setText(i18n("Redo"));
    }

    guiFactory()->showConfigureShortcutsDialog();

    if(currentView()) {
        undoAction->setText(oldUndoText);
        redoAction->setText(oldRedoText);
    }

    Q_EMIT keyBindingsChanged();
}

void KoMainWindow::slotConfigureToolbars()
{
    if (rootDocument()) {
        KConfigGroup componentConfigGroup =  KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
        saveMainWindowSettings(componentConfigGroup);
    }

    KoEditToolBar edit(factory(), this);
    connect(&edit, &KoEditToolBar::newToolBarConfig, this, &KoMainWindow::slotNewToolbarConfig);
    (void) edit.exec();
}

void KoMainWindow::slotNewToolbarConfig()
{
    if (rootDocument()) {
        KConfigGroup componentConfigGroup =  KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
        applyMainWindowSettings(componentConfigGroup);
    }

    KXMLGUIFactory *factory = guiFactory();
    Q_UNUSED(factory);

    // Check if there's an active view
    if (!d->activeView)
        return;

    plugActionList(QStringLiteral("toolbarlist"), d->toolbarList);
    actionCollection()->action(QStringLiteral("options_configure_toolbars"))->setEnabled(!d->toolbarList.isEmpty());

}

void KoMainWindow::slotToolbarToggled(bool toggle)
{
    //debugMain <<"KoMainWindow::slotToolbarToggled" << sender()->name() <<" toggle=" << true;
    // The action (sender) and the toolbar have the same name
    KToolBar * bar = toolBar(sender()->objectName());
    if (bar) {
        if (toggle)
            bar->show();
        else
            bar->hide();

        if (rootDocument()) {
            KConfigGroup componentConfigGroup =  KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
            saveMainWindowSettings(componentConfigGroup);
        }
    } else
        warnMain << "slotToolbarToggled : Toolbar " << sender()->objectName() << " not found!";
}

void KoMainWindow::viewFullscreen(bool fullScreen)
{
    if (fullScreen) {
        window()->setWindowState(window()->windowState() | Qt::WindowFullScreen);   // set
    } else {
        window()->setWindowState(window()->windowState() & ~Qt::WindowFullScreen);   // reset
    }
}

void KoMainWindow::slotProgress(int value)
{
    QMutexLocker locker(&d->progressMutex);
    debugMain << value;
    if (value <= -1 || value >= 100) {
        if (d->progress) {
            statusBar()->removeWidget(d->progress);
            delete d->progress;
            d->progress = nullptr;
        }
        d->firstTime = true;
        return;
    }
    if (d->firstTime || !d->progress) {
        // The statusbar might not even be created yet.
        // So check for that first, and create it if necessary
        QStatusBar *bar = findChild<QStatusBar *>();
        if (!bar) {
            statusBar()->show();
            QApplication::sendPostedEvents(this, QEvent::ChildAdded);
        }

        if (d->progress) {
            statusBar()->removeWidget(d->progress);
            delete d->progress;
            d->progress = nullptr;
        }

        d->progress = new QProgressBar(statusBar());
        d->progress->setMaximumHeight(statusBar()->fontMetrics().height());
        d->progress->setRange(0, 100);
        statusBar()->addPermanentWidget(d->progress);
        d->progress->show();
        d->firstTime = false;
    }
    if (!d->progress.isNull()) {
        d->progress->setValue(value);
    }
    locker.unlock();
    qApp->processEvents();
}

void KoMainWindow::setMaxRecentItems(uint _number)
{
    d->recent->setMaxItems(_number);
}

void KoMainWindow::slotEmailFile()
{
    if (!rootDocument())
        return;

    // Subject = Document file name
    // Attachment = The current file
    // Message Body = The current document in HTML export? <-- This may be an option.
    QString theSubject;
    QList<QUrl> urls;
    QString fileURL;
    if (rootDocument()->url().isEmpty() ||
            rootDocument()->isModified()) {
        //Save the file as a temporary file
        bool const tmp_modified = rootDocument()->isModified();
        QUrl const tmp_url = rootDocument()->url();
        QByteArray const tmp_mimetype = rootDocument()->outputMimeType();

        // a little open, close, delete dance to make sure we have a nice filename
        // to use, but won't block windows from creating a new file with this name.
        QTemporaryFile *tmpfile = new QTemporaryFile();
        tmpfile->open();
        QString fileName = tmpfile->fileName();
        tmpfile->close();
        delete tmpfile;

        QUrl u = QUrl::fromLocalFile(fileName);
        rootDocument()->setUrl(u);
        rootDocument()->setModified(true);
        rootDocument()->setOutputMimeType(rootDocument()->nativeFormatMimeType());

        saveDocument(false, true);

        fileURL = fileName;
        theSubject = i18n("Document");
        urls.append(QUrl::fromUserInput(fileURL));

        rootDocument()->setUrl(tmp_url);
        rootDocument()->setModified(tmp_modified);
        rootDocument()->setOutputMimeType(tmp_mimetype);
    } else {
        fileURL = rootDocument()->url().url();
        theSubject = i18n("Document - %1", rootDocument()->url().fileName());
        urls.append(QUrl::fromUserInput(fileURL));
    }

    debugMain << "(" << fileURL << ")";

    if (!fileURL.isEmpty()) {
        auto job = new KEMailClientLauncherJob();
        job->setAttachments(urls);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->start();
    }
}

void KoMainWindow::slotReloadFile()
{
    KoDocument* pDoc = rootDocument();
    if (!pDoc || pDoc->url().isEmpty() || !pDoc->isModified())
        return;

    bool bOk = KMessageBox::questionTwoActions(this,
                                          i18n("You will lose all changes made since your last save\n"
                                               "Do you want to continue?"),
                                          i18n("Warning"),
                                          KStandardGuiItem::save(),
                                          KStandardGuiItem::dontSave()) == KMessageBox::PrimaryAction;
    if (!bOk)
        return;

    QUrl url = pDoc->url();
    if (!pDoc->isEmpty()) {
        saveWindowSettings();
        setRootDocument(nullptr);   // don't delete this main window when deleting the document
        if(d->rootDocument)
            d->rootDocument->clearUndoHistory();
        delete d->rootDocument;
        d->rootDocument = nullptr;
    }
    openDocument(url);
    return;

}

void KoMainWindow::slotImportFile()
{
    debugMain;

    d->isImporting = true;
    slotFileOpen();
    d->isImporting = false;
}

void KoMainWindow::slotExportFile()
{
    debugMain;

    d->isExporting = true;
    slotFileSaveAs();
    d->isExporting = false;
}

bool KoMainWindow::isImporting() const
{
    return d->isImporting;
}

bool KoMainWindow::isExporting() const
{
    return d->isExporting;
}

void KoMainWindow::setPartToOpen(KoPart *part)
{
    d->partToOpen = part;
}

KoComponentData KoMainWindow::componentData() const
{
    return d->componentData;
}

QDockWidget* KoMainWindow::createDockWidget(KoDockFactoryBase* factory)
{
    QDockWidget* dockWidget = nullptr;
    if (!d->dockWidgetsMap.contains(factory->id())) {
        dockWidget = factory->createDockWidget();

        // It is quite possible that a dock factory cannot create the dock; don't
        // do anything in that case.
        if (!dockWidget) return nullptr;
        d->dockWidgets.push_back(dockWidget);

        KoDockWidgetTitleBar *titleBar = nullptr;
        // Check if the dock widget is supposed to be collapsable
        if (!dockWidget->titleBarWidget()) {
            titleBar = new KoDockWidgetTitleBar(dockWidget);
            dockWidget->setTitleBarWidget(titleBar);
            titleBar->setCollapsable(factory->isCollapsable());
        }

        dockWidget->setObjectName(factory->id());
        dockWidget->setParent(this);

        if (dockWidget->widget() && dockWidget->widget()->layout())
            dockWidget->widget()->layout()->setContentsMargins(1, 1, 1, 1);

        Qt::DockWidgetArea side = Qt::RightDockWidgetArea;
        bool visible = true;

        switch (factory->defaultDockPosition()) {
        case KoDockFactoryBase::DockTornOff:
            dockWidget->setFloating(true); // position nicely?
            break;
        case KoDockFactoryBase::DockTop:
            side = Qt::TopDockWidgetArea; break;
        case KoDockFactoryBase::DockLeft:
            side = Qt::LeftDockWidgetArea; break;
        case KoDockFactoryBase::DockBottom:
            side = Qt::BottomDockWidgetArea; break;
        case KoDockFactoryBase::DockRight:
            side = Qt::RightDockWidgetArea; break;
        case KoDockFactoryBase::DockMinimized:
        default:
            side = Qt::RightDockWidgetArea;
            visible = false;
        }

        if (rootDocument()) {
            KConfigGroup group =  KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName()).group(QStringLiteral("DockWidget ") + factory->id());
            side = static_cast<Qt::DockWidgetArea>(group.readEntry("DockArea", static_cast<int>(side)));
            if (side == Qt::NoDockWidgetArea) side = Qt::RightDockWidgetArea;
        }
        addDockWidget(side, dockWidget);
        if (dockWidget->features() & QDockWidget::DockWidgetClosable) {
            d->dockWidgetMenu->addAction(dockWidget->toggleViewAction());
            if (!visible)
                dockWidget->hide();
        }

        bool collapsed = factory->defaultCollapsed();
        bool locked = false;
        if (rootDocument()) {
            KConfigGroup group =  KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName()).group(QStringLiteral("DockWidget ") + factory->id());
            collapsed = group.readEntry("Collapsed", collapsed);
            locked = group.readEntry("Locked", locked);
        }
        if (titleBar && collapsed)
            titleBar->setCollapsed(true);
        if (titleBar && locked)
            titleBar->setLocked(true);

        if (titleBar) {
            KConfigGroup configGroupInterface =  KSharedConfig::openConfig()->group("Interface");
            titleBar->setVisible(configGroupInterface.readEntry("ShowDockerTitleBars", true));
        }

        d->dockWidgetsMap.insert(factory->id(), dockWidget);
    } else {
        dockWidget = d->dockWidgetsMap[ factory->id()];
    }

#ifdef Q_OS_DARWIN
    dockWidget->setAttribute(Qt::WA_MacSmallSize, true);
#endif
    dockWidget->setFont(KoDockRegistry::dockFont());

    connect(dockWidget, &QDockWidget::dockLocationChanged, this, &KoMainWindow::forceDockTabFonts);

    return dockWidget;
}

void KoMainWindow::forceDockTabFonts()
{
    QObjectList chis = children();
    for (int i = 0; i < chis.size(); ++i) {
        if (chis.at(i)->inherits("QTabBar")) {
            ((QTabBar *)chis.at(i))->setFont(KoDockRegistry::dockFont());
        }
    }
}

QList<QDockWidget*> KoMainWindow::dockWidgets() const
{
    return d->dockWidgetsMap.values();
}

/*QList<KoCanvasObserverBase*> KoMainWindow::canvasObservers() const
{

    QList<KoCanvasObserverBase*> observers;

    for (QDockWidget *docker : qAsConst(dockWidgets())) {
        KoCanvasObserverBase *observer = dynamic_cast<KoCanvasObserverBase*>(docker);
        if (observer) {
            observers << observer;
        }
    }
    return observers;
}*/

void KoMainWindow::toggleDockersVisibility(bool visible)
{
    if (!visible) {
        d->m_dockerStateBeforeHiding = saveState();
        const auto widgets = children();
        for (QObject* widget : widgets) {
            if (widget->inherits("QDockWidget")) {
                QDockWidget* dw = static_cast<QDockWidget*>(widget);
                if (dw->isVisible()) {
                    dw->hide();
                }
            }
        }
    }
    else {
        restoreState(d->m_dockerStateBeforeHiding);
    }
}

KRecentFilesAction *KoMainWindow::recentAction() const
{
    return d->recent;
}

KoView* KoMainWindow::currentView() const
{
    // XXX
    if (d->activeView) {
        return d->activeView;
    }
    else if (!d->rootViews.isEmpty()) {
        return d->rootViews.first();
    }
    return nullptr;
}

void KoMainWindow::newView()
{
    Q_ASSERT((d != nullptr && d->activeView && d->activePart && d->activeView->koDocument()));

    KoMainWindow *mainWindow = d->activePart->createMainWindow();
    mainWindow->setRootDocument(d->activeView->koDocument(), d->activePart);
    mainWindow->show();
}

void KoMainWindow::createMainwindowGUI()
{
    if (isHelpMenuEnabled() && !d->m_helpMenu) {
        d->m_helpMenu = new KHelpMenu(this, componentData().aboutData(), true);

        KActionCollection *actions = actionCollection();
        QAction *helpContentsAction = d->m_helpMenu->action(KHelpMenu::menuHelpContents);
        QAction *whatsThisAction = d->m_helpMenu->action(KHelpMenu::menuWhatsThis);
        QAction *reportBugAction = d->m_helpMenu->action(KHelpMenu::menuReportBug);
        QAction *switchLanguageAction = d->m_helpMenu->action(KHelpMenu::menuSwitchLanguage);
        QAction *aboutAppAction = d->m_helpMenu->action(KHelpMenu::menuAboutApp);
        QAction *aboutKdeAction = d->m_helpMenu->action(KHelpMenu::menuAboutKDE);

        if (helpContentsAction) {
            actions->addAction(helpContentsAction->objectName(), helpContentsAction);
        }
        if (whatsThisAction) {
            actions->addAction(whatsThisAction->objectName(), whatsThisAction);
        }
        if (reportBugAction) {
            actions->addAction(reportBugAction->objectName(), reportBugAction);
        }
        if (switchLanguageAction) {
            actions->addAction(switchLanguageAction->objectName(), switchLanguageAction);
        }
        if (aboutAppAction) {
            actions->addAction(aboutAppAction->objectName(), aboutAppAction);
        }
        if (aboutKdeAction) {
            actions->addAction(aboutKdeAction->objectName(), aboutKdeAction);
        }
    }
    QAction *configureAction = new QAction(this);
    configureAction->setIcon(koIcon("configure"));
    actionCollection()->addAction(QStringLiteral("configure"), configureAction);
    connect(configureAction, &QAction::triggered, this, &KoMainWindow::slotConfigure);

    guiFactory()->addClient(this);
}

// PartManager

void KoMainWindow::removePart(KoPart *part)
{
    if (d->m_registeredPart.data() != part) {
        return;
    }
    d->m_registeredPart = nullptr;
    if (part == d->m_activePart) {
        setActivePart(nullptr, nullptr);
    }
}

void KoMainWindow::setActivePart(KoPart *part, QWidget *widget)
{
    if (part && d->m_registeredPart.data() != part) {
        warnMain << "trying to activate a non-registered part!" << part->objectName();
        return; // don't allow someone call setActivePart with a part we don't know about
    }

    // don't activate twice
    if (d->m_activePart && part && d->m_activePart == part &&
         (!widget || d->m_activeWidget == widget))
        return;

    KoPart *oldActivePart = d->m_activePart;
    QWidget *oldActiveWidget = d->m_activeWidget;

    d->m_activePart = part;
    d->m_activeWidget = widget;

    if (oldActivePart) {
        KoPart *savedActivePart = part;
        QWidget *savedActiveWidget = widget;

        if (oldActiveWidget) {
            disconnect(oldActiveWidget, &QObject::destroyed, this, &KoMainWindow::slotWidgetDestroyed);
        }

        d->m_activePart = savedActivePart;
        d->m_activeWidget = savedActiveWidget;
    }

    if (d->m_activePart && d->m_activeWidget) {
        connect(d->m_activeWidget, &QObject::destroyed, this, &KoMainWindow::slotWidgetDestroyed);
    }
    // Set the new active instance in KGlobal
//     KGlobal::setActiveComponent(d->m_activePart ? d->m_activePart->componentData() : KGlobal::mainComponent());

    // old slot called from part manager
    KoPart *newPart = static_cast<KoPart*>(d->m_activePart.data());

    if (d->activePart && d->activePart == newPart) {
        //debugMain <<"no need to change the GUI";
        return;
    }

    KXMLGUIFactory *factory = guiFactory();

    if (d->activeView) {

        factory->removeClient(d->activeView);

        unplugActionList(QStringLiteral("toolbarlist"));
        qDeleteAll(d->toolbarList);
        d->toolbarList.clear();
    }

    if (!d->mainWindowGuiIsBuilt) {
        createMainwindowGUI();
    }

    if (newPart && d->m_activeWidget && d->m_activeWidget->inherits("KoView")) {
        d->activeView = qobject_cast<KoView *>(d->m_activeWidget);
        d->activePart = newPart;
        //debugMain <<"new active part is" << d->activePart;

        factory->addClient(d->activeView);

        // Position and show toolbars according to user's preference
        setAutoSaveSettings(newPart->componentData().componentName(), false);

        KConfigGroup configGroupInterface =  KSharedConfig::openConfig()->group("Interface");
        const bool showDockerTitleBar = configGroupInterface.readEntry("ShowDockerTitleBars", true);
        for (QDockWidget *wdg : qAsConst(d->dockWidgets)) {
            if ((wdg->features() & QDockWidget::DockWidgetClosable) == 0) {
                if (wdg->titleBarWidget()) {
                    wdg->titleBarWidget()->setVisible(showDockerTitleBar);
                }
                wdg->setVisible(true);
            }
        }

        // Create and plug toolbar list for Settings menu
        const auto widgets = factory->containers(QStringLiteral("ToolBar"));
        for (QWidget* it : widgets) {
            KToolBar * toolBar = ::qobject_cast<KToolBar *>(it);
            if (toolBar) {
                KToggleAction * act = new KToggleAction(i18n("Show %1 Toolbar", toolBar->windowTitle()), this);
                actionCollection()->addAction(toolBar->objectName(), act);
                act->setCheckedState(KGuiItem(i18n("Hide %1 Toolbar", toolBar->windowTitle())));
                connect(act, &QAction::toggled, this, &KoMainWindow::slotToolbarToggled);
                act->setChecked(!toolBar->isHidden());
                d->toolbarList.append(act);
            } else
                warnMain << "Toolbar list contains a " << it->metaObject()->className() << " which is not a toolbar!";
        }
        plugActionList(QStringLiteral("toolbarlist"), d->toolbarList);
        actionCollection()->action(QStringLiteral("options_configure_toolbars"))->setEnabled(!d->toolbarList.isEmpty());
    }
    else {
        d->activeView = nullptr;
        d->activePart = nullptr;
    }

    if (d->activeView) {
        d->activeView->guiActivateEvent(true);
    }
}

void KoMainWindow::slotWidgetDestroyed()
{
    debugMain;
    if (static_cast<const QWidget *>(sender()) == d->m_activeWidget)
        setActivePart(nullptr, nullptr); //do not remove the part because if the part's widget dies, then the
    //part will delete itself anyway, invoking removePart() in its destructor
}

void KoMainWindow::slotDocumentTitleModified(const QString &caption, bool mod)
{
    updateCaption(caption, mod);
    updateReloadFileAction(d->rootDocument);
}

void KoMainWindow::showDockerTitleBars(bool show)
{
    const auto dockers = dockWidgets();
    for (QDockWidget *dock : dockers) {
        if (dock->titleBarWidget()) {
            dock->titleBarWidget()->setVisible(show);
        }
    }

    KConfigGroup configGroupInterface =  KSharedConfig::openConfig()->group("Interface");
    configGroupInterface.writeEntry("ShowDockerTitleBars", show);
}

void KoMainWindow::slotConfigure()
{
    Q_EMIT configure(this);
}

void KoMainWindow::setBlockClose(bool value)
{
    d->blockClose = value;
}
