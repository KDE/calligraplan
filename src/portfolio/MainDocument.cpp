/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * SPDX-FileCopyrightText: 2004, 2010, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "MainDocument.h"
#include "Part.h"
#include "View.h"

#include <kptproject.h>
#include <kptschedule.h>

#include <KoStore.h>
#include <KoStoreDevice.h>
#include <KoOdfReadStore.h>
#include <KoUpdater.h>
#include <KoProgressUpdater.h>
#include <KoDocumentInfo.h>
#include <KoApplication.h>
#include <ExtraProperties.h>

#include <KMessageBox>

#include <QStringLiteral>
#include <QtGlobal>

MainDocument::MainDocument(KoPart *part)
    : KoDocument(part)
{
    Q_ASSERT(part);
    setAlwaysAllowSaving(true);
}

MainDocument::~MainDocument()
{
    for (KoDocument *doc : std::as_const(m_documents)) {
        if (doc->documentPart()->mainwindowCount() > 0) {
            // The doc has been opened in a separate window
            for (KoMainWindow *mw : doc->documentPart()->mainWindows()) {
                mw->setNoCleanup(false); // mw will delete the doc
            }
        } else {
            delete doc;
        }
    }
}

void MainDocument::initEmpty()
{
    KoDocument::initEmpty();
}


void MainDocument::setReadWrite(bool rw)
{
    KoDocument::setReadWrite(rw);
}

const KoXmlDocument& MainDocument::xmlDocument() const
{
    return m_xmlDocument;
}

bool MainDocument::loadOdf(KoOdfReadStore &odfStore)
{
    //warnPlan<< "OpenDocument not supported, let's try native xml format";
    return loadXML(odfStore.contentDoc(), nullptr); // We have only one format, so try to load that!
}

bool MainDocument::loadXML(const KoXmlDocument &document, KoStore*)
{
    m_xmlDocument = document;
    QPointer<KoUpdater> updater;
    if (progressUpdater()) {
        updater = progressUpdater()->startSubtask(1, QStringLiteral("Plan::Portfolio::loadXML"));
        updater->setProgress(0);
    }
    KoXmlElement portfolio = document.documentElement();
    // Check if this is the right app
    QString value = portfolio.attribute("mime", QString());
    if (value.isEmpty()) {
//         errorPlan << "No mime type specified!";
        setErrorMessage(i18n("Invalid document. No mimetype specified."));
        return false;
    }
    if (value != PLANPORTFOLIO_MIME_TYPE) {
//         errorPlan << "Unknown mime type " << value;
        setErrorMessage(i18n("Invalid document. Expected mimetype %1, got %2", PLANPORTFOLIO_MIME_TYPE, value));
        return false;
    }
    QString syntaxVersion = portfolio.attribute("version", PLANPORTFOLIO_FILE_SYNTAX_VERSION);
    if (syntaxVersion > PLANPORTFOLIO_FILE_SYNTAX_VERSION) {
        KMessageBox::ButtonCode ret = KMessageBox::warningContinueCancel(
            nullptr, i18n("This document was created with a newer version of Plan (syntax version: %1)\n"
            "Opening it in this version of Plan will lose some information.", syntaxVersion),
            i18n("File-Format Mismatch"), KGuiItem(i18n("Continue")));
        if (ret == KMessageBox::Cancel) {
            setErrorMessage(QStringLiteral("USER_CANCELED"));
            return false;
        }
    }
    KoXmlElement projects = portfolio.namedItem("projects").toElement();
    int step = 100 / (projects.childNodesCount() + 1);
    int progress = 0;
    KoXmlElement p;
    forEachElement(p, projects) {
        if (updater) {
            progress += step;
            updater->setProgress(progress);
        }
        QString name = p.attribute("name");
        QUrl url = QUrl::fromUserInput(p.attribute("url"));
        if (url.isValid()) {
            KoPart *part = koApp->getPartFromUrl(url);
            Q_ASSERT(part);
            KoDocument *doc = part->createDocument(part);
            doc->setAutoSave(0);
            doc->setUrl(url);
            KPlato::Project *proj = doc->project();
            Q_ASSERT(proj);
            proj->setName(name);
            doc->setProperty(ISPORTFOLIO, p.attribute(QStringLiteral(ISPORTFOLIO)).toInt());
            doc->setProperty(SCHEDULEMANAGERNAME, p.attribute(QStringLiteral(SCHEDULEMANAGERNAME)));
            doc->setProperty(SCHEDULINGCONTROL, p.attribute(QStringLiteral(SCHEDULINGCONTROL)));
            doc->setProperty(SCHEDULINGPRIORITY, p.attribute(QStringLiteral(SCHEDULINGPRIORITY)).toInt());
            if (p.hasAttribute(SAVEEMBEDDED)) {
                doc->setProperty(SAVEEMBEDDED, p.attribute(QStringLiteral(SAVEEMBEDDED)).toInt());
                doc->setProperty(EMBEDDEDURL, p.attribute(QStringLiteral(EMBEDDEDURL)));
            }
            doc->setProperty(BLOCKSHAREDPROJECTSLOADING, true);
            if (!addDocument(doc)) {
                // should not happen
                Q_ASSERT_X(false, Q_FUNC_INFO, "Document already exists.");
                doc->deleteLater();
            }
        } else {
            qWarning()<<Q_FUNC_INFO<<"Invalid url:"<<url;
        }
    }
    if (updater) {
        updater->setProgress(100); // the rest is only processing, not loading
    }
    return true;
}

