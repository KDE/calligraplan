/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2009, 2012 Dag Andersen <dag.andersen@kdemail.net>
 SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 
 SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "workpackage.h"

#include "plan/KPlatoXmlLoader.h" //NOTE: this file should probably be moved

#include "part.h"
#include "kptglobal.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kptdocuments.h"
#include "kptcommand.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include "kptconfigbase.h"
#include "kptcommonstrings.h"

#include <ProjectLoaderBase.h>
#include <MimeTypes.h>

#include <KoStore.h>
#include <KoXmlReader.h>
#include <KoStoreDevice.h>
#include <KoResourcePaths.h>

#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QDateTime>
#include <QDomDocument>

#include <KMessageBox>


#include "debugarea.h"

namespace KPlatoWork
{

WorkPackage::WorkPackage(bool fromProjectStore)
    : m_project(new KPlato::Project()),
    m_fromProjectStore(fromProjectStore),
    m_modified(false),
    m_nogui(false)
{
    m_project->setConfig(&m_config);
}

WorkPackage::WorkPackage(KPlato::Project *project, bool fromProjectStore)
    : m_project(project),
    m_fromProjectStore(fromProjectStore),
    m_modified(false),
    m_nogui(false)
{
    Q_ASSERT(project);
    Q_ASSERT (project->childNode(0));

    m_project->setConfig(&m_config);

    if (! project->scheduleManagers().isEmpty()) {
        // should be only one manager, so just get the first
        const QList<KPlato::ScheduleManager*> &lst = m_project->scheduleManagers();
        project->setCurrentSchedule(lst.first()->scheduleId());
    }
    connect(project, &KPlato::Project::projectChanged, this, &WorkPackage::projectChanged);

}

WorkPackage::~WorkPackage()
{
    delete m_project;
    qDeleteAll(m_childdocs);
}

void WorkPackage::setSettings(const KPlato::WorkPackageSettings &settings)
{
    if (m_settings != settings) {
        m_settings = settings;
        setModified(true);
    }
}

//TODO find a way to know when changes are undone
void WorkPackage::projectChanged()
{
    debugPlanWork;
    setModified(true);
}

bool WorkPackage::addChild(Part *part, const KPlato::Document *doc)
{
    Q_UNUSED(part)
    DocumentChild *ch = findChild(doc);
    if (ch) {
        if (ch->isOpen()) {
            if (!m_nogui) {
                KMessageBox::error(nullptr, i18n("Document is already open"));
            }
            return false;
        }
    } else {
        ch = new DocumentChild(this);
        if (! ch->setDoc(doc)) {
            delete ch;
            return false;
        }
    }
    if (! ch->editDoc()) {
        delete ch;
        return false;
    }
    if (! m_childdocs.contains(ch)) {
        m_childdocs.append(ch);
        connect(ch, &DocumentChild::fileModified, this, &WorkPackage::slotChildModified);
    }
    return true;
}

void WorkPackage::slotChildModified(bool mod)
{
    debugPlanWork<<mod;
    Q_EMIT modified(isModified());
    Q_EMIT saveWorkPackage(this);
}

void WorkPackage::removeChild(DocumentChild *child)
{
    disconnect(child, &DocumentChild::fileModified, this, &WorkPackage::slotChildModified);

    int i = m_childdocs.indexOf(child);
    if (i != -1) {
        // TODO: process etc
        m_childdocs.removeAt(i);
        delete child;
    } else {
        warnPlanWork<<"Could not find document child";
    }
}

bool WorkPackage::contains(const KPlato::Document *doc) const
{
    return node() ? node()->documents().contains(doc) : false;
}

DocumentChild *WorkPackage::findChild(const KPlato::Document *doc) const
{
    for (DocumentChild *c : std::as_const(m_childdocs)) {
        if (c->doc() == doc) {
            return c;
        }
    }
    return nullptr;
}

bool WorkPackage::loadXML(const KoXmlElement &element, KPlato::XMLLoaderObject &status)
{
    QString wbsCode = QStringLiteral("Unknown");
    bool ok = status.loadProject(m_project, element.ownerDocument());
    if (!ok) {
        warnPlanXml<<"Failed to load project";
        status.addMsg(KPlato::XMLLoaderObject::Errors, QStringLiteral("Loading of work package failed"));
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Failed to load project: %1" , m_project->name()));
        }
    } else {
        KoXmlElement te = element.namedItem("task").toElement();
        if (!te.isNull()) {
            wbsCode = te.attribute("wbs", QStringLiteral("empty"));
        }
    }
    if (ok) {
        KoXmlElement e = element.namedItem(QStringLiteral("workpackage")).toElement();
        debugPlanWork<<e.tagName();
        if (e.tagName() == QStringLiteral("workpackage")) {
            KPlato::Task *t = static_cast<KPlato::Task*>(m_project->childNode(0));
            t->workPackage().setOwnerName(e.attribute("owner"));
            t->workPackage().setOwnerId(e.attribute("owner-id"));
            m_sendUrl = QUrl(e.attribute("save-url"));
            m_fetchUrl = QUrl(e.attribute("load-url"));
            m_wbsCode = wbsCode;

            KPlato::Resource *r = m_project->findResource(t->workPackage().ownerId());
            if (r == nullptr) {
                warnPlanWork<<"Cannot find resource id!"<<t->workPackage().ownerId()<<t->workPackage().ownerName();
            }
            debugPlanWork<<"is this me?"<<t->workPackage().ownerName();
            auto ch = e.namedItem(QStringLiteral("settings")).toElement();
            if (!ch.isNull()) {
                m_settings.loadXML(ch);
            }
        }
    }
    if (! m_project->scheduleManagers().isEmpty()) {
        // should be only one manager
        const QList<KPlato::ScheduleManager*> &lst = m_project->scheduleManagers();
        m_project->setCurrentSchedule(lst.first()->scheduleId());
    }
    return ok;
}

