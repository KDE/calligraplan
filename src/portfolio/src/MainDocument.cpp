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
#include "Factory.h"

#include <kptproject.h>
#include <kptschedule.h>

#include <KoStore.h>
#include <KoXmlReader.h>
#include <KoStoreDevice.h>
#include <KoOdfReadStore.h>
#include <KoUpdater.h>
#include <KoProgressUpdater.h>
#include <KoDocumentInfo.h>
#include <KoApplication.h>

#include <KMessageBox>

#include <QStringLiteral>

MainDocument::MainDocument(KoPart *part)
    : KoDocument(part)
{
    Q_ASSERT(part);
}

MainDocument::~MainDocument()
{
    
    for (KoDocument *doc : qAsConst(m_documents)) {
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

bool MainDocument::loadOdf(KoOdfReadStore &odfStore)
{
    //warnPlan<< "OpenDocument not supported, let's try native xml format";
    return loadXML(odfStore.contentDoc(), nullptr); // We have only one format, so try to load that!
}

bool MainDocument::loadXML(const KoXmlDocument &document, KoStore*)
{
    QPointer<KoUpdater> updater;
    if (progressUpdater()) {
        updater = progressUpdater()->startSubtask(1, "Plan::Portfolio::loadXML");
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
        setErrorMessage(i18n("Invalid document. Expected mimetype %1, got %2", QStringLiteral(PLANPORTFOLIO_MIME_TYPE), value));
        return false;
    }
    QString syntaxVersion = portfolio.attribute("version", PLANPORTFOLIO_FILE_SYNTAX_VERSION);
    if (syntaxVersion > PLANPORTFOLIO_FILE_SYNTAX_VERSION) {
        KMessageBox::ButtonCode ret = KMessageBox::warningContinueCancel(
            nullptr, i18n("This document was created with a newer version of Plan (syntax version: %1)\n"
            "Opening it in this version of Plan will lose some information.", syntaxVersion),
            i18n("File-Format Mismatch"), KGuiItem(i18n("Continue")));
        if (ret == KMessageBox::Cancel) {
            setErrorMessage("USER_CANCELED");
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
            connect(doc, &KoDocument::completed, this, &MainDocument::slotProjectDocumentLoaded);
            doc->setUrl(url);
            KPlato::Project *proj = doc->project();
            Q_ASSERT(proj);
            proj->setName(name);
            doc->setProperty(ISPORTFOLIO, p.attribute(QStringLiteral(ISPORTFOLIO)));
            doc->setProperty(SCHEDULEMANAGERNAME, p.attribute(QStringLiteral(SCHEDULEMANAGERNAME)));
            doc->setProperty(SCHEDULINGCONTROL, p.attribute(QStringLiteral(SCHEDULINGCONTROL)));
            doc->setProperty(SCHEDULINGPRIORITY, p.attribute(QStringLiteral(SCHEDULINGPRIORITY)));
            addDocument(doc);
            doc->setProperty(BLOCKSHAREDPROJECTSLOADING, true);
            doc->openUrl(url);
        }
    }
    if (updater) {
        updater->setProgress(100); // the rest is only processing, not loading
    }
    return true;
}

void MainDocument::slotProjectDocumentLoaded()
{
    KoDocument *doc = qobject_cast<KoDocument*>(sender());
    if (doc) {
        disconnect(doc, &KoDocument::completed, this, &MainDocument::slotProjectDocumentLoaded);
        if (!doc->project()->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString())) {
            KPlato::ScheduleManager *sm = findBestScheduleManager(doc);
            if (sm) {
                setDocumentProperty(doc, SCHEDULEMANAGERNAME, sm->name());
            }
        }
    }
}

bool MainDocument::completeLoading(KoStore *store)
{
    setModified(false);
    Q_EMIT changed();
    return true;
}

QDomDocument createDocument() {
    QDomDocument document("portfolio");
    document.appendChild(document.createProcessingInstruction(
        "xml",
        "version=\"1.0\" encoding=\"UTF-8\"") );

    QDomElement doc = document.createElement("portfolio");
    doc.setAttribute("editor", "PlanPortfolio");
    doc.setAttribute("mime", PLANPORTFOLIO_MIME_TYPE);
    doc.setAttribute("version", PLANPORTFOLIO_FILE_SYNTAX_VERSION);
    document.appendChild(doc);
    return document;
}

QDomDocument MainDocument::saveXML()
{
    //debugPlan;
    QDomDocument document = createDocument();
    QDomElement portfolio = document.documentElement();
    QDomElement projects = document.createElement("projects");
    portfolio.appendChild(projects);
    for (KoDocument *doc : qAsConst(m_documents)) {
        QDomElement p = document.createElement("project");
        p.setAttribute(QStringLiteral(SCHEDULEMANAGERNAME), doc->property(SCHEDULEMANAGERNAME).toString());
        p.setAttribute(QStringLiteral(ISPORTFOLIO), doc->property(ISPORTFOLIO).toBool());
        p.setAttribute(QStringLiteral(SCHEDULINGCONTROL), doc->property(SCHEDULINGCONTROL).toString());
        p.setAttribute(QStringLiteral(SCHEDULINGPRIORITY), doc->property(SCHEDULINGPRIORITY).toString());
        p.setAttribute(QStringLiteral("url"), QString(doc->url().toEncoded()));
        p.setAttribute(QStringLiteral("name"), doc->projectName());
        projects.appendChild(p);
    }
    return document;
}

bool MainDocument::completeSaving(KoStore *store)
{
    return true;
}

bool MainDocument::isLoading() const
{
    return KoDocument::isLoading();
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

void MainDocument::setDocumentProperty(KoDocument *doc, const char *name, const QVariant &value)
{
    int index = m_documents.indexOf(doc);
    bool change = doc->property(name) != value;
    if (change) {
        doc->setProperty(name, value);
        if (!doc->isModified()) {
            doc->setModified(true);
        }
        Q_EMIT documentChanged(doc, index);
        if (name == SCHEDULEMANAGERNAME) {
            Q_EMIT doc->scheduleManagerChanged(doc->project()->findScheduleManagerByName(value.toString()));
        }
    }
}

bool MainDocument::addDocument(KoDocument *newdoc)
{
    Q_ASSERT(!m_documents.contains(newdoc));
    if (m_documents.contains(newdoc)) {
        return false;
    }
    Q_EMIT documentAboutToBeInserted(m_documents.count());
    m_documents << newdoc;
    newdoc->setAutoSave(0);
    connect(newdoc->project(), &KPlato::Project::projectCalculated, this, &MainDocument::slotProjectChanged);
    Q_EMIT documentInserted();
    setModified(true);
    return true;
}

void MainDocument::removeDocument(KoDocument *doc)
{
    if (m_documents.contains(doc)) {
        Q_EMIT documentAboutToBeRemoved(m_documents.indexOf(doc));
        disconnect(doc->project(), &KPlato::Project::projectCalculated, this, &MainDocument::slotProjectChanged);
        m_documents.removeOne(doc);
        Q_EMIT documentRemoved();
        setModified(true);
    }
}

void MainDocument::slotProjectChanged()
{
    KPlato::Project *project = qobject_cast<KPlato::Project*>(sender());
    if (project) {
        for (KoDocument *doc : qAsConst(m_documents)) {
            if (doc->project() == project) {
                Q_EMIT projectChanged(doc);
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
            sm = m; // the latest sceduled
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