void MainDocument::slotProjectDocumentLoaded()
{
    KoDocument *document = qobject_cast<KoDocument*>(sender());
    if (document) {
        disconnect(document, &KoDocument::completed, this, &MainDocument::slotProjectDocumentLoaded);
        disconnect(document, &KoDocument::canceled, this, &MainDocument::slotProjectDocumentCanceled);
        // remove if duplicate
        for (const auto doc : std::as_const(m_documents)) {
            if (document == doc) {
                continue;
            }
            if (document->project()->id() == doc->project()->id()) {
                KMessageBox::error(nullptr,
                                   xi18nc("@info", "The project already exists.<nl/>Project: %1<nl/>Document: %2", document->project()->name(), document->url().toDisplayString()),
                                   i18nc("@title:window", "Could not add project"));

                m_documents.removeOne(document);
                document->deleteLater();;
                return;
            }
        }
        if (!document->project()->findScheduleManagerByName(document->property(SCHEDULEMANAGERNAME).toString())) {
            KPlato::ScheduleManager *sm = findBestScheduleManager(document);
            if (sm) {
                document->setProperty(SCHEDULEMANAGERNAME, sm->name());
            }
        }
        const auto managers = document->project()->allScheduleManagers();
        for (const auto m : managers) {
            m->setProperty(ORIGINALSCHEDULEMANAGER, true);
        }
        document->setModified(false);
    }
}

void MainDocument::slotProjectDocumentCanceled()
{
    KoDocument *document = qobject_cast<KoDocument*>(sender());
    if (document) {
        disconnect(document, &KoDocument::completed, this, &MainDocument::slotProjectDocumentLoaded);
        disconnect(document, &KoDocument::canceled, this, &MainDocument::slotProjectDocumentCanceled);
        document->setProperty(STATUS, QStringLiteral("loading-error"));
    }
}

bool MainDocument::completeLoading(KoStore *store)
{
    setModified(false);
    for (auto doc : std::as_const(m_documents)) {
        connect(doc, &KoDocument::completed, this, &MainDocument::slotProjectDocumentLoaded);
        connect(doc, &KoDocument::canceled, this, &MainDocument::slotProjectDocumentCanceled);
        if (doc->property(SAVEEMBEDDED).toBool()) {
            const auto url = doc->url();
            doc->loadEmbeddedDocument(store, doc->property(EMBEDDEDURL).toString());
            doc->setUrl(url); // restore external url
        } else {
            doc->openUrl(doc->url());
        }
    }
    Q_EMIT changed();
    return true;
}

QDomDocument createDocument() {
    QDomDocument document(QStringLiteral("portfolio"));
    document.appendChild(document.createProcessingInstruction(
        QStringLiteral("xml"),
        QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));

    QDomElement doc = document.createElement(QStringLiteral("portfolio"));
    doc.setAttribute(QStringLiteral("editor"), QStringLiteral("PlanPortfolio"));
    doc.setAttribute(QStringLiteral("mime"), PLANPORTFOLIO_MIME_TYPE);
    doc.setAttribute(QStringLiteral("version"), PLANPORTFOLIO_FILE_SYNTAX_VERSION);
    document.appendChild(doc);
    return document;
}