bool WorkPackage::loadKPlatoXML(const KoXmlElement &element, KPlato::XMLLoaderObject &status)
{
    bool ok = false;
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        debugPlanWork<<e.tagName();
        if (e.tagName() == QStringLiteral("project")) {
            status.setProject(m_project);
            KPlato::KPlatoXmlLoader loader(status, m_project);
            debugPlanWork<<"loading new project";
            if (! (ok = loader.load(m_project, e, status))) {
                status.addMsg(KPlato::XMLLoaderObject::Errors, QStringLiteral("Loading of work package failed"));
                if (!m_nogui) {
                    KMessageBox::error(nullptr, i18n("Failed to load project: %1" , m_project->name()));
                }
            }
        }
    }
    if (ok) {
        KoXmlNode n = element.firstChild();
        for (; ! n.isNull(); n = n.nextSibling()) {
            if (! n.isElement()) {
                continue;
            }
            KoXmlElement e = n.toElement();
            debugPlanWork<<e.tagName();
            if (e.tagName() == QStringLiteral("workpackage")) {
                KPlato::Task *t = static_cast<KPlato::Task*>(m_project->childNode(0));
                t->workPackage().setOwnerName(e.attribute("owner"));
                t->workPackage().setOwnerId(e.attribute("owner-id"));

                KPlato::Resource *r = m_project->findResource(t->workPackage().ownerId());
                if (r == nullptr) {
                    debugPlanWork<<"Cannot find resource id!!"<<t->workPackage().ownerId()<<t->workPackage().ownerName();
                }
                debugPlanWork<<"is this me?"<<t->workPackage().ownerName();
                KoXmlNode ch = e.firstChild();
                for (; ! ch.isNull(); ch = ch.nextSibling()) {
                    if (! ch.isElement()) {
                        continue;
                    }
                    KoXmlElement el = ch.toElement();
                    debugPlanWork<<el.tagName();
                    if (el.tagName() == QStringLiteral("settings")) {
                        m_settings.loadXML(el);
                    }
                }
            }
        }
    }
    if (! m_project->scheduleManagers().isEmpty()) {
        // should be only one manager
        const QList<KPlato::ScheduleManager*> &lst = m_project->scheduleManagers();
        m_project->setCurrentSchedule(lst.first()->scheduleId());
    }
    return ok;
}

bool WorkPackage::saveToStream(QIODevice * dev)
{
    QDomDocument doc = saveXML();
    // Save to buffer
    QByteArray s = doc.toByteArray(); // utf8 already
    dev->open(QIODevice::WriteOnly);
    int nwritten = dev->write(s.data(), s.size());
    if (nwritten != (int)s.size())
        warnPlanWork << "wrote " << nwritten << "- expected" <<  s.size();
    return nwritten == (int)s.size();
}

