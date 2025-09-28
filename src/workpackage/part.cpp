/* This file is part of the KDE project
 SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
 SPDX-FileCopyrightText: 2004-2009 Dag Andersen <dag.andersen@kdemail.net>
 SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
  SPDX-FileCopyrightText: 2007-2009, 2012 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "part.h"
#include "view.h"
#include "factory.h"
#include "mainwindow.h"
#include "workpackage.h"
#include "calligraplanworksettings.h"
#include <MimeTypes.h>

#include "plan/KPlatoXmlLoader.h" //NB!

#include "kptglobal.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptdocuments.h"
#include "kptcommand.h"

#include <KoXmlReader.h>
#include <KoStore.h>
#include <KoDocumentInfo.h>
#include <KoResourcePaths.h>
#include <KoComponentData.h>

#include <QPainter>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QFileSystemWatcher>
#include <kundo2qstack.h>
#include <QPointer>
#include <QUrl>
#include <QMimeDatabase>
#include <QApplication>

#include <KLocalizedString>
#include <KMessageBox>
#include <KParts/PartManager>
#include <KOpenWithDialog>
#include <KIO/DesktopExecParser>
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KProcess>
#include <KActionCollection>
#include <KApplicationTrader>

#include "debugarea.h"

namespace KPlatoWork
{

//-------------------------------
DocumentChild::DocumentChild(WorkPackage *parent)
    : QObject(parent),
    m_doc(nullptr),
    m_type(Type_Unknown),
    m_copy(false),
    m_process(nullptr),
    m_editor(nullptr),
    m_editormodified(false),
    m_filemodified(false),
    m_fileSystemWatcher(new QFileSystemWatcher(this))

{
}

DocumentChild::~DocumentChild()
{
    debugPlanWork;
    disconnect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &DocumentChild::slotDirty);
    m_fileSystemWatcher->removePath(filePath());

    if (m_type == Type_Calligra || m_type == Type_KParts) {
        delete m_editor;
    }
}

WorkPackage *DocumentChild::parentPackage() const
{
    return static_cast<WorkPackage*>(parent());
}

void DocumentChild::setFileInfo(const QUrl &url)
{
    m_fileinfo.setFile(url.path());
    //debugPlanWork<<url;
    bool res = connect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &DocumentChild::slotDirty);
    //debugPlanWork<<res<<filePath();
#ifndef NDEBUG
    Q_ASSERT(res);
#else
    Q_UNUSED(res);
#endif
    m_fileSystemWatcher->addPath(filePath());
}

void DocumentChild::setModified(bool mod)
{
    debugPlanWork<<mod<<filePath();
    if (m_editormodified != mod) {
        m_editormodified = mod;
        Q_EMIT modified(mod);
    }
}

void DocumentChild::slotDirty(const QString &file)
{
    //debugPlanWork<<filePath()<<file<<m_filemodified;
    if (file == filePath() && ! m_filemodified) {
        debugPlanWork<<file<<"is modified";
        m_filemodified = true;
        Q_EMIT fileModified(true);
    }
}

void DocumentChild::slotUpdateModified()
{
    if (m_type == Type_KParts && m_editor && (m_editor->isModified() != m_editormodified)) {
        setModified(m_editor->isModified());
    }
    QTimer::singleShot(500, this, &DocumentChild::slotUpdateModified);
}

bool DocumentChild::setDoc(const KPlato::Document *doc)
{
    Q_ASSERT (m_doc == nullptr);
    if (isOpen()) {
        KMessageBox::error(nullptr, i18n("Document is already open:<br>%1", doc->url().url()));
        return false;
    }
    m_doc = doc;
    QUrl url;
    if (parentPackage()->newDocuments().contains(doc)) {
        url = parentPackage()->newDocuments().value(doc);
        Q_ASSERT(url.isValid());
        parentPackage()->removeNewDocument(doc);
    } else if (doc->sendAs() == KPlato::Document::SendAs_Copy) {
        url = parentPackage()->extractFile(doc);
        if (url.url().isEmpty()) {
            KMessageBox::error(nullptr, i18n("Could not extract document from storage:<br>%1", doc->url().url()));
            return false;
        }
        m_copy = true;
    } else {
        url = doc->url();
    }
    if (! url.isValid()) {
        KMessageBox::error(nullptr, i18n("Invalid URL:<br>%1", url.url()));
        return false;
    }
    setFileInfo(url);
    return true;
}

bool DocumentChild::openDoc(const KPlato::Document *doc, KoStore *store)
{
    Q_ASSERT (m_doc == nullptr);
    if (isOpen()) {
        KMessageBox::error(nullptr, i18n("Document is already open:<br>%1", doc->url().path()));
        return false;
    }
    m_doc = doc;
    QUrl url;
    if (doc->sendAs() == KPlato::Document::SendAs_Copy) {
        url = parentPackage()->extractFile(doc, store);
        if (url.url().isEmpty()) {
            KMessageBox::error(nullptr, i18n("Could not extract document from storage:<br>%1", doc->url().path()));
            return false;
        }
        m_copy = true;
    } else {
        url = doc->url();
    }
    if (! url.isValid()) {
        KMessageBox::error(nullptr, i18n("Invalid URL:<br>%1", url.url()));
        return false;
    }
    setFileInfo(url);
    return true;
}

bool DocumentChild::editDoc()
{
    Q_ASSERT(m_doc != nullptr);
    debugPlanWork<<"file:"<<filePath();
    if (isOpen()) {
        KMessageBox::error(nullptr, i18n("Document is already open:<br> %1", m_doc->url().path()));
        return false;
    }
    if (! m_fileinfo.exists()) {
        KMessageBox::error(nullptr, i18n("File does not exist:<br>%1", fileName()));
        return false;
    }
    QUrl filename = QUrl::fromLocalFile(filePath());
    const QMimeType mimetype = QMimeDatabase().mimeTypeForUrl(filename);
    KService::Ptr service = KApplicationTrader::preferredService(mimetype.name());
    bool editing = startProcess(service, filename);
    if (editing) {
        m_type = Type_Other; // FIXME: try to be more specific
    }
    return editing;
}

bool DocumentChild::startProcess(KService::Ptr service, const QUrl &url)
{
    QStringList args;
    QList<QUrl> files;
    if (url.isValid()) {
        files << url;
    }
    if (service) {
        KIO::DesktopExecParser parser(*service, files);
        parser.setUrlsAreTempFiles(false);
        args = parser.resultingArguments();
    } else {
        QList<QUrl> list;
        QPointer<KOpenWithDialog> dlg = new KOpenWithDialog(list, i18n("Edit with:"), QString(), nullptr);
        if (dlg->exec() == QDialog::Accepted && dlg){
            args << dlg->text();
        }
        if (args.isEmpty()) {
            debugPlanWork<<"No executable selected";
            return false;
        }
        args << url.url();
        delete dlg;
    }
    debugPlanWork<<args;
    m_process = new KProcess();
    m_process->setProgram(args);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &DocumentChild::slotEditFinished);
    connect(m_process, &KProcess::errorOccurred, this,  &DocumentChild::slotEditError);
    m_process->start();
    //debugPlanWork<<m_process->pid()<<m_process->program();
    return true;
}

bool DocumentChild::isModified() const
{
    return m_editormodified;
}

bool DocumentChild::isFileModified() const
{
    return m_filemodified;
}

void DocumentChild::slotEditFinished(int /*par*/,  QProcess::ExitStatus)
{
    //debugPlanWork<<par<<filePath();
    delete m_process;
    m_process = nullptr;
}

