/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "ProjectLoader_v0.h"

#include "kptlocale.h"
#include "kptxmlloaderobject.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcalendar.h"
#include "kptschedule.h"
#include "kptrelation.h"
#include "kptresource.h"
#include "kptaccount.h"
#include "kptappointment.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QTimeZone>


using namespace KPlato;

struct BlockCalendarVersion {
    Calendar *cal;
    BlockCalendarVersion(Calendar *c) : cal(c) { c->setBlockVersion(true); }
    ~BlockCalendarVersion() { cal->setBlockVersion(false); }
};

ProjectLoader_v0::ProjectLoader_v0()
    : ProjectLoaderBase()
{
}

bool KPlato::ProjectLoader_v0::load(KPlato::XMLLoaderObject& context, const KoXmlDocument &document)
{
//     debugPlanXml<<KoXml::asQDomDocument(document).toString();
    auto projectElement = document.documentElement().namedItem("project").toElement();
    if (projectElement.isNull()) {
        errorPlanXml<<"Could not find a project element";
        return false;
    }
#if 1
    return load(&context.project(), projectElement, context);
#else
    return context.project().load(projectElement, context);
#endif

}

bool ProjectLoader_v0::load(Project *project, const KoXmlElement &projectElement, XMLLoaderObject &status)
{
    //debugPlanXml<<"--->";
    QString s;
    bool ok = false;
    if (projectElement.hasAttribute("name")) {
        project->setName(projectElement.attribute("name"));
    }
    if (projectElement.hasAttribute("id")) {
        project->removeId(project->id());
        project->setId(projectElement.attribute("id"));
        project->registerNodeId(project);
    }
    if (projectElement.hasAttribute("priority")) {
        project->setPriority(projectElement.attribute(QStringLiteral("priority"), "0").toInt());
    }
    if (projectElement.hasAttribute("leader")) {
        project->setLeader(projectElement.attribute("leader"));
    }
    if (projectElement.hasAttribute("description")) {
        project->setDescription(projectElement.attribute("description"));
    }
    if (projectElement.hasAttribute("timezone")) {
        QTimeZone tz(projectElement.attribute("timezone").toLatin1());
        if (tz.isValid()) {
            project->setTimeZone(tz);
        } else warnPlanXml<<"No timezone specified, using default (local)";
        status.setProjectTimeZone(project->timeZone());
    }
    if (projectElement.hasAttribute("scheduling")) {
        // Allow for both numeric and text
        s = projectElement.attribute("scheduling", "0");
        int constraint = s.toInt(&ok);
        if (!ok) {
            project->setConstraint(s);
        }
        constraint = project->constraint();
        if (constraint != Node::MustStartOn && constraint != Node::MustFinishOn) {
            errorPlanXml << "Illegal constraint: " << project->constraintToString();
            project->setConstraint(Node::MustStartOn);
        }
    }
    if (projectElement.hasAttribute("start-time")) {
        s = projectElement.attribute("start-time");
        if (!s.isEmpty()) {
            project->setConstraintStartTime(DateTime::fromString(s, project->timeZone()));
        }
    }
    if (projectElement.hasAttribute("end-time")) {
        s = projectElement.attribute("end-time");
        if (!s.isEmpty()) {
            project->setConstraintEndTime(DateTime::fromString(s, project->timeZone()));
        }
    }
    status.setProgress(10);

    // Load the project children
    KoXmlElement e = projectElement;
    if (status.version() < "0.7.0") {
        e = projectElement;
    } else {
        e = projectElement.namedItem("project-settings").toElement();
    }
    if (!e.isNull()) {
        loadSettings(e, status);
    }
    e = projectElement.namedItem("documents").toElement();
    if (!e.isNull()) {
        load(project->documents(), e, status);
    }
    // Do calendars first, they only reference other calendars
    //debugPlanXml<<"Calendars--->";
    if (status.version() < "0.7.0") {
        e = projectElement;
    } else {
        e = projectElement.namedItem("calendars").toElement();
    }
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<e.tagName();
        QList<Calendar*> cals;
        KoXmlElement ce;
        forEachElement(ce, e) {
            if (ce.tagName() != "calendar") {
                continue;
            }
            // Load the calendar.
            // Referenced by resources
            Calendar *child = new Calendar();
            child->setProject(project);
            if (load(child, ce, status)) {
                cals.append(child); // temporary, reorder later
            } else {
                // TODO: Complain about this
                errorPlanXml << "Failed to load calendar";
                delete child;
            }
        }
        // calendars references calendars in arbitrary saved order
        bool added = false;
        do {
            added = false;
            QList<Calendar*> lst;
            while (!cals.isEmpty()) {
                Calendar *c = cals.takeFirst();
                BlockCalendarVersion b(c);
                if (c->parentId().isEmpty()) {
                    if (status.version() < QStringLiteral("0.6")) {
                        // base calendar was stored in standard-worktime and is set as status.baseCalendar()
                        // In later versions status.baseCalendar() == nullptr
                        warnPlanXml<<"Pre 0.6 version: calendar added to project:"<<c->name()<<"parent:"<<status.baseCalendar();
                    }
                    project->addCalendar(c, status.baseCalendar());
                    added = true;
                } else {
                    Calendar *par = project->calendar(c->parentId());
                    if (par) {
                        BlockCalendarVersion b(par);
                        project->addCalendar(c, par);
                        added = true;
                        //debugPlanXml<<"added:"<<c->name()<<" to parent:"<<par->name();
                    } else {
                        lst.append(c); // treat later
                        //debugPlanXml<<"treat later:"<<c->name();
                    }
                }
            }
            cals = lst;
        } while (added);
        if (!cals.isEmpty()) {
            errorPlanXml<<"All calendars not saved!";
        }
        //debugPlanXml<<"Calendars<---";
    }
    status.setProgress(15);

    KoXmlNode n;
    // Resource groups and resources, can reference calendars
    if (status.version() < "0.7.0") {
        forEachElement(e, projectElement) {
            if (e.tagName() == "resource-group") {
                debugPlanXml<<status.version()<<e.tagName();
                // Load the resource group
                ResourceGroup *child = new ResourceGroup();
                if (load(child, e, status)) {
                    project->addResourceGroup(child);
                } else {
                    // TODO: Complain about this
                    errorPlanXml<<"Failed to load resource group";
                    delete child;
                }
            }
        }
        const auto groups = project->resourceGroups();
        for (ResourceGroup *g : groups) {
            const auto resources = g->resources();
            for (Resource *r : resources) {
                project->addResource(r);
            }
        }
    } else {
        e = projectElement.namedItem("resource-groups").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement ge;
            forEachElement(ge, e) {
                if (ge.nodeName() != "resource-group") {
                    continue;
                }
                ResourceGroup *child = new ResourceGroup();
                if (load(child, ge, status)) {
                    project->addResourceGroup(child);
                } else {
                    // TODO: Complain about this
                    errorPlanXml<<"Failed to load resource group";
                    delete child;
                }
            }
        }
        e = projectElement.namedItem("resources").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.nodeName() != "resource") {
                    continue;
                }
                Resource *r = new Resource();
                if (load(r, re, status)) {
                    project->addResource(r);
                } else {
                    errorPlanXml<<"Failed to load resource xml";
                    delete r;
                }
            }
        }
        // resource-group relations
        e = projectElement.namedItem("resource-group-relations").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.nodeName() != "resource-group-relation") {
                    continue;
                }
                ResourceGroup *g = project->group(re.attribute("group-id"));
                Resource *r = project->resource(re.attribute("resource-id"));
                if (r && g) {
                    r->addParentGroup(g);
                } else {
                    errorPlanXml<<"Failed to load resource-group-relation";
                }
            }
        }
        e = projectElement.namedItem("resource-teams").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement el;
            forEachElement(el, e) {
                if (el.tagName() == "team") {
                    Resource *r = project->findResource(el.attribute("team-id"));
                    Resource *tm = project->findResource(el.attribute("member-id"));
                    if (r == nullptr || tm == nullptr) {
                        errorPlanXml<<"resource-teams: cannot find resources";
                        continue;
                    }
                    if (r == tm) {
                        errorPlanXml<<"resource-teams: a team cannot be a member of itself";
                        continue;
                    }
                    r->addTeamMemberId(tm->id());
                } else {
                    errorPlanXml<<"resource-teams: unhandled tag"<<el.tagName();
                }
            }
        }
        e = projectElement.namedItem("required-resources").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.nodeName() != "required-resource") {
                    continue;
                }
                Resource *required = project->resource(re.attribute("required-id"));
                Resource *resource = project->resource(re.attribute("resource-id"));
                if (required && resource) {
                    resource->addRequiredId(required->id());
                } else {
                    errorPlanXml<<"Failed to load required-resource";
                }
            }
        }
        e = projectElement.namedItem("external-appointments").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement ext;
            forEachElement(ext, e) {
                if (e.nodeName() != "external-appointment") {
                    continue;
                }
                Resource *resource = project->resource(e.attribute("resource-id"));
                if (!resource) {
                    errorPlanXml<<"Cannot find resource:"<<e.attribute("resource-id");
                    continue;
                }
                QString projectId = e.attribute("project-id");
                if (projectId.isEmpty()) {
                    errorPlanXml<<"Missing project id";
                    continue;
                }
                resource->clearExternalAppointments(projectId); // in case...
                AppointmentIntervalList lst;
                lst.loadXML(e, status);
                Appointment *a = new Appointment();
                a->setIntervals(lst);
                a->setAuxcilliaryInfo(e.attribute("project-name", "Unknown"));
                resource->addExternalAppointment(projectId, a);
            }
        }
    }

    status.setProgress(20);

    // The main stuff
    if (status.version() < "0.7.0") {
        e = projectElement;
    } else {
        e = projectElement.namedItem("tasks").toElement();
    }
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<"tasks:"<<e.tagName();
        KoXmlElement te;
        forEachElement(te, e) {
            if (te.tagName() != "task") {
                continue;
            }
            //debugPlanXml<<"Task--->";
            // Depends on resources already loaded
            Task *child = new Task(project);
            if (load(child, te, status)) {
                if (!project->addTask(child, project)) {
                    errorPlanXml<<"Failed to load task";
                    delete child; // TODO: Complain about this
                } else {
                    debugPlanXml<<status.version()<<"Added task:"<<child;
                }
            } else {
                // TODO: Complain about this
                errorPlanXml<<"Failed to load task";
                delete child;
            }
        }
        if (project->numChildren() == 0) {
            debugPlanXml<<"No tasks added from"<<e.tagName();
        }
    }

    status.setProgress(70);

    // These go last
    e = projectElement.namedItem("accounts").toElement();
    if (!load(project->accounts(), e, status)) {
        errorPlanXml << "Failed to load accounts";
    }

    n = projectElement.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        //debugPlanXml<<n.isElement();
        if (! n.isElement()) {
            continue;
        }
        if (status.version() < "0.7.0" && e.tagName() == "relation") {
            debugPlanXml<<status.version()<<e.tagName();
            // Load the relation
            // References tasks
            Relation *child = new Relation();
            if (!load(child, e, status)) {
                // TODO: Complain about this
                errorPlanXml << "Failed to load relation";
                delete child;
            }
            //debugPlanXml<<"Relation<---";
        } else if (status.version() < "0.7.0" && e.tagName() == "resource-requests") {
            // NOTE: Not supported: request to project (or sub-project)
            Q_ASSERT(false);
        } else if (status.version() < "0.7.0" && e.tagName() == "wbs-definition") {
            load(project->wbsDefinition(), e, status);
        } else if (e.tagName() == "locale") {
            // handled earlier
        } else if (e.tagName() == "resource-group") {
            // handled earlier
        } else if (e.tagName() == "calendar") {
            // handled earlier
        } else if (e.tagName() == "standard-worktime") {
            // handled earlier
        } else if (e.tagName() == "project") {
            // handled earlier
        } else if (e.tagName() == "task") {
            // handled earlier
        } else if (e.tagName() == "shared-resources") {
            // handled earlier
        } else if (e.tagName() == "documents") {
            // handled earlier
        } else if (e.tagName() == "workpackageinfo") {
            // handled earlier
        } else if (e.tagName() == "task-modules") {
            // handled earlier
        } else if (e.tagName() == "accounts") {
            // handled earlier
        } else {
            warnPlanXml<<"Unhandled tag:"<<e.tagName();
        }
    }
    if (status.version() < "0.7.0") {
        e = projectElement.namedItem("schedules").toElement();
    } else {
        e = projectElement.namedItem("project-schedules").toElement();
    }
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<e.tagName();
        // References tasks and resources
        KoXmlElement sn;
        forEachElement(sn, e) {
            //debugPlanXml<<sn.tagName()<<" Version="<<status.version();
            ScheduleManager *sm = nullptr;
            bool add = false;
            if (status.version() <= "0.5") {
                if (sn.tagName() == "schedule") {
                    sm = project->findScheduleManagerByName(sn.attribute("name"));
                    if (sm == nullptr) {
                        sm = new ScheduleManager(*project, sn.attribute("name"));
                        add = true;
                    }
                }
            } else if (sn.tagName() == "schedule-management" || (status.version() < "0.7.0" && sn.tagName() == "plan")) {
                sm = new ScheduleManager(*project);
                add = true;
            } else {
                continue;
            }
            if (sm) {
                debugPlanXml<<"load schedule manager";
                if (load(sm, sn, status)) {
                    if (add)
                        project->addScheduleManager(sm);
                } else {
                    errorPlanXml << "Failed to load schedule manager";
                    delete sm;
                }
            } else {
                debugPlanXml<<"No schedule manager ?!";
            }
        }
        //debugPlanXml<<"Node schedules<---";
    }
    e = projectElement.namedItem("resource-teams").toElement();
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<e.tagName();
        // References other resources
        KoXmlElement re;
        forEachElement(re, e) {
            if (re.tagName() != "team") {
                continue;
            }
            Resource *r = project->findResource(re.attribute("team-id"));
            Resource *tm = project->findResource(re.attribute("member-id"));
            if (r == nullptr || tm == nullptr) {
                errorPlanXml<<"resource-teams: cannot find resources";
                continue;
            }
            if (r == tm) {
                errorPlanXml<<"resource-teams: a team cannot be a member of itself";
                continue;
            }
            r->addTeamMemberId(tm->id());
        }
    }
    e = projectElement;
    if (status.version() >= "0.7.0") {
        e = projectElement.namedItem("relations").toElement();
    }
    debugPlanXml<<status.version()<<e.tagName();
    if (!e.isNull()) {
        KoXmlElement de;
        forEachElement(de, e) {
            if (de.tagName() != "relation") {
                continue;
            }
            Relation *child = new Relation();
            if (!load(child, de, status)) {
                // TODO: Complain about this
                errorPlanXml << "Failed to load relation";
                delete child;
            }
        }
    }
    if (status.version() >= "0.7.0") {
        e = projectElement.namedItem("resource-requests").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.tagName() != "resource-request") {
                    continue;
                }
                Node *task = project->findNode(re.attribute("task-id"));
                if (!task) {
                    warnPlanXml<<re.tagName()<<"Failed to find task";
                    continue;
                }
                Resource *resource = project->findResource(re.attribute("resource-id"));
                Q_ASSERT(resource);
                Q_ASSERT(task);
                if (resource && task) {
                    int units = re.attribute("units", "100").toInt();
                    ResourceRequest *request = new ResourceRequest(resource, units);
                    int requestId = re.attribute("request-id").toInt();
                    Q_ASSERT(requestId > 0);
                    request->setId(requestId);
                    task->requests().addResourceRequest(request);
                } else {
                    warnPlanXml<<re.tagName()<<"Failed to find resource";
                }
            }
        }
        e = projectElement.namedItem("required-resource-requests").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.tagName() != "required-resource-request") {
                    continue;
                }
                Node *task = project->findNode(re.attribute("task-id"));
                Q_ASSERT(task);
                if (!task) {
                    continue;
                }
                ResourceRequest *request = task->requests().resourceRequest(re.attribute("request-id").toInt());
                Resource *required = project->findResource(re.attribute("required-id"));
                Q_ASSERT(required);
                if (required && request->resource() != required) {
                    if (request->requiredResources().contains(required)) {
                        errorPlanXml<<"Required resource request exists"<<required;
                        continue;
                    }
                    request->addRequiredResource(required);
                } else {
                    errorPlanXml<<"Loading required resource requests failed";
                }
            }
        }
        e = projectElement.namedItem("alternative-requests").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.tagName() != "alternative-request") {
                    continue;
                }
                Node *task = project->findNode(re.attribute("task-id"));
                if (!task) {
                    warnPlanXml<<re.tagName()<<"Failed to find task";
                    continue;
                }
                const ResourceRequestCollection &collection = task->requests();
                ResourceRequest *rr = collection.resourceRequest(re.attribute("request-id").toInt());
                Q_ASSERT(rr);
                if (!rr) {
                    errorPlanXml<<"Failed to find request to add lternatives to";
                    continue;
                }
                Resource *resource = project->findResource(re.attribute("resource-id"));
                //Q_ASSERT(resource);
                if (!resource) {
                    errorPlanXml<<"Alternative request: Failed to find resource:"<<re.attribute("resource-id");
                    continue;
                }
                ResourceRequest *alternative = new ResourceRequest(resource, re.attribute("units", "100").toInt());
                rr->addAlternativeRequest(alternative);
            }
        }
    }
    //debugPlanXml<<"<---";

    status.setProgress(90);

    return true;
}