bool WorkPackage::saveNativeFormat(Part *part, const QString &path)
{
    Q_UNUSED(part)
    if (path.isEmpty()) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Cannot save to empty filename"));
        }
        return false;
    }
    debugPlanWork<<node()->name()<<path;
    KoStore* store = KoStore::createStore(path, KoStore::Write, "application/x-vnd.kde.plan.work", KoStore::Auto);
    if (store->bad()) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Could not create the file for saving"));
        }
        delete store;
        return false;
    }
    if (store->open("root")) {
        KoStoreDevice dev(store);
        if (! saveToStream(&dev) || ! store->close()) {
            debugPlanWork << "saveToStream failed";
            delete store;
            return false;
        }
    } else {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Not able to write '%1'. Partition full?", QStringLiteral("maindoc.xml")));
        }
        delete store;
        return false;
    }

    if (!completeSaving(store)) {
        delete store;
        return false;
    }
    if (!store->finalize()) {
        delete store;
        return false;
    }
    // Success
    delete store;
    m_modified = false;
    return true;
}

bool WorkPackage::completeSaving(KoStore *store)
{
    debugPlanWork;
    KoStore *oldstore = KoStore::createStore(filePath(), KoStore::Read, QByteArray(""), KoStore::Zip);
    if (oldstore->bad()) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Failed to open store:\n %1", filePath()));
        }
        return false;
    }
    if (oldstore->hasFile("documentinfo.xml")) {
        copyFile(oldstore, store, QStringLiteral("documentinfo.xml"));
    }
    if (oldstore->hasFile("preview.png")) {
        copyFile(oldstore, store, QStringLiteral("preview.png"));
    }

    // First get all open documents
    debugPlanWork<<m_childdocs.count();
    for (DocumentChild *cd : std::as_const(m_childdocs)) {
        if (! cd->saveToStore(store)) {
        }
    }
    // Then get new files
    const QList<KPlato::Document*> documents = node()->documents().documents();
    for (const KPlato::Document *doc : documents) {
        if (m_newdocs.contains(doc)) {
            store->addLocalFile(m_newdocs[ doc ].path(), doc->url().fileName());
            m_newdocs.remove(doc);
            // TODO remove temp file ??
        }
    }
    // Then get files from the old store copied to the new store
    for (KPlato::Document *doc : documents) {
        if (doc->sendAs() != KPlato::Document::SendAs_Copy) {
            continue;
        }
        if (! store->hasFile(doc->url().fileName())) {
            copyFile(oldstore, store, doc->url().fileName());
        }
    }
    return true;
}

QString WorkPackage::fileName(const Part *part) const
{
    Q_UNUSED(part);
    if (m_project == nullptr) {
        warnPlanWork<<"No project in this package";
        return QString();
    }
    KPlato::Node *n = node();
    if (n == nullptr) {
        warnPlanWork<<"No node in this project";
        return QString();
    }
    QString projectName = m_project->name().remove(QLatin1Char(' '));
    // FIXME: workaround: KoResourcePaths::saveLocation("projects", QStringLiteral(projectName) + QLatin1Char('/'));
    const QString path = KoResourcePaths::saveLocation("appdata", QStringLiteral("projects/") + projectName + QLatin1Char('/'));
    QString wpName = n->name();
    wpName = QString(wpName.remove(QLatin1Char(' ')).replace(QLatin1Char('/'), QLatin1Char('_')) + QLatin1Char('_') + n->id() + QStringLiteral(".planwork"));
    return path + wpName;
}

void WorkPackage::removeFile()
{
    QFile file(m_filePath);
    if (! file.exists()) {
        warnPlanWork<<"No project in this package";
        return;
    }
    file.remove();
}

void WorkPackage::saveToProjects(Part *part)
{
    debugPlanWork;
    QString path = fileName(part);
    debugPlanWork<<node()->name();
    if (saveNativeFormat(part, path)) {
        m_fromProjectStore = true;
        m_filePath = path;
    } else {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Cannot save to projects store:\n%1" , path));
        }
    }
    return;
}