void DocumentChild::slotEditError(QProcess::ProcessError status)
{
    debugPlanWork<<status;
    if (status == QProcess::FailedToStart || status == QProcess::Crashed) {
        m_process->deleteLater();
        m_process = nullptr;
    } else debugPlanWork<<"Error="<<status<<" what to do?";
}

bool DocumentChild::saveToStore(KoStore *store)
{
    debugPlanWork<<filePath();
    m_fileSystemWatcher->removePath(filePath());
    bool ok = false;
    bool wasmod = m_filemodified;
    if (m_type == Type_Calligra || m_type == Type_KParts) {
        if (m_editor->isModified()) {
            ok = m_editor->save(); // hmmmm
        } else {
            ok = true;
        }
    } else if (m_type == Type_Other) {
        if (isOpen()) {
            warnPlanWork<<"External editor open";
        }
        ok = true;
    } else {
        errorPlanWork<<"Unknown document type";
    }
    if (ok) {
        debugPlanWork<<"Add to store:"<<fileName();
        store->addLocalFile(filePath(), fileName());
        m_filemodified = false;
        if (wasmod != m_filemodified) {
            Q_EMIT fileModified(m_filemodified);
        }
    }
    m_fileSystemWatcher->addPath(filePath());
    return ok;
}


//------------------------------------
Part::Part(QWidget *parentWidget, QObject *parent, const QVariantList & /*args*/)
    : KParts::ReadWritePart(parent),
    m_xmlLoader(),
    m_modified(false),
    m_loadingFromProjectStore(false),
    m_undostack(new KUndo2QStack(this)),
    m_nogui(false)
{
    debugPlanWork;
    setComponentName(Factory::global().componentName(), Factory::global().componentDisplayName());
    if (isReadWrite()) {
        setXMLFile(QStringLiteral("calligraplanwork.rc"));
    } else {
        setXMLFile(QStringLiteral("calligraplanwork_readonly.rc"));
    }

    m_view = new View(this, parentWidget, actionCollection());
    setWidget(m_view);
    connect(m_view, &View::viewDocument, this, &Part::viewWorkpackageDocument);

    loadWorkPackages();

    connect(m_undostack, &KUndo2QStack::cleanChanged, this, &Part::setDocumentClean);

}