bool KPlato::ProjectLoader_v0::loadSettings(const KoXmlElement& element, KPlato::XMLLoaderObject& status)
{
    Q_UNUSED(status)

    Project &project = status.project();
    project.setUseSharedResources(false); // default should be off in case old project NOTE: review

    KoXmlElement e;
    forEachElement(e, element) {
        debugPlanXml<<status.version()<<e.tagName();
        if (e.tagName() == "locale") {
            Locale *l = project.locale();
            l->setCurrencySymbol(e.attribute("currency-symbol", ""));
            if (e.hasAttribute("currency-digits")) {
                l->setMonetaryDecimalPlaces(e.attribute("currency-digits").toInt());
            }
            QLocale::Language language = QLocale::AnyLanguage;
            QLocale::Country country = QLocale::AnyCountry;
            if (e.hasAttribute("language")) {
                language = static_cast<QLocale::Language>(e.attribute("language").toInt());
            }
            if (e.hasAttribute("country")) {
                country = static_cast<QLocale::Country>(e.attribute("country").toInt());
            }
            l->setCurrencyLocale(language, country);
        } else if (e.tagName() == "shared-resources") {
            project.setUseSharedResources(e.attribute("use", "0").toInt());
            project.setSharedResourcesFile(e.attribute("file"));
        } else if (e.tagName() == QLatin1String("workpackageinfo")) {
            auto workPackageInfo = project.workPackageInfo();
            if (e.hasAttribute("check-for-workpackages")) {
                workPackageInfo.checkForWorkPackages = e.attribute("check-for-workpackages").toInt();
            }
            if (e.hasAttribute("retrieve-url")) {
                workPackageInfo.retrieveUrl = QUrl(e.attribute("retrieve-url"));
            }
            if (e.hasAttribute("delete-after-retrieval")) {
                workPackageInfo.deleteAfterRetrieval = e.attribute("delete-after-retrieval").toInt();
            }
            if (e.hasAttribute("archive-after-retrieval")) {
                workPackageInfo.archiveAfterRetrieval = e.attribute("archive-after-retrieval").toInt();
            }
            if (e.hasAttribute("archive-url")) {
                workPackageInfo.archiveUrl = QUrl(e.attribute("archive-url"));
            }
            if (e.hasAttribute("publish-url")) {
                workPackageInfo.publishUrl = QUrl(e.attribute("publish-url"));
            }
            project.setWorkPackageInfo(workPackageInfo);
        } else if (e.tagName() == QLatin1String("task-modules")) {
            project.setUseLocalTaskModules(false);
            QList<QUrl> urls;
            for (KoXmlNode child = e.firstChild(); !child.isNull(); child = child.nextSibling()) {
                KoXmlElement path = child.toElement();
                if (path.isNull()) {
                    continue;
                }
                QString s = path.attribute("url");
                if (!s.isEmpty()) {
                    QUrl url = QUrl::fromUserInput(s);
                    if (!urls.contains(url)) {
                        urls << url;
                    }
                }
            }
            bool useLocal = e.attribute("use-local-task-modules").toInt();
            project.setTaskModules(urls, useLocal);
        } else if (e.tagName() == "standard-worktime") {
            // Load standard worktime
            StandardWorktime *child = new StandardWorktime();
            if (load(child, e, status)) {
                project.setStandardWorktime(child);
            } else {
                errorPlanXml << "Failed to load standard worktime";
                delete child;
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Task *task, const KoXmlElement &element, XMLLoaderObject &status)
{
    QString s;
    bool ok = false;
    task->setId(element.attribute(QStringLiteral("id")));
    task->setPriority(element.attribute(QStringLiteral("priority"), "0").toInt());

    task->setName(element.attribute(QStringLiteral("name")));
    task->setLeader(element.attribute(QStringLiteral("leader")));
    task->setDescription(element.attribute(QStringLiteral("description")));
    //debugPlanXml<<m_name<<": id="<<task->id();

    // Allow for both numeric and text
    QString constraint = element.attribute(QStringLiteral("scheduling"),QStringLiteral("0"));
    auto c = (Node::ConstraintType)constraint.toInt(&ok);
    if (!ok) {
        task->setConstraint(constraint);
    } else {
        task->setConstraint(c);
    }
    s = element.attribute(QStringLiteral("constraint-starttime"));
    if (!s.isEmpty()) {
        task->setConstraintStartTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    s = element.attribute(QStringLiteral("constraint-endtime"));
    if (!s.isEmpty()) {
        task->setConstraintEndTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    task->setStartupCost(element.attribute(QStringLiteral("startup-cost"), QStringLiteral("0.0")).toDouble());
    task->setShutdownCost(element.attribute(QStringLiteral("shutdown-cost"), QStringLiteral("0.0")).toDouble());

    // Load the task children
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QLatin1String("project")) {
            // Load the subproject
/*              Project *child = new Project(this, status);
            if (child->load(e)) {
                if (!project.addSubTask(child, this)) {
                    delete child;  // TODO: Complain about this
                }
            } else {
                // TODO: Complain about this
                delete child;
            }*/
        } else if (e.tagName() == QLatin1String("task")) {
            if (status.loadTaskChildren()) {
                // Load the task
                Task *child = new Task(task);
                if (load(child, e, status)) {
                    if (!status.project().addSubTask(child, task)) {
                        errorPlanXml<<"Failed to add task";
                        delete child;  // TODO: Complain about this
                    }
                } else {
                    // TODO: Complain about this
                    errorPlanXml<<"Failed to load task";
                    delete child;
                }
            }
        } else if (e.tagName() == QLatin1String("resource")) {
            // TODO: Load the resource (projects don't have resources yet)
        } else if (e.tagName() == QLatin1String("estimate") ||
                   (/*status.version() < "0.6" &&*/ e.tagName() == QLatin1String("effort"))) {
            //  Load the estimate
            load(task->estimate(), e, status);
        } else if (e.tagName() == QLatin1String("workpackage")) {
            load(task->workPackage(), e, status);
        } else if (e.tagName() == QLatin1String("progress")) {
            load(task->completion(), e, status);
        } else if (e.tagName() == QLatin1String("task-schedules") || (status.version() < "0.7.0" && e.tagName() == QLatin1String("schedules"))) {
            KoXmlNode n = e.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                if (el.tagName() == QLatin1String("schedule")) {
                    NodeSchedule *sch = new NodeSchedule();
                    if (loadNodeSchedule(sch, el, status)) {
                        sch->setNode(task);
                        task->addSchedule(sch);
                    } else {
                        errorPlanXml<<"Failed to load schedule";
                        delete sch;
                    }
                }
            }
        } else if (e.tagName() == QLatin1String("resourcegroup-request")) {
            Q_ASSERT(status.version() < "0.7.0");
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.tagName() == "resource-request") {
                    ResourceRequest *r = new ResourceRequest();
                    if (load(r, re, status)) {
                        task->requests().addResourceRequest(r);
                    } else {
                        errorPlanXml<<"Failed to load resource request";
                        delete r;
                    }
                }
            }
            ResourceGroup *group = status.project().group(e.attribute("group-id"));
            if (!group) {
                errorPlanXml<<"Could not find resourcegroup"<<e.attribute("group-id");
            } else {
                QList<ResourceRequest*> groupRequests;
                int numRequests = e.attribute("units").toInt();
                for (int i = 0; i < numRequests; ++i) {
                    const auto resources = group->resources();
                    for (Resource *r : resources) {
                        if (!task->requests().find(r)) {
                            groupRequests << new ResourceRequest(r, 100);
                            task->requests().addResourceRequest(groupRequests.last());
                        }
                    }
                }
                for (ResourceRequest *rr : qAsConst(groupRequests)) {
                    const auto resources = group->resources();
                    for (Resource *r : resources) {
                        if (!task->requests().find(r)) {
                            rr->addAlternativeRequest(new ResourceRequest(r));
                        }
                    }
                }
            }
        } else if (e.tagName() == QLatin1String("documents")) {
            load(task->documents(), e, status);
        } else if (e.tagName() == QLatin1String("workpackage-log")) {
            KoXmlNode n = e.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                if (el.tagName() == QLatin1String("workpackage")) {
                    WorkPackage *wp = new WorkPackage(task);
                    if (loadWpLog(wp, el, status)) {
                        task->addWorkPackage(wp);
                    } else {
                        errorPlanXml<<"Failed to load logged workpackage";
                        delete wp;
                    }
                }
            }
        }
    }
    //debugPlanXml<<m_name<<" loaded";
    return true;
}