bool WorkPackage::isModified() const
{
    if (m_modified) {
        return true;
    }
    for (DocumentChild *ch : std::as_const(m_childdocs)) {
        if (ch->isModified() || ch->isFileModified()) {
            return true;
        }
    }
    return false;
}

QString WorkPackage::name() const
{
    KPlato::Task *t = task();
    return t ? t->name() : QString();
}

KPlato::Node *WorkPackage::node() const
{
    return m_project == nullptr ? nullptr : m_project->childNode(0);
}

KPlato::Task *WorkPackage::task() const
{
    KPlato::Task *task = qobject_cast<KPlato::Task*>(node());
    Q_ASSERT(task);
    return task;
}

bool WorkPackage::removeDocument(Part *part, KPlato::Document *doc)
{
    KPlato::Node *n = node();
    if (n == nullptr) {
        return false;
    }
    part->addCommand(new KPlato::DocumentRemoveCmd(n->documents(), doc, kundo2_i18n("Remove document")));
    return true;
}

bool WorkPackage::copyFile(KoStore *from, KoStore *to, const QString &filename)
{
    QByteArray data;
    if (! from->extractFile(filename , data)) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Failed read file:\n %1", filename));
        }
        return false;
    }
    if (! to->addDataToFile(data, filename)) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Failed write file:\n %1", filename));
        }
        return false;
    }
    debugPlanWork<<"Copied file:"<<filename;
    return true;
}

QDomDocument WorkPackage::saveXML()
{
    debugPlanWork;
    QDomDocument document(QStringLiteral("plan-workpackage"));

    document.appendChild(document.createProcessingInstruction(
                              QStringLiteral("xml"),
                              QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));

    QDomElement doc = document.createElement(QStringLiteral("planwork"));
    doc.setAttribute(QStringLiteral("editor"), QStringLiteral("PlanWork"));
    doc.setAttribute(QStringLiteral("mime"), PLANWORK_MIME_TYPE);
    doc.setAttribute(QStringLiteral("version"), PLAN_FILE_SYNTAX_VERSION);
    doc.setAttribute(QStringLiteral("plan-version"), PLAN_FILE_SYNTAX_VERSION);
    document.appendChild(doc);

    // Work package info
    QDomElement wp = document.createElement(QStringLiteral("workpackage"));
    wp.setAttribute(QStringLiteral("time-tag"), QDateTime::currentDateTime().toString(Qt::ISODate));
    m_settings.saveXML(wp);
    KPlato::Task *t = qobject_cast<KPlato::Task*>(node());
    if (t) {
        wp.setAttribute(QStringLiteral("owner"), t->workPackage().ownerName());
        wp.setAttribute(QStringLiteral("owner-id"), t->workPackage().ownerId());
    }
    doc.appendChild(wp);
    m_project->save(doc, KPlato::XmlSaveContext());
    return document;
}