Part::Part(QObject *parent)
    : KParts::ReadWritePart(parent),
    m_view(nullptr),
    m_xmlLoader(),
    m_modified(false),
    m_loadingFromProjectStore(false),
    m_undostack(new KUndo2QStack(this)),
    m_nogui(true)
{
    debugPlanWork;
    setComponentName(Factory::global().componentName(), Factory::global().componentDisplayName());
    if (isReadWrite()) {
        setXMLFile(QStringLiteral("calligraplanwork.rc"));
    } else {
        setXMLFile(QStringLiteral("calligraplanwork_readonly.rc"));
    }

    connect(m_undostack, &KUndo2QStack::cleanChanged, this, &Part::setDocumentClean);

}

Part::~Part()
{
    debugPlanWork;
//    m_config.save();
    // views must be deleted before packages
    delete m_view;
    qDeleteAll(m_packageMap);
    PlanWorkSettings::self()->save();
}

void Part::addCommand(KUndo2Command *cmd)
{
    if (cmd) {
        m_undostack->push(cmd);
    }
}

bool Part::setWorkPackage(WorkPackage *wp, KoStore *store)
{
    QString id = wp->id();
    debugPlanWork<<wp->name()<<"exists"<<m_packageMap.contains(id);
    if (m_packageMap.contains(id)) {
        if (!m_nogui) {
            if (KMessageBox::warningTwoActions(nullptr,
                                               i18n("<p>The work package already exists in the projects store.</p>"
                                                    "<p>Project: %1<br>Task: %2</p>"
                                                    "<p>Do you want to update the existing package with data from the new?</p>",
                                                    wp->project()->name(), wp->node()->name()),
                                               i18nc("@title:window", "Update Work Package"),
                                               KStandardGuiItem::apply(),
                                               KStandardGuiItem::discard()) == KMessageBox::SecondaryAction) {
                delete wp;
                return false;
            }
        }
        m_packageMap[ id ]->merge(this, wp, store);
        delete wp;
        return true;
    }
    wp->setNoGui(m_nogui);
    wp->setFilePath(m_loadingFromProjectStore ? wp->fileName(this) : localFilePath());
    m_packageMap[ id ] = wp;
    if (! m_loadingFromProjectStore) {
        wp->saveToProjects(this);
    }
    connect(wp->project(), &KPlato::Project::projectChanged, wp, &KPlatoWork::WorkPackage::projectChanged);
    connect (wp, &KPlatoWork::WorkPackage::modified, this, &KPlatoWork::Part::setModified);
    Q_EMIT workPackageAdded(wp, indexOf(wp));
    connect(wp, &WorkPackage::saveWorkPackage, this, &Part::saveWorkPackage);
    return true;
}

