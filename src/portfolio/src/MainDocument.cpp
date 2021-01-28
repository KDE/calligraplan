/* This file is part of the KDE project
 * Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * Copyright (C) 2004, 2010, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * Copyright (C) 2007 Thorsten Zachmann <zachmann@kde.org>
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
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
    QList<KoDocument*> docs = documents();
    for (KoDocument *doc : qAsConst(docs)) {
        if (doc->documentPart()->mainwindowCount() > 0) {
            doc->setParent(nullptr);
            for (KoMainWindow *mw : doc->documentPart()->mainWindows()) {
                mw->setNoCleanup(false);
            }
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
    const QList<KoDocument*> docs = documents();
    for (KoDocument *doc : docs) {
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

void MainDocument::setDocumentProperty(KoDocument *doc, const char *name, const QVariant &value)
{
    int index = documents().indexOf(doc);
    bool change = doc->property(name) != value;
    if (change) {
        doc->setProperty(name, value);
        if (!doc->isModified()) {
            doc->setModified(true);
        }
        Q_EMIT documentChanged(doc, index);
    }
}

bool MainDocument::addDocument(KoDocument *newdoc)
{
    const QList<KoDocument*> docs = documents();
    for (const KoDocument *doc : docs) {
        if (doc->project()->id() == newdoc->project()->id()) {
            return false;
        }
    }
    newdoc->setParent(this);
    newdoc->setAutoSave(0);
    connect(newdoc->project(), &KPlato::Project::projectCalculated, this, &MainDocument::slotProjectChanged);
    setModified(true);
    return true;
}

void MainDocument::removeDocument(KoDocument *doc)
{
    const QList<KoDocument*> docs = documents();
    if (docs.contains(doc)) {
        disconnect(doc->project(), &KPlato::Project::projectCalculated, this, &MainDocument::slotProjectChanged);
        doc->setParent(nullptr);
        setModified(true);
    }
}

void MainDocument::slotProjectChanged()
{
    KPlato::Project *project = qobject_cast<KPlato::Project*>(sender());
    if (project) {
        for (KoDocument *doc : documents()) {
            if (doc->project() == project) {
                Q_EMIT projectChanged(doc);
                return;
            }
        }
    }
}

QList<KoDocument*> MainDocument::documents() const
{
    return findChildren<KoDocument*>(QString(), Qt::FindDirectChildrenOnly);
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