void WorkPackage::merge(Part *part, const WorkPackage *wp, KoStore *store)
{
    debugPlanWork;
    const KPlato::Node *from = wp->node();
    KPlato::Node *to = node();

    KPlato::MacroCommand *m = new KPlato::MacroCommand(kundo2_i18n("Merge data"));
    if (m_wbsCode != wp->wbsCode()) {
        debugPlanWork<<"wbs code:"<<wp->wbsCode();
        m->addCommand(new ModifyWbsCodeCmd(this, wp->wbsCode()));
    }
    if (to->name() != from->name()) {
        debugPlanWork<<"name:"<<from->name();
        m->addCommand(new KPlato::NodeModifyNameCmd(*to, from->name()));
    }
    if (to->description() != from->description()) {
        debugPlanWork<<"description:"<<from->description();
        m->addCommand(new KPlato::NodeModifyDescriptionCmd(*to, from->description()));
    }
    if (to->startTime() != from->startTime() && from->startTime().isValid()) {
        debugPlanWork<<"start time:"<<from->startTime();
        m->addCommand(new KPlato::NodeModifyStartTimeCmd(*to, from->startTime()));
    }
    if (to->endTime() != from->endTime() && from->endTime().isValid()) {
        debugPlanWork<<"end time:"<<from->endTime();
        m->addCommand(new KPlato::NodeModifyEndTimeCmd(*to, from->endTime()));
    }
    if (to->leader() != from->leader()) {
        debugPlanWork<<"leader:"<<from->leader();
        m->addCommand(new KPlato::NodeModifyLeaderCmd(*to, from->leader()));
    }

    if (from->type() == KPlato::Node::Type_Task && from->type() == KPlato::Node::Type_Task) {
        if (static_cast<KPlato::Task*>(to)->workPackage().ownerId() != static_cast<const KPlato::Task*>(from)->workPackage().ownerId()) {
            debugPlanWork<<"merge:"<<"different owners"<<static_cast<const KPlato::Task*>(from)->workPackage().ownerName()<<static_cast<KPlato::Task*>(to)->workPackage().ownerName();
            if (static_cast<KPlato::Task*>(to)->workPackage().ownerId().isEmpty()) {
                //TODO cmd
                static_cast<KPlato::Task*>(to)->workPackage().setOwnerId(static_cast<const KPlato::Task*>(from)->workPackage().ownerId());
                static_cast<KPlato::Task*>(to)->workPackage().setOwnerName(static_cast<const KPlato::Task*>(from)->workPackage().ownerName());
            }
        }
        const QList<KPlato::Document*> documents = from->documents().documents();
        for (KPlato::Document *doc : documents) {
            KPlato::Document *org = to->documents().findDocument(doc->url());
            if (org) {
                // TODO: also handle modified type, sendas
                // update ? what if open, modified ...
                if (doc->type() == KPlato::Document::Type_Product) {
                    //### FIXME. user feedback
                    warnPlanWork<<"We do not update existing deliverables (except name change)";
                    if (doc->name() != org->name()) {
                        m->addCommand(new KPlato::DocumentModifyNameCmd(org, doc->name()));
                    }
                } else {
                    if (doc->name() != org->name()) {
                        m->addCommand(new KPlato::DocumentModifyNameCmd(org, doc->name()));
                    }
                    if (doc->sendAs() != org->sendAs()) {
                        m->addCommand(new KPlato::DocumentModifySendAsCmd(org, doc->sendAs()));
                    }
                    if (doc->sendAs() == KPlato::Document::SendAs_Copy) {
                        debugPlanWork<<"Update existing doc:"<<org->url();
                        openNewDocument(org, store);
                    }
                }
            } else {
                debugPlanWork<<"new document:"<<doc->typeToString(doc->type())<<doc->url();
                KPlato::Document *newdoc = new KPlato::Document(*doc);
                m->addCommand(new KPlato::DocumentAddCmd(to->documents(), newdoc));
                if (doc->sendAs() == KPlato::Document::SendAs_Copy) {
                    debugPlanWork<<"Copy file";
                    openNewDocument(newdoc, store);
                }
            }
        }
    }
    const KPlato::Project *fromProject = wp->project();
    KPlato::Project *toProject = m_project;
    const KPlato::ScheduleManager *fromSm = fromProject->scheduleManagers().value(0);
    Q_ASSERT(fromSm);
    KPlato::ScheduleManager *toSm = toProject->scheduleManagers().value(0);

    if (!toSm || fromSm->managerId() != toSm->managerId() || fromSm->scheduleId() != toSm->scheduleId()) {
        // rescheduled, update schedules
        debugPlanWork<<"schedules:"<<fromSm;
        m->addCommand(new CopySchedulesCmd(*fromProject, *toProject));
    }
    if (m->isEmpty()) {
        debugPlanWork<<"Nothing to merge";
        delete m;
    } else {
        part->addCommand(m);
    }
}

void WorkPackage::openNewDocument(const KPlato::Document *doc, KoStore *store)
{
    const QUrl url = extractFile(doc, store);
    if (url.url().isEmpty()) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Could not extract document from storage:<br>%1", doc->url().path()));
        }
        return;
    }
    if (! url.isValid()) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("Invalid URL:<br>%1", url.path()));
        }
        return;
    }
    m_newdocs.insert(doc, url);
}