void Part::removeWorkPackage(KPlato::Node *node, KPlato::MacroCommand *m)
{
    debugPlanWork<<node->name();
    WorkPackage *wp = findWorkPackage(node);
    if (wp == nullptr) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Remove failed. Cannot find work package"));
        }
        return;
    }
    PackageRemoveCmd *cmd = new PackageRemoveCmd(this, wp, kundo2_i18nc("@action", "Remove work package"));
    if (m) {
        m->addCommand(cmd);
    } else {
        addCommand(cmd);
    }
}

void Part::removeWorkPackages(const QList<KPlato::Node*> &nodes)
{
    debugPlanWork<<nodes;
    KPlato::MacroCommand *m = new KPlato::MacroCommand(kundo2_i18np("Remove work package", "Remove work packages", nodes.count()));
    for (KPlato::Node *n : nodes) {
        removeWorkPackage(n, m);
    }
    if (m->isEmpty()) {
        delete m;
    } else {
        addCommand(m);
    }
}

void Part::removeWorkPackage(WorkPackage *wp)
{
    //debugPlanWork;
    int row = indexOf(wp);
    if (row >= 0) {
        const QList<QString> &lst = m_packageMap.keys();
        const QString &key = lst.value(row);
        m_packageMap.remove(key);
        Q_EMIT workPackageRemoved(wp, row);
    }
}

void Part::addWorkPackage(WorkPackage *wp)
{
    //debugPlanWork;
    QString id = wp->id();
    Q_ASSERT(! m_packageMap.contains(id));
    wp->setNoGui(m_nogui);
    m_packageMap[ id ] = wp;
    Q_EMIT workPackageAdded(wp, indexOf(wp));
}

bool Part::loadWorkPackages()
{
    m_loadingFromProjectStore = true;
    const QStringList lst = KoResourcePaths::findAllResources("projects", QStringLiteral("*.planwork"), KoResourcePaths::Recursive | KoResourcePaths::NoDuplicates);
    debugPlanWork<<lst;
    for (const QString &file : lst) {
        if (! loadNativeFormatFromStore(file)) {
            if (!m_nogui) {
                KMessageBox::information(nullptr, i18n("Failed to load file:<br>%1" , file));
            }
        }
    }
    m_loadingFromProjectStore = false;
    return true;

}

bool Part::loadWorkPackage(const QString& fileName)
{
    m_loadingFromProjectStore = true;
    return loadNativeFormatFromStore(fileName);
}

bool Part::loadNativeFormatFromStore(const QString& file)
{
    debugPlanWork<<file;
    KoStore * store = KoStore::createStore(file, KoStore::Read, "", KoStore::Auto);

    if (store->bad()) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Not a valid work package file:<br>%1", file));
        }
        delete store;
        QApplication::restoreOverrideCursor();
        return false;
    }

    const bool success = loadNativeFormatFromStoreInternal(store);

    delete store;

    return success;
}