bool ProjectLoader_v0::load(Calendar *calendar, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml;
    BlockCalendarVersion b(calendar);

    calendar->setId(element.attribute(QStringLiteral("id")));
    calendar->setParentId(element.attribute(QStringLiteral("parent")));
    calendar->setName(element.attribute(QStringLiteral("name"),QLatin1String("")));
    QTimeZone tz(element.attribute(QStringLiteral("timezone")).toLatin1());
    if (tz.isValid()) {
        calendar->setTimeZone(tz);
    } else {
        warnPlanXml<<"No timezone specified, use default (local)";
    }
    calendar->setDefault((bool)element.attribute(QStringLiteral("default"),QStringLiteral("0")).toInt());
    if (calendar->isDefault()) {
        status.project().setDefaultCalendar(calendar);
    }
    if (status.version() < "0.7.0") {
        calendar->setShared(element.attribute(QStringLiteral("shared"), QStringLiteral("0")).toInt());
    } else {
        calendar->setShared(element.attribute(QStringLiteral("origin"), QStringLiteral("local")) != QStringLiteral("local"));
    }

#ifdef HAVE_KHOLIDAYS
    calendar->setHolidayRegion(element.attribute(QStringLiteral("holiday-region")));
#endif

    //debugPlanXml<<KoXml::childNodesCount(element);
    {KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == QLatin1String("weekday")) {
            if (!load(calendar->weekdays(), e, status)) {
                errorPlanXml<<calendar->name()<<": Failed to load calendar weekday";
                return false;
            }
        } else if (e.tagName() == QLatin1String("day")) {
            CalendarDay *day = new CalendarDay();
            if (load(day, e, status)) {
                if (!day->date().isValid()) {
                    delete day;
                    errorPlanXml<<calendar->name()<<": Failed to load calendarDay - Invalid date";
                } else {
                    CalendarDay *d = calendar->findDay(day->date());
                    if (d) {
                        // already exists, keep the new
                        delete calendar->takeDay(d);
                        warnPlanXml<<calendar->name()<<" Load calendarDay - Date already exists";
                    }
                    calendar->addDay(day);
                }
            } else {
                delete day;
                errorPlanXml<<"Failed to load calendarDay";
                continue; // don't throw away the whole calendar
            }
        }
    }}
    // this must go last
    KoXmlElement e = element.namedItem(QStringLiteral("cache")).toElement();
    if (!e.isNull()) {
        calendar->loadCacheVersion(e, status);
    }
    //debugPlanXml<<"Loaded calendar:"<<calendar->name();
    return true;
}