QDomDocument MainDocument::saveXML()
{
    //debugPlan;
    QDomDocument document = createDocument();
    QDomElement portfolio = document.documentElement();
    QDomElement projects = document.createElement(QStringLiteral("projects"));
    portfolio.appendChild(projects);
    int count = 1;
    for (KoDocument *doc : std::as_const(m_documents)) {
        QDomElement p = document.createElement(QStringLiteral("project"));
        p.setAttribute(QStringLiteral(SCHEDULEMANAGERNAME), doc->property(SCHEDULEMANAGERNAME).toString());
        p.setAttribute(QStringLiteral(ISPORTFOLIO), doc->property(ISPORTFOLIO).toBool() ? 1 : 0);
        p.setAttribute(QStringLiteral(SCHEDULINGCONTROL), doc->property(SCHEDULINGCONTROL).toString());
        p.setAttribute(QStringLiteral(SCHEDULINGPRIORITY), doc->property(SCHEDULINGPRIORITY).toString());
        if (doc->property(SAVEEMBEDDED).toBool()) {
            p.setAttribute(QStringLiteral(SAVEEMBEDDED), doc->property(SAVEEMBEDDED).toBool() ? 1 : 0);
            const QString s = QStringLiteral("Projects/Project_") + QString::number(count++);
            doc->setProperty(EMBEDDEDURL, s);
            p.setAttribute(QStringLiteral(EMBEDDEDURL), s);
        }
        p.setAttribute(QStringLiteral("url"), QLatin1String(doc->url().toEncoded()));
        p.setAttribute(QStringLiteral("name"), doc->projectName());
        projects.appendChild(p);
    }
    Q_EMIT saveSettings(document);
    return document;
}

bool MainDocument::completeSaving(KoStore *store)
{
    for (KoDocument *doc : std::as_const(m_documents)) {
        if (doc->property(SAVEEMBEDDED).toBool()) {
            doc->setAlwaysAllowSaving(true);
            saveDocumentToStore(store, doc);
        }
    }
    return true;
}

bool MainDocument::isLoading() const
{
    return KoDocument::isLoading();
}

bool MainDocument::isModified() const
{
    if (KoDocument::isModified()) {
        return true;
    }
    for (const auto child : std::as_const(m_documents)) {
        if (child->isModified() && child->property(SAVEEMBEDDED).toBool()) {
            return true;
        }
    }
    return false;
}

// For embedded documents
bool MainDocument::loadFromStore(KoStore *_store, const QString& url)
{
    QUrl externUrl = this->url();
    bool ret = KoDocument::loadFromStore(_store, url);
    setUrl(externUrl);
    return ret;
}

// Called for embedded documents
bool MainDocument::saveDocumentToStore(KoStore *store, KoDocument *doc)
{
    auto path = doc->property(EMBEDDEDURL).toString();
    //qInfo()<<Q_FUNC_INFO<<doc<<path;
    path.prepend(QStringLiteral("tar:/"));
    // In the current directory we're the king :-)
    store->pushDirectory();
    if (store->open(path)) {
        KoStoreDevice dev(store);

        QDomDocument qdoc = doc->saveXML();
        // Save to buffer
        QByteArray s = qdoc.toByteArray(); // utf8 already
        if (!dev.open(QIODevice::WriteOnly)) {
            //qInfo()<<Q_FUNC_INFO << "Failed to open device";
        }
        int nwritten = dev.write(s.data(), s.size());
        if (nwritten != (int)s.size()) {
            //qInfo()<<Q_FUNC_INFO << "Failed: wrote " << nwritten << "- expected" <<  s.size();
            store->close();
            return false;
        }
        if (!store->close()) {
            //qInfo() << Q_FUNC_INFO << "Failed to close store";
            return false;
        }
    } else {
        //qInfo() << Q_FUNC_INFO << "Failed to save document to store";
        return false;
    }
/*
    if (!completeSaving(_store))
        return false;*/

    // Now that we're done leave the directory again
    store->popDirectory();

    //qInfo() << Q_FUNC_INFO << "Saved document to store";

    return true;
}

void MainDocument::setModified(bool mod)
{
    KoDocument::setModified(mod);
    Q_EMIT changed();
}