int WorkPackage::queryClose(Part *part)
{
    debugPlanWork<<isModified();
    QString name = node()->name();
    QStringList lst;
    if (! m_childdocs.isEmpty()) {
        for (DocumentChild *ch : std::as_const(m_childdocs)) {
            if (ch->isOpen() && ch->doc()->sendAs() == KPlato::Document::SendAs_Copy) {
                lst << ch->doc()->url().fileName();
            }
        }
    }
    if (! lst.isEmpty()) {
        KMessageBox::ButtonCode result = KMessageBox::Continue;
        if (!m_nogui) {
            result = KMessageBox::warningContinueCancelList(nullptr,
                    i18np(
                        "<p>The work package <b>'%2'</b> has an open document.</p><p>Data may be lost if you continue.</p>",
                        "<p>The work package <b>'%2'</b> has open documents.</p><p>Data may be lost if you continue.</p>",
                        lst.count(),
                        name),
                    lst);
        }
        switch (result) {
            case KMessageBox::Continue: {
                debugPlanWork<<"Continue";
                break;
            }
            default: // case KMessageBox::Cancel :
                debugPlanWork<<"Cancel";
                return KMessageBox::Cancel;
                break;
        }
    }
    if (! isModified()) {
        return KMessageBox::PrimaryAction;
    }
    KMessageBox::ButtonCode res = KMessageBox::PrimaryAction;
    if (!m_nogui) {
        res = KMessageBox::warningTwoActionsCancel(nullptr,
                i18n("<p>The work package <b>'%1'</b> has been modified.</p><p>Do you want to save it?</p>", name),
                QString(),
                KStandardGuiItem::save(),
                KStandardGuiItem::discard());
    }
    switch (res) {
        case KMessageBox::PrimaryAction: {
            debugPlanWork<<"Yes";
            saveToProjects(part);
            break;
        }
        case KMessageBox::SecondaryAction:
            debugPlanWork<<"No";
            break;
        default: // case KMessageBox::Cancel :
            debugPlanWork<<"Cancel";
            break;
    }
    return res;
}

QUrl WorkPackage::extractFile(const KPlato::Document *doc)
{
    KoStore *store = KoStore::createStore(m_filePath, KoStore::Read, "", KoStore::Zip);
    if (store->bad())
    {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("<p>Work package <b>'%1'</b></p><p>Could not open store:</p><p>%2</p>", node()->name(), m_filePath));
        }
        delete store;
        return QUrl();
    }
    const QUrl url = extractFile(doc, store);
    delete store;
    return url;
}

QUrl WorkPackage::extractFile(const KPlato::Document *doc, KoStore *store)
{
    //FIXME: should use a special tmp dir
    QString tmp = QDir::tempPath() + QLatin1Char('/') + doc->url().fileName();
    const QUrl url = QUrl::fromLocalFile(tmp);
    debugPlanWork<<"Extract: "<<doc->url().fileName()<<" -> "<<url.path();
    if (! store->extractFile(doc->url().fileName(), url.path())) {
        if (!m_nogui) {
            KMessageBox::error(nullptr, i18n("<p>Work package <b>'%1'</b></p><p>Could not extract file:</p><p>%2</p>", node()->name(), doc->url().fileName()));
        }
        return QUrl();
    }
    return url;
}

QString WorkPackage::id() const
{
    QString id;
    if (node()) {
        id = m_project->id() + node()->id();
    }
    return id;
}

void WorkPackage::setNoGui(bool nogui)
{
    m_nogui = nogui;
}