bool ProjectLoader_v0::load(CalendarDay *day, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<element.tagName()<<element.attributeNames()<<element.attribute("state");
    bool ok=false;
    day->setState(QString(element.attribute("state", QString::number(CalendarDay::Undefined))).toInt(&ok));
    if (day->state() < CalendarDay::Undefined || day->state() > CalendarDay::Working) {
        errorPlanXml<<"Failed to load calendar day - invalid state:"<<day->state();
        return false;
    }
    QString s = element.attribute("date");
    // A weekday has no date
    if (!s.isEmpty()) {
        day->setDate(QDate::fromString(s, Qt::ISODate));
        if (! day->date().isValid()) {
            day->setDate(QDate::fromString(s));
        }
    }
    day->clearIntervals();
    KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == QLatin1String("time-interval") || (status.version() < "0.7.0" && e.tagName() == QLatin1String("interval"))) {
            //debugPlanXml<<"Interval start="<<e.attribute("start")<<" end="<<e.attribute("end");
            QString st = e.attribute("start");
            if (st.isEmpty()) {
                errorPlanXml<<"Empty interval";
                continue;
            }
            QTime start = QTime::fromString(st);
            int length = 0;
            if (status.version() <= "0.6.1") {
                QString en = e.attribute("end");
                if (en.isEmpty()) {
                    // Be lenient when loading old format
                    warnPlanXml<<"Invalid interval end";
                    continue;
                }
                QTime end = QTime::fromString(en);
                length = start.msecsTo(end);
            } else {
                length = e.attribute("length", "0").toInt();
            }
            if (length <= 0) {
                errorPlanXml<<"Invalid interval length";
                return false;
            }
            day->addInterval(new TimeInterval(start, length));
        } else {
            warnPlanXml<<"Unknown tag:"<<e.tagName();
        }
    }
    //debugPlanXml<<"Loaded day:"<<day;
    return true;
}