void MainDocument::emitDocumentChanged(KoDocument *doc)
{
    Q_EMIT documentChanged(doc, m_documents.indexOf(doc));
}

bool MainDocument::isEqual(const char *s1, const char *s2) const
{
    return (strcmp(s1, s2) == 0);
}

bool MainDocument::setDocumentProperty(KoDocument *doc, const char *name, const QVariant &value)
{
    bool changed = doc->property(name) != value;
    if (changed) {
        doc->setProperty(name, value);
        Q_EMIT documentChanged(doc, m_documents.indexOf(doc));
        if (isEqual(name, SCHEDULEMANAGERNAME)) {
            Q_EMIT doc->scheduleManagerChanged(doc->project()->findScheduleManagerByName(value.toString()));
        }
        // These are actually properties of the main document
        if (isEqual(name, ISPORTFOLIO)
            || isEqual(name, SCHEDULEMANAGERNAME)
            || isEqual(name, SCHEDULINGCONTROL)
            || isEqual(name, SCHEDULINGPRIORITY)
            || isEqual(name, SAVEEMBEDDED))
        {
            setModified(true);
        }
    }
    return changed;
}

bool MainDocument::addDocument(KoDocument *newdoc)
{
    Q_ASSERT(!m_documents.contains(newdoc));
    if (m_documents.contains(newdoc)) {
        return false;
    }
    for (const auto doc : std::as_const(m_documents)) {
        if (doc->project()->id() == newdoc->project()->id()) {
            return false;
        }
    }
    Q_EMIT documentAboutToBeInserted(m_documents.count());
    m_documents << newdoc;
    newdoc->setAutoSave(0);
    connect(newdoc, &KoDocument::modified, this, &MainDocument::slotDocumentModified);
    connect(newdoc->project(), &KPlato::Project::projectChanged, this, &MainDocument::slotProjectChanged);
    Q_EMIT documentInserted();
    setModified(true);
    return true;
}

void MainDocument::removeDocument(KoDocument *doc)
{
    if (m_documents.contains(doc)) {
        Q_EMIT documentAboutToBeRemoved(m_documents.indexOf(doc));
        disconnect(doc->project(), &KPlato::Project::projectChanged, this, &MainDocument::slotProjectChanged);
        m_documents.removeOne(doc);
        delete doc;
        Q_EMIT documentRemoved();
        setModified(true);
    }
}

void MainDocument::slotDocumentModified(bool mod)
{
    if (mod) {
        Q_EMIT documentModified();
    }
}

void MainDocument::slotProjectChanged()
{
    KPlato::Project *project = qobject_cast<KPlato::Project*>(sender());
    if (project) {
        for (KoDocument *doc : std::as_const(m_documents)) {
            if (doc->project() == project) {
                Q_EMIT projectChanged(doc);
                doc->setModified(true);
                return;
            }
        }
    }
}

QList<KoDocument*> MainDocument::documents() const
{
    return m_documents;
}

void MainDocument::emitChanged()
{
    Q_EMIT changed();
}

KPlato::ScheduleManager *MainDocument::scheduleManager(const KoDocument *doc) const
{
    auto manager = doc->project()->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
    if (!manager) {
        manager = findBestScheduleManager(doc);
    }
    return manager;
}

KPlato::ScheduleManager *MainDocument::findBestScheduleManager(const KoDocument *doc) const
{
    const QList<KPlato::ScheduleManager*> lst = doc->project()->allScheduleManagers();
    KPlato::ScheduleManager *sm = lst.value(0);
    for (KPlato::ScheduleManager *m : lst) {
        if (m->isBaselined()) {
            sm = m;
            break;
        }
        if (m->isScheduled()) {
            sm = m; // the latest scheduled
        }
    }
    return sm;
}

KPlato::SchedulerPlugin *MainDocument::schedulerPlugin(const QString &key) const
{
    return schedulerPlugins().value(key);
}

QMap<QString, KPlato::SchedulerPlugin*> MainDocument::schedulerPlugins() const
{
    // FIXME
    if (m_documents.isEmpty()) {
        return QMap<QString, KPlato::SchedulerPlugin*>();
    }
    return m_documents.first()->schedulerPlugins();
}

bool MainDocument::isChildrenModified() const
{
    for (const auto doc : std::as_const(m_documents)) {
        if (doc->isModified()) {
            return true;
        }
    }
    return false;
}