//--------------------------------
PackageRemoveCmd::PackageRemoveCmd(Part *part, WorkPackage *value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_part(part),
    m_value(value),
    m_mine(false)
{
}
PackageRemoveCmd::~PackageRemoveCmd()
{
    if (m_mine) {
        m_value->removeFile();
        delete m_value;
    }
}
void PackageRemoveCmd::execute()
{
    m_part->removeWorkPackage(m_value);
    m_mine = true;
}
void PackageRemoveCmd::unexecute()
{
    m_part->addWorkPackage(m_value);
    m_mine = false;
}

//---------------------
CopySchedulesCmd::CopySchedulesCmd(const KPlato::Project &fromProject, KPlato::Project &toProject, const KUndo2MagicString &name)
    : KPlato::NamedCommand(name),
      m_project(toProject)
{
    QDomDocument olddoc;
    QDomElement e = olddoc.createElement(QStringLiteral("old"));
    olddoc.appendChild(e);
    toProject.save(e, KPlato::XmlSaveContext());
    m_olddoc = olddoc.toString();

    QDomDocument newdoc;
    e = newdoc.createElement(QStringLiteral("new"));
    newdoc.appendChild(e);
    fromProject.save(e, KPlato::XmlSaveContext());
    m_newdoc = newdoc.toString();
}
void CopySchedulesCmd::execute()
{
    load(m_newdoc);
}
void CopySchedulesCmd::unexecute()
{
    load(m_olddoc);
}

void CopySchedulesCmd::load(const QString &doc)
{
    clearSchedules();

    KoXmlDocument d;
    d.setContent(doc);
    KoXmlElement proj = d.documentElement().namedItem("project").toElement();
    Q_ASSERT(! proj.isNull());
    KoXmlElement tasks = proj.namedItem("tasks").toElement();
    if (tasks.isNull()) {
        return;
    }
    KoXmlElement task = tasks.namedItem("task").toElement();
    if (task.isNull()) {
        return;
    }
    KoXmlElement ts = task.namedItem("task-schedules").namedItem("task-schedule").toElement();
    if (ts.isNull()) {
        return;
    }
    KoXmlElement ps = proj.namedItem("project-schedules").namedItem("schedule-management").toElement();

    KPlato::XMLLoaderObject status;
    status.setProject(&m_project);
    status.setVersion(PLAN_FILE_SYNTAX_VERSION);
    status.setMimetype(PLAN_MIME_TYPE);
    const auto loader = status.loader();
    // task first
    KPlato::NodeSchedule *ns = new KPlato::NodeSchedule();
    if (loader->loadNodeSchedule(ns, ts, status)) {
        debugPlanWork<<ns->name()<<ns->type()<<ns->id();
        ns->setNode(m_project.childNode(0));
        m_project.childNode(0)->addSchedule(ns);
    } else {
        Q_ASSERT(false);
        delete ns;
    }
    if (!ps.isNull()) {
        // schedule manager next (includes main schedule and resource schedule)
        KPlato::ScheduleManager *sm = new KPlato::ScheduleManager(m_project);
        if (loader->load(sm, ps, status)) {
            m_project.addScheduleManager(sm);
        } else {
            Q_ASSERT(false);
            delete sm;
        }
        if (sm) {
            m_project.setCurrentSchedule(sm->scheduleId());
        }
    }
    m_project.childNode(0)->changed();
}

void CopySchedulesCmd::clearSchedules()
{
    const QList<KPlato::Schedule*> schedules = m_project.childNode(0)->schedules().values();
    for (KPlato::Schedule *s : schedules) {
        m_project.takeSchedule(s);
    }
    for (KPlato::Schedule *s : schedules) {
        const QList<KPlato::Appointment*> appointments = s->appointments();
        for (KPlato::Appointment *a : appointments) {
            if (a->resource() && a->resource()->resource()) {
                a->resource()->resource()->takeSchedule(a->resource());
            }
        }
        m_project.childNode(0)->takeSchedule(s);
    }
    const QList<KPlato::ScheduleManager*> managers = m_project.scheduleManagers();
    for (KPlato::ScheduleManager *sm : managers) {
        m_project.takeScheduleManager(sm);
        delete sm;
    }
}

//---------------------
ModifyWbsCodeCmd::ModifyWbsCodeCmd(WorkPackage *wp, QString wbsCode,  const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_wp(wp)
    , m_old(wp->wbsCode())
    , m_new(wbsCode)
{
}

void ModifyWbsCodeCmd::execute()
{
    m_wp->setWbsCode(m_new);
}

void ModifyWbsCodeCmd::unexecute()
{
    m_wp->setWbsCode(m_old);
}

}  //KPlatoWork namespace

QDebug operator<<(QDebug dbg, const KPlatoWork::WorkPackage *wp)
{
    if (!wp) {
        return dbg.noquote() << "WorkPackage[0x0]";
    }
    return dbg << *wp;
}

QDebug operator<<(QDebug dbg, const KPlatoWork::WorkPackage &wp)
{
    dbg.noquote().nospace() << "WorkPackage[";
    dbg << "Project: " << wp.project()->name();
    dbg << " Task: " << wp.name();
    dbg << " Docs: " << wp.childDocs().count();
    dbg << ']';
    return dbg;
}