bool Part::loadNativeFormatFromStoreInternal(KoStore * store)
{
    if (store->hasFile("root")) {
        KoXmlDocument doc;
        bool ok = loadAndParse(store, QStringLiteral("root"), doc);
        if (ok) {
            ok = loadXML(doc, store);
        }
        if (!ok) {
            QApplication::restoreOverrideCursor();
            return false;
        }

    } else {
        errorPlanWork << "ERROR: No maindoc.xml" << '\n';
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Invalid document. The document does not contain 'maindoc.xml'."));
        }
        QApplication::restoreOverrideCursor();
        return false;
    }
//     if (store->hasFile("documentinfo.xml")) {
//         KoXmlDocument doc;
//         if (oldLoadAndParse(store, "documentinfo.xml", doc)) {
//             d->m_docInfo->load(doc);
//         }
//     } else {
//         //debugPlanWork <<"cannot open document info";
//         delete d->m_docInfo;
//         d->m_docInfo = new KoDocumentInfo(this);
//     }

    bool res = completeLoading(store);
    QApplication::restoreOverrideCursor();
    return res;
}

bool Part::loadAndParse(KoStore* store, const QString& filename, KoXmlDocument& doc)
{
    //debugPlanWork <<"Trying to open" << filename;

    if (!store->open(filename)) {
        warnPlanWork << "Entry " << filename << " not found!";
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Failed to open file: %1", filename));
        }
        return false;
    }
    // Error variables for QDomDocument::setContent
    QString errorMsg;
    int errorLine, errorColumn;
    bool ok = doc.setContent(store->device(), &errorMsg, &errorLine, &errorColumn);
    store->close();
    if (!ok) {
        errorPlanWork << "Parsing error in " << filename << "! Aborting!" << '\n'
        << " In line: " << errorLine << ", column: " << errorColumn << '\n'
        << " Error message: " << errorMsg;
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Parsing error in file '%1' at line %2, column %3<br>Error message: %4", filename  , errorLine, errorColumn ,
                                   QCoreApplication::translate("QXml", errorMsg.toUtf8().constData(), nullptr)));
        }
        return false;
    }
    return true;
}

bool Part::loadXML(const KoXmlDocument &document, KoStore* store)
{
    debugPlanXml;
    QString value;
    KoXmlElement plan = document.documentElement();

    // Check if this is the right app
    value = plan.attribute("mime", QString());
    if (value.isEmpty()) {
        errorPlanWork << "No mime type specified!" << '\n';
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Invalid document. No mimetype specified."));
        }
        return false;
    } else if (value == KPLATO_MIME_TYPE) {
        return loadKPlatoXML(document, store);
    } else if (value != PLANWORK_MIME_TYPE) {
        errorPlanWork << "Unknown mime type " << value;
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Invalid document. Expected mimetype application/x-vnd.kde.plan.work, got %1", value));
        }
        return false;
    }
    m_xmlLoader.setMimetype(plan.attribute("mime"));
    QString syntaxVersion = plan.attribute("version", PLAN_FILE_SYNTAX_VERSION);
    m_xmlLoader.setVersion(syntaxVersion);
    if (syntaxVersion > PLAN_FILE_SYNTAX_VERSION) {
        KMessageBox::ButtonCode ret = KMessageBox::Cancel;
        if (!m_nogui) {
            ret = KMessageBox::warningContinueCancel(
                      nullptr, i18n("This document is a newer version than supported by PlanWork (syntax version: %1)<br>"
                               "Opening it in this version of PlanWork will lose some information.", syntaxVersion),
                      i18n("File-Format Mismatch"), KGuiItem(i18n("Continue")));
        }
        if (ret == KMessageBox::Cancel) {
            return false;
        }
    }
    m_xmlLoader.setVersion(plan.attribute("plan-version", PLAN_FILE_SYNTAX_VERSION));
    m_xmlLoader.startLoad();
    WorkPackage *wp = new WorkPackage(m_loadingFromProjectStore);
    wp->loadXML(plan, m_xmlLoader);
    m_xmlLoader.stopLoad();
    if (!setWorkPackage(wp, store)) {
        // rejected, so nothing changed...
        return true;
    }
    Q_EMIT changed();
    return true;
}

