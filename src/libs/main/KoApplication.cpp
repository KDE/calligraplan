/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2009 Thomas Zander <zander@kde.org>
   SPDX-FileCopyrightText: 2012 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoApplication.h"

#include "MimeTypes.h"
#include "KoGlobal.h"

#ifndef QT_NO_DBUS
#include "KoApplicationAdaptor.h"
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#endif

#include "KoPrintJob.h"
#include "KoDocumentEntry.h"
#include "KoDocument.h"
#include "KoMainWindow.h"
#include "KoAutoSaveRecoveryDialog.h"
#include <KoDpi.h>
#include "KoPart.h"
#include <KoPluginLoader.h>
#include <config.h>
#include <KoResourcePaths.h>
#include <KoComponentData.h>

#include <KLocalizedString>
#include <KDesktopFile>
#include <KMessageBox>
#include <KIconLoader>
#include <MainDebug.h>
#include <KConfig>
#include <KConfigGroup>
#include <KRecentDirs>
#include <KAboutData>
#include <KSharedConfig>
#include <KDBusService>
#include <KRecentFilesAction>

#include <QFile>
#include <QWidget>
#include <QSysInfo>
#include <QStringList>
#include <QProcessEnvironment>
#include <QDir>
#include <QPluginLoader>
#include <QCommandLineParser>
#include <QMimeDatabase>

#include <stdlib.h>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tchar.h>
#endif


KoApplication* KoApplication::KoApp = nullptr;

namespace {
const QTime appStartTime(QTime::currentTime());
}

class KoApplicationPrivate
{
public:
    KoApplicationPrivate() {}

    QByteArray nativeMimeType;
    QList<KoPart *> partList;
};

KoApplication::KoApplication(const QByteArray &nativeMimeType,
                             const QString &windowIconName,
                             AboutDataGenerator aboutDataGenerator,
                             int &argc, char **argv)
    : QApplication(argc, argv)
    , d(new KoApplicationPrivate())
{

    QScopedPointer<KAboutData> aboutData(aboutDataGenerator());
    KAboutData::setApplicationData(*aboutData);

    setWindowIcon(QIcon::fromTheme(windowIconName, windowIcon()));

    KoApplication::KoApp = this;

    d->nativeMimeType = nativeMimeType;

    // Initialize all Calligra directories etc.
    KoGlobal::initialize();

#ifndef QT_NO_DBUS
    KDBusService service(KDBusService::Multiple);

    new KoApplicationAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/application"), this);
#endif

#if 0
Note: This issue seems to be fixed in qt 5.2.
      Keep the code until it is verified.
#ifdef Q_OS_MACOS
    if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8)
    {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(QLatin1String(".Lucida Grande UI"), QLatin1String("Lucida Grande"));
    }

    setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif
#endif
}

KoApplication::~KoApplication()
{
    delete d;
}

#if defined(Q_OS_WIN) && defined(ENV32BIT)
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL isWow64()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(0 != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            //handle error
        }
    }
    return bIsWow64;
}
#endif

bool KoApplication::openAutosaveFile(const QDir &autosaveDir, const QString &autosaveFile)
{
    const QStringList split = autosaveFile.split(QLatin1Char('-'));
    // FIXME: more generic?
    QString mimetype = split.last().endsWith(QString::fromLatin1(".planp")) ? PLANPORTFOLIO_MIME_TYPE : PLAN_MIME_TYPE;
    KoPart *part = getPart(split.value(0).remove(QLatin1Char('.')), mimetype);
    if (!part) {
        return false;
    }
    connect(part, &KoPart::destroyed, this, &KoApplication::slotPartDestroyed);
    d->partList << part;
    QUrl url = QUrl::fromLocalFile(autosaveDir.absolutePath() + QDir::separator() + autosaveFile);
    KoMainWindow *mainWindow = part->createMainWindow();
    mainWindow->show();
    if (mainWindow->openDocument(part, url)) {
        part->document()->resetURL();
        part->document()->setModified(true);
        // TODO: what if the app crashes immediately, before another autosave was made? better keep & rename
        QFile::remove(url.toLocalFile());
        return true;
    }
    return false;
}