bool ProjectLoader_v0::load(CalendarWeekdays *weekdays, const KoXmlElement& element, XMLLoaderObject& status)
{
    bool ok;
    int dayNo = QString(element.attribute("day","-1")).toInt(&ok);
    //debugPlanXml<<"weekday:"<<dayNo;
    if (dayNo < 0 || dayNo > 6) {
        warnPlanXml<<"Illegal weekday: "<<dayNo;
        return true; // we continue anyway
    }
    CalendarDay *day = weekdays->weekday(dayNo + 1);
    if (day == nullptr) {
        errorPlanXml<<"No weekday: "<<dayNo;
        return false;
    }
    if (!load(day, element, status)) {
        errorPlanXml<<"Failed to load weekday: "<<dayNo;
        return false;
    }
    return true;

}

bool ProjectLoader_v0::load(StandardWorktime *swt, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"swt";
    swt->setYear(Duration::fromString(element.attribute("year"), Duration::Format_Hour));
    swt->setMonth(Duration::fromString(element.attribute("month"), Duration::Format_Hour));
    swt->setWeek(Duration::fromString(element.attribute("week"), Duration::Format_Hour));
    swt->setDay(Duration::fromString(element.attribute("day"), Duration::Format_Hour));

    KoXmlElement e;
    forEachElement (e, element) {
        if (e.tagName() == "calendar") {
            // pre 0.6 version stored base calendar in standard worktime
            if (status.version() >= "0.6") {
                warnPlanXml<<"Old format, calendar in standard worktime";
                warnPlanXml<<"Tries to load anyway";
            }
            Calendar *calendar = new Calendar;
            if (load(calendar, e, status)) {
                status.project().addCalendar(calendar);
                calendar->setDefault(true);
                status.project().setDefaultCalendar(calendar); // hmmm
                status.setBaseCalendar(calendar);
            } else {
                delete calendar;
                warnPlanXml<<"Failed to load calendar";
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Relation *relation, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"relation";
    relation->setParent(status.project().findNode(element.attribute("parent-id")));
    if (relation->parent() == nullptr) {
        warnPlanXml<<"Parent node == 0, cannot find id:"<<element.attribute("parent-id");
        return false;
    }
    relation->setChild(status.project().findNode(element.attribute("child-id")));
    if (relation->child() == nullptr) {
        warnPlanXml<<"Child node == 0, cannot find id:"<<element.attribute("child-id");
        return false;
    }
    if (relation->child() == relation->parent()) {
        warnPlanXml<<"Parent node == child node";
        return false;
    }
    if (!relation->parent()->legalToLink(relation->child())) {
        warnPlanXml<<"Realation is not legal:"<<relation->parent()->name()<<"->"<<relation->child()->name();
        return false;
    }
    relation->setType(element.attribute("type"));

    relation->setLag(Duration::fromString(element.attribute("lag")));

    if (!relation->parent()->addDependChildNode(relation)) {
        errorPlanXml<<"Failed to add relation: Child="<<relation->child()->name()<<" parent="<<relation->parent()->name();
        return false;
    }
    if (!relation->child()->addDependParentNode(relation)) {
        relation->parent()->takeDependChildNode(relation);
        errorPlanXml<<"Failed to add relation: Child="<<relation->child()->name()<<" parent="<<relation->parent()->name();
        return false;
    }
    //debugPlanXml<<"Added relation: Child="<<relation->child()->name()<<" parent="<<relation->parent()->name();
    return true;
}

bool ProjectLoader_v0::load(ResourceGroup *rg, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"resource-group";
    rg->setId(element.attribute("id"));
    rg->setName(element.attribute("name"));
    rg->setType(element.attribute("type"));

    if (status.version() < "0.7.0") {
        rg->setShared(element.attribute("shared", "0").toInt());
    } else {
        rg->setShared(element.attribute("origin", "local") != "local");
    }
    rg->setCoordinator(element.attribute("coordinator"));

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "resource-group") {
            ResourceGroup *child = new ResourceGroup();
            if (child->load(e, status)) {
                rg->addChildGroup(child);
            } else {
                errorPlanXml<<"Faild to load ResourceGroup";
                delete child;
            }
        } else if (status.version() < "0.7.0" && e.tagName() == "resource") {
            // Load the resource
            Resource *child = new Resource();
            if (load(child, e, status)) {
                child->addParentGroup(rg);
            } else {
                // TODO: Complain about this
                errorPlanXml<<"Faild to load resource";
                delete child;
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Resource *resource, const KoXmlElement &element, XMLLoaderObject &status)
{
    const Locale *locale = status.project().locale();
    QString s;
    resource->setId(element.attribute("id"));
    resource->setName(element.attribute("name"));
    resource->setInitials( element.attribute("initials"));
    resource->setEmail(element.attribute("email"));
    resource->setAutoAllocate((bool)(element.attribute("auto-allocate", "0").toInt()));
    resource->setType(element.attribute("type"));
    if (status.version() < "0.7.0") {
        resource->setShared(element.attribute("shared").toInt());
    } else {
        resource->setShared(element.attribute("origin", "local") != "local");
    }
    resource->setCalendar(status.project().findCalendar(element.attribute("calendar-id")));
    resource->setUnits(element.attribute("units", "100").toInt());
    s = element.attribute("available-from");
    if (!s.isEmpty())
        resource->setAvailableFrom(DateTime::fromString(s, status.projectTimeZone()));
    s = element.attribute("available-until");
    if (!s.isEmpty())
        resource->setAvailableUntil(DateTime::fromString(s, status.projectTimeZone()));

    // NOTE: money was earlier (2.x) saved with symbol so we need to handle that
    QString money = element.attribute("normal-rate");
    bool ok = false;
    resource->cost().normalRate = money.toDouble(&ok);
    if (!ok) {
        resource->cost().normalRate = locale->readMoney(money);
        debugPlanXml<<"normal-rate failed, tried readMoney()"<<money<<"->"<<resource->cost().normalRate;;
    }
    money = element.attribute("overtime-rate");
    resource->cost().overtimeRate = money.toDouble(&ok);
    if (!ok) {
        resource->cost().overtimeRate = locale->readMoney(money);
        debugPlanXml<<"overtime-rate failed, tried readMoney()"<<money<<"->"<<resource->cost().overtimeRate;;
    }
    resource->cost().account = status.project().accounts().findAccount(element.attribute("account"));

    if (status.version() < "0.7.0") {
        KoXmlElement e;
        KoXmlElement parent = element.namedItem("required-resources").toElement();
        forEachElement(e, parent) {
            if (e.nodeName() == "resource") {
                QString id = e.attribute("id");
                if (id.isEmpty()) {
                    warnPlanXml<<"Missing resource id";
                    continue;
                }
                resource->addRequiredId(id);
            }
        }
        parent = element.namedItem("external-appointments").toElement();
        forEachElement(e, parent) {
            if (e.nodeName() == "project") {
                QString id = e.attribute("id");
                if (id.isEmpty()) {
                    errorPlanXml<<"Missing project id";
                    continue;
                }
                resource->clearExternalAppointments(id); // in case...
                AppointmentIntervalList lst;
                lst.loadXML(e, status);
                Appointment *a = new Appointment();
                a->setIntervals(lst);
                a->setAuxcilliaryInfo(e.attribute("name", "Unknown"));
                resource->addExternalAppointment(id, a);
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Accounts &accounts, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"accounts";
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "account") {
            Account *child = new Account();
            if (load(child, e, status)) {
                accounts.insert(child);
            } else {
                // TODO: Complain about this
                warnPlanXml<<"Loading failed";
                delete child;
            }
        }
    }
    if (element.hasAttribute("default-account")) {
        accounts.setDefaultAccount(accounts.findAccount(element.attribute("default-account")));
        if (accounts.defaultAccount() == nullptr) {
            warnPlanXml<<"Could not find default account.";
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Account* account, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"account";
    account->setName(element.attribute("name"));
    account->setDescription(element.attribute("description"));
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "costplace") {
            Account::CostPlace *child = new Account::CostPlace(account);
            if (load(child, e, status)) {
                account->append(child);
            } else {
                delete child;
            }
        } else if (e.tagName() == "account") {
            Account *child = new Account();
            if (load(child, e, status)) {
                account->insert(child);
            } else {
                // TODO: Complain about this
                warnPlanXml<<"Loading failed";
                delete child;
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Account::CostPlace* cp, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"cost-place";
    cp->setObjectId(element.attribute("object-id"));
    if (cp->objectId().isEmpty()) {
        // check old format
        cp->setObjectId(element.attribute("node-id"));
        if (cp->objectId().isEmpty()) {
            errorPlanXml<<"No object id";
            return false;
        }
    }
    cp->setNode(status.project().findNode(cp->objectId()));
    if (cp->node() == nullptr) {
        cp->setResource(status.project().findResource(cp->objectId()));
        if (cp->resource() == nullptr) {
            errorPlanXml<<"Cannot find object with id: "<<cp->objectId();
            return false;
        }
    }
    bool on = (bool)(element.attribute("running-cost").toInt());
    if (on) cp->setRunning(on);
    on = (bool)(element.attribute("startup-cost").toInt());
    if (on) cp->setStartup(on);
    on = (bool)(element.attribute("shutdown-cost").toInt());
    if (on) cp->setShutdown(on);
    return true;
}

bool ProjectLoader_v0::load(ScheduleManager *manager, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"schedule-manager";
    MainSchedule *sch = nullptr;
    if (status.version() <= "0.5") {
        manager->setUsePert(false);
        MainSchedule *sch = loadMainSchedule(manager, element, status);
        if (sch && sch->type() == Schedule::Expected) {
            sch->setManager(manager);
            manager->setExpected(sch);
        } else {
            delete sch;
        }
        return true;
    }
    manager->setName(element.attribute("name"));
    manager->setManagerId(element.attribute("id"));
    manager->setUsePert(element.attribute("distribution").toInt() == 1);
    manager->setAllowOverbooking((bool)(element.attribute("overbooking").toInt()));
    manager->setCheckExternalAppointments((bool)(element.attribute("check-external-appointments").toInt()));
    manager->setSchedulingDirection((bool)(element.attribute("scheduling-direction").toInt()));
    manager->setBaselined((bool)(element.attribute("baselined").toInt()));
    manager->setSchedulerPluginId(element.attribute("scheduler-plugin-id"));
    manager->setRecalculate((bool)(element.attribute("recalculate").toInt()));
    manager->setRecalculateFrom(DateTime::fromString(element.attribute("recalculate-from"), status.projectTimeZone()));
    if (element.hasAttribute("scheduling-mode")) {
        manager->setSchedulingMode(element.attribute("scheduling-mode").toInt());
    }

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        //debugPlanXml<<e.tagName();
        if (e.tagName() == "schedule") {
            sch = loadMainSchedule(manager, e, status);
            if (sch) {
                sch->setManager(manager);
                switch (sch->type()) {
                    case Schedule::Expected: manager->setExpected(sch); break;
                }
            }
        } else if (e.tagName() == "schedule-management" || (status.version() < "0.7.0" && e.tagName() == "plan")) {
            ScheduleManager *sm = new ScheduleManager(status.project());
            if (load(sm, e, status)) {
                status.project().addScheduleManager(sm, manager);
            } else {
                errorPlanXml<<"Failed to load schedule manager"<<'\n';
                delete sm;
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Schedule *schedule, const KoXmlElement& element, XMLLoaderObject& /*status*/)
{
    debugPlanXml<<"schedule";
    schedule->setName(element.attribute("name"));
    schedule->setType(element.attribute("type"));
    schedule->setId(element.attribute("id").toLong());

    return true;
}

MainSchedule* ProjectLoader_v0::loadMainSchedule(ScheduleManager* /*manager*/, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"main-schedule";
    MainSchedule *sch = new MainSchedule();
    if (loadMainSchedule(sch, element, status)) {
        status.project().addSchedule(sch);
        sch->setNode(&(status.project()));
        status.project().setParentSchedule(sch);
        // If it's here, it's scheduled!
        sch->setScheduled(true);
    } else {
        errorPlanXml << "Failed to load schedule";
        delete sch;
        sch = nullptr;
    }
    return sch;
}

bool ProjectLoader_v0::loadMainSchedule(MainSchedule *ms, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml;
    QString s;
    load(ms, element, status);

    s = element.attribute("start");
    if (!s.isEmpty()) {
        ms->startTime = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("end");
    if (!s.isEmpty()) {
        ms->endTime = DateTime::fromString(s, status.projectTimeZone());
    }
    ms->duration = Duration::fromString(element.attribute("duration"));
    ms->constraintError = element.attribute("scheduling-conflict", "0").toInt();

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement el = n.toElement();
        if (el.tagName() == "appointment") {
            // Load the appointments.
            // Resources and tasks must already be loaded
            Appointment * child = new Appointment();
            if (! load(child, el, status, *ms)) {
                // TODO: Complain about this
                errorPlanXml << "Failed to load appointment" << '\n';
                delete child;
            }
        } else if (el.tagName() == "criticalpath-list") {
            // Tasks must already be loaded
            for (KoXmlNode n1 = el.firstChild(); ! n1.isNull(); n1 = n1.nextSibling()) {
                if (! n1.isElement()) {
                    continue;
                }
                KoXmlElement e1 = n1.toElement();
                if (e1.tagName() != "criticalpath") {
                    continue;
                }
                QList<Node*> lst;
                for (KoXmlNode n2 = e1.firstChild(); ! n2.isNull(); n2 = n2.nextSibling()) {
                    if (! n2.isElement()) {
                        continue;
                    }
                    KoXmlElement e2 = n2.toElement();
                    if (e2.tagName() != "node") {
                        continue;
                    }
                    QString s = e2.attribute("id");
                    Node *node = status.project().findNode(s);
                    if (node) {
                        lst.append(node);
                    } else {
                        errorPlanXml<<"Failed to find node id="<<s;
                    }
                }
                ms->m_pathlists.append(lst);
            }
            ms->criticalPathListCached = true;
        }
    }
    return true;
}

bool ProjectLoader_v0::loadNodeSchedule(NodeSchedule* schedule, const KoXmlElement &element, XMLLoaderObject& status)
{
    debugPlanXml<<"node-schedule";
    QString s;
    load(schedule, element, status);
    s = element.attribute("earlystart");
    if (s.isEmpty()) { // try version < 0.6
        s = element.attribute("earlieststart");
    }
    if (!s.isEmpty()) {
        schedule->earlyStart = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("latefinish");
    if (s.isEmpty()) { // try version < 0.6
        s = element.attribute("latestfinish");
    }
    if (!s.isEmpty()) {
        schedule->lateFinish = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("latestart");
    if (!s.isEmpty()) {
        schedule->lateStart = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("earlyfinish");
    if (!s.isEmpty()) {
        schedule->earlyFinish = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("start");
    if (!s.isEmpty()) {
        schedule->startTime = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("end");
    if (!s.isEmpty()) {
        schedule->endTime = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("start-work");
    if (!s.isEmpty()) {
        schedule->workStartTime = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute("end-work");
    if (!s.isEmpty()) {
        schedule->workEndTime = DateTime::fromString(s, status.projectTimeZone());
    }
    schedule->duration = Duration::fromString(element.attribute("duration"));

    schedule->inCriticalPath = element.attribute("in-critical-path", "0").toInt();
    schedule->resourceError = element.attribute("resource-error", "0").toInt();
    schedule->resourceOverbooked = element.attribute("resource-overbooked", "0").toInt();
    schedule->resourceNotAvailable = element.attribute("resource-not-available", "0").toInt();
    schedule->constraintError = element.attribute("scheduling-conflict", "0").toInt();
    schedule->notScheduled = element.attribute("not-scheduled", "1").toInt();

    schedule->positiveFloat = Duration::fromString(element.attribute("positive-float"));
    schedule->negativeFloat = Duration::fromString(element.attribute("negative-float"));
    schedule->freeFloat = Duration::fromString(element.attribute("free-float"));

    return true;
}

bool ProjectLoader_v0::load(WBSDefinition &def, const KoXmlElement &element, XMLLoaderObject &/*status*/)
{
    debugPlanXml<<"wbs-def";
    def.setProjectCode(element.attribute("project-code"));
    def.setProjectSeparator(element.attribute("project-separator"));
    def.setLevelsDefEnabled((bool)element.attribute("levels-enabled", "0").toInt());
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "default") {
            def.defaultDef().code = e.attribute("code", "Number");
            def.defaultDef().separator = e.attribute("separator", ".");
        } else if (e.tagName() == "levels") {
            KoXmlNode n = e.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                WBSDefinition::CodeDef d;
                d.code = el.attribute("code");
                d.separator = el.attribute("separator");
                int lvl = el.attribute("level", "-1").toInt();
                if (lvl >= 0) {
                    def.setLevelsDef(lvl, d);
                } else errorPlanXml<<"Invalid levels definition";
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Documents &documents, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"documents";
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "document") {
            Document *doc = new Document();
            if (!load(doc, e, status)) {
                warnPlanXml<<"Failed to load document";
                status.addMsg(XMLLoaderObject::Errors, "Failed to load document");
                delete doc;
            } else {
                documents.addDocument(doc);
                status.addMsg(i18n("Document loaded, URL=%1",  doc->url().url()));
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Document *document, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"document";
    Q_UNUSED(status);
    document->setUrl(QUrl(element.attribute("url")));
    document->setType((Document::Type)(element.attribute("type").toInt()));
    document->setStatus(element.attribute("status"));
    document->setSendAs((Document::SendAs)(element.attribute("sendas").toInt()));
    return true;
}

bool ProjectLoader_v0::load(Estimate* estimate, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"estimate";
    estimate->setType(element.attribute("type"));
    estimate->setRisktype(element.attribute("risk"));
    if (status.version() <= "0.6") {
        estimate->setUnit((Duration::Unit)(element.attribute("display-unit", QString().number(Duration::Unit_h)).toInt()));
        QList<qint64> s = status.project().standardWorktime()->scales();
        estimate->setExpectedEstimate(Estimate::scale(Duration::fromString(element.attribute("expected")), estimate->unit(), s));
        estimate->setOptimisticEstimate(Estimate::scale(Duration::fromString(element.attribute("optimistic")), estimate->unit(), s));
        estimate->setPessimisticEstimate(Estimate::scale(Duration::fromString(element.attribute("pessimistic")), estimate->unit(), s));
    } else {
        if (status.version() <= "0.6.2") {
            // 0 pos in unit is now Unit_Y, so add 3 to get the correct new unit
            estimate->setUnit((Duration::Unit)(element.attribute("unit", QString().number(Duration::Unit_ms - 3)).toInt() + 3));
        } else {
            estimate->setUnit(Duration::unitFromString(element.attribute("unit")));
        }
        estimate->setExpectedEstimate(element.attribute("expected", "0.0").toDouble());
        estimate->setOptimisticEstimate(element.attribute("optimistic", "0.0").toDouble());
        estimate->setPessimisticEstimate(element.attribute("pessimistic", "0.0").toDouble());

        estimate->setCalendar(status.project().findCalendar(element.attribute("calendar-id")));
    }
    return true;
}

#if 0
bool ProjectLoader_v0::load(ResourceGroupRequest* gr, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"resourcegroup-request";
    gr->setGroup(status.project().findResourceGroup(element.attribute("group-id")));
    if (gr->group() == 0) {
        errorPlanXml<<"The referenced resource group does not exist: group id="<<element.attribute("group-id");
        return false;
    }
    gr->group()->registerRequest(gr);
    Q_ASSERT(gr->parent());

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "resource-request") {
            ResourceRequest *r = new ResourceRequest();
            if (load(r, e, status)) {
                gr->parent()->addResourceRequest(r, gr);
            } else {
                errorPlanXml<<"Failed to load resource request";
                delete r;
            }
        }
    }
    // meaning of m_units changed
    int x = element.attribute("units").toInt() -gr->count();
    gr->setUnits(x > 0 ? x : 0);

    return true;
}
#endif
bool ProjectLoader_v0::load(ResourceRequest *rr, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"resource-request";
    rr->setResource(status.project().resource(element.attribute("resource-id")));
    if (rr->resource() == nullptr) {
        warnPlanXml<<"The referenced resource does not exist: resource id="<<element.attribute("resource-id");
        return false;
    }
    rr->setUnits(element.attribute("units").toInt());

    KoXmlElement parent = element.namedItem("required-resources").toElement();
    KoXmlElement e;
    QList<Resource*> required;
    forEachElement(e, parent) {
        if (e.nodeName() == "resource") {
            QString id = e.attribute("id");
            if (id.isEmpty()) {
                errorPlanXml<<"Missing resource id";
                continue;
            }
            Resource *r = status.project().resource(id);
            if (r == nullptr) {
                errorPlanXml<<"The referenced resource does not exist: resource id="<<element.attribute("resource-id");
            } else {
                if (r != rr->resource()) {
                    required << r;
                }
            }
        }
    }
    rr->setRequiredResources(required);
    return true;

}

bool ProjectLoader_v0::load(WorkPackage &wp, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"workpackage";
    Q_UNUSED(status);
    wp.setOwnerName(element.attribute("owner"));
    wp.setOwnerId(element.attribute("owner-id"));
    return true;
}

bool ProjectLoader_v0::loadWpLog(WorkPackage *wp, KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"wplog";
    wp->setOwnerName(element.attribute("owner"));
    wp->setOwnerId(element.attribute("owner-id"));
    wp->setTransmitionStatus(wp->transmitionStatusFromString(element.attribute("status")));
    wp->setTransmitionTime(DateTime(QDateTime::fromString(element.attribute("time"), Qt::ISODate)));
    return load(wp->completion(), element, status);
}

bool ProjectLoader_v0::load(Completion &completion, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"completion";
    QString s;
    completion.setStarted((bool)element.attribute("started", "0").toInt());
    completion.setFinished((bool)element.attribute("finished", "0").toInt());
    s = element.attribute("startTime");
    if (!s.isEmpty()) {
        completion.setStartTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    s = element.attribute("finishTime");
    if (!s.isEmpty()) {
        completion.setFinishTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    completion.setEntrymode(element.attribute("entrymode"));
    if (status.version() < "0.6") {
        if (completion.isStarted()) {
            Completion::Entry *entry = new Completion::Entry(element.attribute("percent-finished", "0").toInt(), Duration::fromString(element.attribute("remaining-effort")),  Duration::fromString(element.attribute("performed-effort")));
            entry->note = element.attribute("note");
            QDate date = completion.startTime().date();
            if (completion.isFinished()) {
                date = completion.finishTime().date();
            }
            // almost the best we can do ;)
            completion.addEntry(date, entry);
        }
    } else {
        KoXmlElement e;
        forEachElement(e, element) {
                if (e.tagName() == "completion-entry") {
                    QDate date;
                    s = e.attribute("date");
                    if (!s.isEmpty()) {
                        date = QDate::fromString(s, Qt::ISODate);
                    }
                    if (!date.isValid()) {
                        warnPlanXml<<"Invalid date: "<<date<<s;
                        continue;
                    }
                    Completion::Entry *entry = new Completion::Entry(e.attribute("percent-finished", "0").toInt(), Duration::fromString(e.attribute("remaining-effort")),  Duration::fromString(e.attribute("performed-effort")));
                    completion.addEntry(date, entry);
                } else if (e.tagName() == "used-effort") {
                    KoXmlElement el;
                    forEachElement(el, e) {
                            if (el.tagName() == "resource") {
                                QString id = el.attribute("id");
                                Resource *r = status.project().resource(id);
                                if (r == nullptr) {
                                    warnPlanXml<<"Cannot find resource, id="<<id;
                                    continue;
                                }
                                Completion::UsedEffort *ue = new Completion::UsedEffort();
                                completion.addUsedEffort(r, ue);
                                load(ue, el, status);
                            }
                    }
                }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Completion::UsedEffort* ue, const KoXmlElement& element, XMLLoaderObject& /*status*/)
{
    debugPlanXml<<"used-effort";
    KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == "actual-effort") {
            QDate date = QDate::fromString(e.attribute("date"), Qt::ISODate);
            if (date.isValid()) {
                Completion::UsedEffort::ActualEffort a;
                a.setNormalEffort(Duration::fromString(e.attribute("normal-effort")));
                a.setOvertimeEffort(Duration::fromString(e.attribute("overtime-effort")));
                ue->setEffort(date, a);
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(Appointment *appointment, const KoXmlElement& element, XMLLoaderObject& status, Schedule &sch)
{
    debugPlanXml<<"appointment";
    Node *node = status.project().findNode(element.attribute("task-id"));
    if (node == nullptr) {
        errorPlanXml<<"The referenced task does not exists: "<<element.attribute("task-id");
        return false;
    }
    Resource *res = status.project().resource(element.attribute("resource-id"));
    if (res == nullptr) {
        errorPlanXml<<"The referenced resource does not exists: resource id="<<element.attribute("resource-id");
        return false;
    }
    if (!res->addAppointment(appointment, sch)) {
        errorPlanXml<<"Failed to add appointment to resource: "<<res->name();
        return false;
    }
    if (! node->addAppointment(appointment, sch)) {
        errorPlanXml<<"Failed to add appointment to node: "<<node->name();
        appointment->resource()->takeAppointment(appointment);
        return false;
    }
    //debugPlanXml<<"res="<<m_resource<<" node="<<m_node;
    AppointmentIntervalList lst = appointment->intervals();
    load(lst, element, status);
    if (lst.isEmpty()) {
        errorPlanXml<<"Appointment interval list is empty (added anyway): "<<node->name()<<res->name();
        return false;
    }
    appointment->setIntervals(lst);
    return true;
}

bool ProjectLoader_v0::load(AppointmentIntervalList& lst, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"appointment-interval-list";
    KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == QLatin1String("appointment-interval") || (status.version() < "0.7.0" && e.tagName() == QLatin1String("interval"))) {
            AppointmentInterval a;
            if (load(a, e, status)) {
                lst.add(a);
            } else {
                errorPlanXml<<"Could not load interval";
            }
        }
    }
    return true;
}

bool ProjectLoader_v0::load(AppointmentInterval& interval, const KoXmlElement& element, XMLLoaderObject& status)
{
    bool ok;
    QString s = element.attribute("start");
    if (!s.isEmpty()) {
        interval.setStartTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    s = element.attribute("end");
    if (!s.isEmpty()) {
        interval.setEndTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    double l = element.attribute("load", "100").toDouble(&ok);
    if (ok) {
        interval.setLoad(l);
    }
    debugPlanXml<<"interval:"<<interval;
    return interval.isValid();
}