bool Part::loadKPlatoXML(const KoXmlDocument &document, KoStore*)
{
    debugPlanWork;
    QString value;
    KoXmlElement plan = document.documentElement();

    // Check if this is the right app
    value = plan.attribute("mime", QString());
    if (value.isEmpty()) {
        errorPlanWork << "No mime type specified!" << '\n';
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Invalid document. No mimetype specified."));
        }
        return false;
    } else if (value != KPLATOWORK_MIME_TYPE) {
        errorPlanWork << "Unknown mime type " << value;
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Invalid document. Expected mimetype %2, got %1", value, KPLATOWORK_MIME_TYPE));
        }
        return false;
    }
    QString syntaxVersion = plan.attribute("version", KPLATOWORK_MAX_FILE_SYNTAX_VERSION);
    m_xmlLoader.setVersion(syntaxVersion);
    if (syntaxVersion > KPLATOWORK_MAX_FILE_SYNTAX_VERSION) {
        KMessageBox::ButtonCode ret = KMessageBox::Cancel;
        if (!m_nogui) {
            ret = KMessageBox::warningContinueCancel(
                      nullptr, i18n("This document is a newer version than supported by PlanWork (syntax version: %1)<br>"
                               "Opening it in this version of PlanWork will lose some information.", syntaxVersion),
                      i18n("File-Format Mismatch"), KGuiItem(i18n("Continue")));
        }
        if (ret == KMessageBox::Cancel) {
            return false;
        }
    }
    m_xmlLoader.setMimetype(value);
    m_xmlLoader.setVersion(plan.attribute("kplato-version", KPLATO_MAX_FILE_SYNTAX_VERSION));
    m_xmlLoader.startLoad();
    WorkPackage *wp = new WorkPackage(m_loadingFromProjectStore);
    wp->loadKPlatoXML(plan, m_xmlLoader);
    m_xmlLoader.stopLoad();
    if (! setWorkPackage(wp)) {
        // rejected, so nothing changed...
        return true;
    }
    Q_EMIT changed();
    return true;
}

bool Part::completeLoading(KoStore *)
{
    return true;
}

QUrl Part::extractFile(const KPlato::Document *doc)
{
    WorkPackage *wp = findWorkPackage(doc);
    return wp == nullptr ? QUrl() : wp->extractFile(doc);
}

int Part::docType(const KPlato::Document *doc) const
{
    DocumentChild *ch = findChild(doc);
    if (ch == nullptr) {
        return DocumentChild::Type_Unknown;
    }
    return ch->type();
}

DocumentChild *Part::findChild(const KPlato::Document *doc) const
{
    for (const WorkPackage *wp : std::as_const(m_packageMap)) {
        DocumentChild *c = wp->findChild(doc);
        if (c) {
            return c;
        }
    }
    return nullptr;
}

WorkPackage *Part::findWorkPackage(const KPlato::Document *doc) const
{
    for (const WorkPackage *wp : std::as_const(m_packageMap)) {
        if (wp->contains(doc)) {
            return const_cast<WorkPackage*>(wp);
        }
    }
    return nullptr;
}

WorkPackage *Part::findWorkPackage(const DocumentChild *child) const
{
    for (const WorkPackage *wp : std::as_const(m_packageMap)) {
        if (wp->contains(child)) {
            return const_cast<WorkPackage*>(wp);
        }
    }
    return nullptr;
}

WorkPackage *Part::findWorkPackage(const KPlato::Node *node) const
{
    return m_packageMap.value(node->projectNode()->id() + node->id());
}