bool KoApplication::start(const KoComponentData &componentData)
{
    Q_UNUSED(componentData)

    KAboutData aboutData = KAboutData::applicationData();

    // process commandline parameters
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("benchmark-loading"), i18n("just load the file and then exit")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("benchmark-loading-show-window"), i18n("load the file, show the window and progressbar and then exit")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("profile-filename"), i18n("Filename to write profiling information into."), QStringLiteral("filename")));
    parser.addPositionalArgument(QStringLiteral("[file(s)]"), i18n("File(s) or URL(s) to open"));

    parser.process(*this);

    aboutData.processCommandLine(&parser);

    const QStringList fileUrls = parser.positionalArguments();
    if (fileUrls.isEmpty()) {
        // get all possible autosave files in the home dir, this is for unsaved document autosave files
        // Using the extension allows to avoid relying on the mime magic when opening
        QMimeType mimeType = QMimeDatabase().mimeTypeForName(QString::fromLatin1(d->nativeMimeType));
        if (!mimeType.isValid()) {
            qFatal("It seems your installation is broken/incomplete because we failed to load the native mimetype \"%s\".", d->nativeMimeType.constData());
        }
        QStringList filters;
        filters << QStringLiteral(".%1-%2-%3-autosave%4").arg(applicationName()).arg(QLatin1Char('*')).arg(QLatin1Char('*')).arg(mimeType.preferredSuffix());

#ifdef Q_OS_WIN
        QDir autosaveDir = QDir::tempPath();
#else
        QDir autosaveDir = QDir::home();
#endif
        QStringList pids;
        QString ourPid;
        ourPid.setNum(applicationPid());

#ifndef QT_NO_DBUS
        // all running instances of our application -- bit hackish, but we cannot get at the dbus name here, for some reason
        QDBusReply<QStringList> reply = QDBusConnection::sessionBus().interface()->registeredServiceNames();
        const auto names = reply.value();
        for (const QString &name : names) {
            if (name.contains(QStringLiteral("calligraplan"))) {
                // we got another instance of ourselves running, let's get the pid
                QString pid = name.split(QLatin1Char('-')).last();
                if (pid != ourPid) {
                    pids << pid;
                }
            }
        }
#endif

        // Check for autosave files from a previous run. There can be several, and
        // we want to offer a restore for every one. Including a nice thumbnail!
        QStringList autosaveFiles = autosaveDir.entryList(filters, QDir::Files | QDir::Hidden);
        // remove the autosave files that are saved for other, open instances of ourselves
        for (const QString &autosaveFileName : std::as_const(autosaveFiles)) {
            if (!QFile::exists(autosaveDir.absolutePath() + QDir::separator() + autosaveFileName)) {
                autosaveFiles.removeAll(autosaveFileName);
                continue;
            }
            QStringList split = autosaveFileName.split(QLatin1Char('-'));
            if (split.size() == 4) {
                if (pids.contains(split[1])) {
                    // We've got an active, owned autosave file. Remove.
                    autosaveFiles.removeAll(autosaveFileName);
                }
            }
        }

        // Allow the user to make their selection
        if (!autosaveFiles.isEmpty()) {
            KoAutoSaveRecoveryDialog dlg(autosaveFiles);
            if (dlg.exec() == QDialog::Accepted) {
                QStringList filesToRecover = dlg.recoverableFiles();
                for (const QString &autosaveFileName : std::as_const(autosaveFiles)) {
                    if (!filesToRecover.contains(autosaveFileName)) {
                        // remove the files the user didn't want to recover
                        QFile::remove(autosaveDir.absolutePath() + QDir::separator() + autosaveFileName);
                    }
                }
                autosaveFiles = filesToRecover;
            } else {
                // don't recover any of the files, but don't delete them either
                autosaveFiles.clear();
            }
        }
        if (!autosaveFiles.isEmpty()) {
            short int numberOfOpenDocuments = 0; // number of documents open
            for (const QString &autosaveFile : std::as_const(autosaveFiles)) {
                if (openAutosaveFile(autosaveDir, autosaveFile)) {
                    numberOfOpenDocuments++;
                }
            }
            if (numberOfOpenDocuments > 0) {
                return true;
            }
        }
        KoPart *part = getPart(applicationName(), QString::fromLatin1(d->nativeMimeType));
        if (!part) {
            return false;
        }
        auto mainWindow = part->createMainWindow();
        auto w = part->createWelcomeView(mainWindow);
        if (w) {
            mainWindow->setPartToOpen(part);
            mainWindow->setCentralWidget(w);
        } else {
            mainWindow->setRootDocument(part->document(), part);
        }
        mainWindow->show();
        return true;
    } else {
        const bool benchmarkLoading = parser.isSet(QStringLiteral("benchmark-loading"))
        || parser.isSet(QStringLiteral("benchmark-loading-show-window"));
        // only show the mainWindow when no command-line mode option is passed
        const bool showmainWindow = parser.isSet(QStringLiteral("benchmark-loading-show-window"))
        || !parser.isSet(QStringLiteral("benchmark-loading"));
        const QString profileFileName = parser.value(QStringLiteral("profile-filename"));

        QTextStream profileoutput;
        QFile profileFile(profileFileName);
        if (!profileFileName.isEmpty() && profileFile.open(QFile::WriteOnly | QFile::Truncate)) {
            profileoutput.setDevice(&profileFile);
        }

        // Loop through arguments

        short int numberOfOpenDocuments = 0; // number of documents open
        // TODO: remove once Qt has proper handling itself
        const QRegularExpression withProtocolChecker(QStringLiteral("^[a-zA-Z]+:"));
        for (int argNumber = 0; argNumber < fileUrls.size(); ++argNumber) {
            const QString fileUrl = fileUrls.at(argNumber);
            // convert to an url
            const bool startsWithProtocol = withProtocolChecker.match(fileUrl).hasMatch();
            const QUrl url = startsWithProtocol ?
            QUrl::fromUserInput(fileUrl) :
            QUrl::fromLocalFile(QDir::current().absoluteFilePath(fileUrl));

            KoPart *part = getPart(applicationName(), QString::fromLatin1(d->nativeMimeType));
            if (part) {
                KoDocument *doc = part->document();
                // show a mainWindow asap
                KoMainWindow *mainWindow = part->createMainWindow();
                if (showmainWindow) {
                    mainWindow->show();
                }
                if (benchmarkLoading) {
                    doc->setReadWrite(false);
                    connect(mainWindow, &KoMainWindow::loadCompleted, this, &KoApplication::benchmarkLoadingFinished);
                    connect(mainWindow, &KoMainWindow::loadCompleted, this, &KoApplication::benchmarkLoadingFinished);
                }

                if (profileoutput.device()) {
                    doc->setProfileStream(&profileoutput);
                    profileoutput << "KoApplication::start\t"
                    << appStartTime.msecsTo(QTime::currentTime())
                    <<"\t0" << '\n';
                    doc->setAutoErrorHandlingEnabled(false);
                }
                doc->setProfileReferenceTime(appStartTime);
                if (mainWindow->openDocument(part, url)) {
                    if (benchmarkLoading) {
                        if (profileoutput.device()) {
                            profileoutput << "KoApplication::start\t"
                            << appStartTime.msecsTo(QTime::currentTime())
                            <<"\t100" << '\n';
                        }
                        return true; // only load one document!
                    } else {
                        // Normal case, success
                        numberOfOpenDocuments++;
                    }
                } else {
                    // .... if failed
                    mainWindow->setPartToOpen(nullptr);
                    delete part; // deletes document and mainwindow
                }

                if (profileoutput.device()) {
                    profileoutput << "KoApplication::start\t"
                    << appStartTime.msecsTo(QTime::currentTime())
                    <<"\t100" << '\n';
                }

            }
        }
        if (benchmarkLoading) {
            return false; // no valid urls found.
        }
        if (numberOfOpenDocuments == 0) { // no doc, e.g. all URLs were malformed
            return false;
        }
    }

    // not calling this before since the program will quit there.
    return true;
}

void KoApplication::benchmarkLoadingFinished()
{
    KoPart *part = d->partList.value(0);
    if (!part) {
        return;
    }
    KoMainWindow *mainWindow = part->mainWindows().value(0);
    if (!mainWindow) {
        return;
    }
    // close the document
    mainWindow->slotFileQuit();
}

QList<KoPart*> KoApplication::partList() const
{
    return d->partList;
}

QStringList KoApplication::mimeFilter(KoFilterManager::Direction direction) const
{
    KoDocumentEntry entry = KoDocumentEntry::queryByMimeType(QString::fromLatin1(d->nativeMimeType));
    QJsonObject json = entry.metaData();
    QStringList mimeTypes = json.value(QStringLiteral("X-KDE-ExtraNativeMimeTypes")).toVariant().toStringList();
    QStringList lst = KoFilterManager::mimeFilter(d->nativeMimeType, direction, mimeTypes);
    return lst;
}


bool KoApplication::notify(QObject *receiver, QEvent *event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (std::exception &e) {
        qWarning("Error %s sending event %i to object %s",
                 e.what(), event->type(), qPrintable(receiver->objectName()));
    } catch (...) {
        qWarning("Error <unknown> sending event %i to object %s",
                 event->type(), qPrintable(receiver->objectName()));
    }
    return false;

}