bool Part::editWorkpackageDocument(const KPlato::Document *doc)
{
    //debugPlanWork<<doc<<doc->url();
    // start in any suitable application
    return editOtherDocument(doc);
}

bool Part::editOtherDocument(const KPlato::Document *doc)
{
    Q_ASSERT(doc != nullptr);
    //debugPlanWork<<doc->url();
    WorkPackage *wp = findWorkPackage(doc);
    if (wp == nullptr) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Edit failed. Cannot find a work package."));
        }
        return false;
    }
    return wp->addChild(this, doc);
}

void Part::viewWorkpackageDocument(KPlato::Document *doc)
{
    debugPlanWork<<doc;
    if (doc == nullptr) {
        return;
    }
    QUrl filename;
    if (doc->sendAs() == KPlato::Document::SendAs_Copy) {
        filename = extractFile(doc);
    } else {
        filename = doc->url();
    }
    // open for view
    viewDocument(filename);
}

bool Part::removeDocument(KPlato::Document *doc)
{
    if (doc == nullptr) {
        return false;
    }
    WorkPackage *wp = findWorkPackage(doc);
    if (wp == nullptr) {
        return false;
    }
    return wp->removeDocument(this, doc);
}

bool Part::viewDocument(const QUrl &filename)
{
    debugPlanWork<<"url:"<<filename;
    if (! filename.isValid()) {
        //KMessageBox::error(0, i18n("Cannot open document. Invalid url: %1", filename.pathOrUrl()));
        return false;
    }

    auto *job = new KIO::OpenUrlJob(filename);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    job->start();

    // auto-deletes by default so no need to delete it
    Q_UNUSED(job);
    return true; //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
}

void Part::setDocumentClean(bool clean)
{
    debugPlanWork<<clean;
    setModified(! clean);
    if (! clean) {
        saveModifiedWorkPackages();
        return;
    }
}

void Part::setModified(bool mod)
{
    KParts::ReadWritePart::setModified(mod);
    Q_EMIT captionChanged(QString(), mod);
}

bool Part::saveAs(const QUrl &/*url*/)
{
    return false;
}

void Part::saveModifiedWorkPackages()
{
    for (WorkPackage *wp : std::as_const(m_packageMap)) {
        if (wp->isModified()) {
            saveWorkPackage(wp);
        }
    }
    m_undostack->setClean();
}

void Part::saveWorkPackage(WorkPackage *wp)
{
    wp->saveToProjects(this);
}

bool Part::saveWorkPackages(bool silent)
{
    debugPlanWork<<silent;
    for (WorkPackage *wp : std::as_const(m_packageMap)) {
        wp->saveToProjects(this);
    }
    m_undostack->setClean();
    return true;
}

bool Part::completeSaving(KoStore *store)
{
    Q_UNUSED(store)
    return true;
}

QDomDocument Part::saveXML()
{
    debugPlanWork;
    return QDomDocument();
}

bool Part::queryClose()
{
    debugPlanWork;
    QList<WorkPackage*> modifiedList;
    for (WorkPackage *wp : std::as_const(m_packageMap)) {
        switch (wp->queryClose(this)) {
            case KMessageBox::SecondaryAction:
                modifiedList << wp;
                break;
            case KMessageBox::Cancel:
                debugPlanWork<<"Cancel";
                return false;
        }
    }
    // closeEvent calls queryClose so modified must be reset or else wps are queried all over again
    for (WorkPackage *wp : std::as_const(modifiedList)) {
        wp->setModified(false);
    }
    setModified(false);
    return true;
}

bool Part::openFile()
{
    debugPlanWork<<localFilePath();
    return loadNativeFormatFromStore(localFilePath());
}

bool Part::saveFile()
{
    return false;
}

void Part::setNoGui(bool nogui)
{
    m_nogui = nogui;
    for (auto wp : std::as_const(m_packageMap)) {
        wp->setNoGui(nogui);
    }
}

}  //KPlatoWork namespace