KoApplication *KoApplication::koApplication()
{
    return KoApp;
}

KoPart *KoApplication::getPart(const QString &appName, const QString &mimetype) const
{
    // Find the part component file corresponding to the application instance name
    KoDocumentEntry entry;
    QList<QPluginLoader*> pluginLoaders = KoPluginLoader::pluginLoaders("calligraplan/parts", mimetype);
    for (QPluginLoader *loader : std::as_const(pluginLoaders)) {
        if (loader->fileName().contains(appName + QStringLiteral("part"))) {
            entry = KoDocumentEntry(loader);
            pluginLoaders.removeOne(loader);
            break;
        }
    }
    qDeleteAll(pluginLoaders);

    if (entry.isEmpty()) {
        QMessageBox::critical(nullptr, i18n("%1: Critical Error", appName), i18n("Essential application components could not be found.\n"
        "This might be an installation issue.\n"
        "Try restarting or reinstalling."));
        return nullptr;
    }
    QString errorMsg;
    KoPart *part = entry.createKoPart(&errorMsg);
    if (!part) {
        if (!errorMsg.isEmpty())
            KMessageBox::error(nullptr, errorMsg);
        return nullptr;
    }
    connect(part, &KoPart::destroyed, this, &KoApplication::slotPartDestroyed);
    d->partList << part;
    return part;
}

KoPart *KoApplication::getPartFromUrl(const QUrl &url) const
{
    const QString mimetype = QMimeDatabase().mimeTypeForUrl(url).name();

    // Find the part component file corresponding to mimetype
    QList<QPluginLoader*> pluginLoaders = KoPluginLoader::pluginLoaders("calligraplan/parts", mimetype);
    if (pluginLoaders.isEmpty()) {
        warnMain<<Q_FUNC_INFO<<"Unknown mimetype:"<<mimetype<<"url:"<<url;
        return nullptr;
    }
    QPluginLoader *loader = pluginLoaders.value(0);
    pluginLoaders.removeOne(loader);
    qDeleteAll(pluginLoaders);

    KoDocumentEntry entry(loader);
    if (entry.isEmpty()) {
        QMessageBox::critical(nullptr, i18n("%1: Critical Error", mimetype), i18n("Essential application components could not be found.\n"
        "This might be an installation issue.\n"
        "Try restarting or reinstalling."));
        return nullptr;
    }
    QString errorMsg;
    KoPart *part = entry.createKoPart(&errorMsg);
    if (!part) {
        if (!errorMsg.isEmpty())
            KMessageBox::error(nullptr, errorMsg);
        return nullptr;
    }
    connect(part, &KoPart::destroyed, this, &KoApplication::slotPartDestroyed);
    d->partList << part;
    return part;
}

void KoApplication::slotPartDestroyed()
{
    for (int i = 0; i < d->partList.count(); ++i) {
        if (d->partList.at(i) == sender()) {
            d->partList.removeAt(i);
            break;
        }
    }
}
