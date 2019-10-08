/* This file is part of the KDE project
 * Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * Copyright (C) 2004, 2010, 2012 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * Copyright (C) 2007 Thorsten Zachmann <zachmann@kde.org>
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
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
#include "kptmaindocument.h"
#include "kptpart.h"
#include "kptview.h"
#include "kptfactory.h"
#include "kptproject.h"
#include "kptlocale.h"
#include "kptresource.h"
#include "kptcontext.h"
#include "kptschedulerpluginloader.h"
#include "kptschedulerplugin.h"
#include "kptbuiltinschedulerplugin.h"
#include "kptschedule.h"
#include "kptcommand.h"
#include "calligraplansettings.h"
#include "kpttask.h"
#include "KPlatoXmlLoader.h"
#include "XmlSaveContext.h"
#include "kptpackage.h"
#include "kptdebug.h"

#include <KoStore.h>
#include <KoXmlReader.h>
#include <KoStoreDevice.h>
#include <KoOdfReadStore.h>
#include <KoUpdater.h>
#include <KoProgressUpdater.h>
#include <KoDocumentInfo.h>

#include <QApplication>
#include <QPainter>
#include <QDir>
#include <QMutableMapIterator>
#include <QTemporaryFile>

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <KIO/CopyJob>

#include <kundo2command.h>

#ifdef HAVE_KHOLIDAYS
#include <KHolidays/HolidayRegion>
#endif

namespace KPlato
{

MainDocument::MainDocument(KoPart *part)
        : KoDocument(part),
        m_project( 0 ),
        m_context( 0 ), m_xmlLoader(),
        m_loadingTemplate( false ),
        m_loadingSharedResourcesTemplate( false ),
        m_viewlistModified( false ),
        m_checkingForWorkPackages( false ),
        m_loadingSharedProject(false),
        m_skipSharedProjects(false),
        m_isTaskModule(false),
        m_calculationCommand(nullptr),
        m_currentCalculationManager(nullptr),
        m_nextCalculationManager(nullptr)
{
    Q_ASSERT(part);
    setAlwaysAllowSaving(true);
    m_config.setReadWrite( isReadWrite() );

    loadSchedulerPlugins();

    setProject( new Project( m_config ) ); // after config & plugins are loaded
    m_project->setId( m_project->uniqueNodeId() );
    m_project->registerNodeId( m_project ); // register myself

    connect(this, &MainDocument::insertSharedProject, this, &MainDocument::slotInsertSharedProject);
}


MainDocument::~MainDocument()
{
    qDeleteAll( m_schedulerPlugins );
    if ( m_project ) {
        m_project->deref(); // deletes if last user
    }
    qDeleteAll( m_mergedPackages );
    delete m_context;
    delete m_calculationCommand;
}

void MainDocument::slotNodeChanged(Node *node, int property)
{
    switch (property) {
        case Node::TypeProperty:
        case Node::ResourceRequestProperty:
        case Node::ConstraintTypeProperty:
        case Node::StartConstraintProperty:
        case Node::EndConstraintProperty:
        case Node::PriorityProperty:
        case Node::EstimateProperty:
        case Node::EstimateRiskProperty:
            setCalculationNeeded();
            break;
        case Node::EstimateOptimisticProperty:
        case Node::EstimatePessimisticProperty:
            if (node->estimate()->risktype() != Estimate::Risk_None) {
                setCalculationNeeded();
            }
            break;
        default:
            break;
    }
}

void MainDocument::slotScheduleManagerChanged(ScheduleManager *sm, int property)
{
    if (sm->schedulingMode() == ScheduleManager::AutoMode) {
        switch (property) {
            case ScheduleManager::DirectionProperty:
            case ScheduleManager::OverbookProperty:
            case ScheduleManager::DistributionProperty:
            case ScheduleManager::SchedulingModeProperty:
            case ScheduleManager::GranularityProperty:
                setCalculationNeeded();
                break;
            default:
                break;
        }
    }
}

void MainDocument::setCalculationNeeded()
{
    for (ScheduleManager *sm : m_project->allScheduleManagers()) {
        if (sm->isBaselined()) {
            continue;
        }
        if (sm->schedulingMode() == ScheduleManager::AutoMode) {
            m_nextCalculationManager = sm;
            break;
        }
    }
    if (!m_currentCalculationManager) {
        m_currentCalculationManager = m_nextCalculationManager;
        m_nextCalculationManager = nullptr;

        QTimer::singleShot(0, this, &MainDocument::slotStartCalculation);
    }
}

void MainDocument::slotStartCalculation()
{
    if (m_currentCalculationManager) {
        m_calculationCommand = new CalculateScheduleCmd(*m_project, m_currentCalculationManager);
        m_calculationCommand->redo();
    }
}

void MainDocument::slotCalculationFinished(Project *p, ScheduleManager *sm)
{
    if (sm != m_currentCalculationManager) {
        return;
    }
    delete m_calculationCommand;
    m_calculationCommand = nullptr;
    m_currentCalculationManager = m_nextCalculationManager;
    m_nextCalculationManager = nullptr;
    if (m_currentCalculationManager) {
        QTimer::singleShot(0, this, &MainDocument::slotStartCalculation);
    }
}

void MainDocument::setReadWrite( bool rw )
{
    m_config.setReadWrite( rw );
    KoDocument::setReadWrite( rw );
}

void MainDocument::loadSchedulerPlugins()
{
    // Add built-in scheduler
    addSchedulerPlugin( "Built-in", new BuiltinSchedulerPlugin( this ) );

    // Add all real scheduler plugins
    SchedulerPluginLoader *loader = new SchedulerPluginLoader(this);
    connect(loader, &SchedulerPluginLoader::pluginLoaded, this, &MainDocument::addSchedulerPlugin);
    loader->loadAllPlugins();
}

void MainDocument::addSchedulerPlugin( const QString &key, SchedulerPlugin *plugin)
{
    debugPlan<<plugin;
    m_schedulerPlugins[key] = plugin;
}

void MainDocument::configChanged()
{
    //m_project->setConfig( m_config );
}

void MainDocument::setProject( Project *project )
{
    if ( m_project ) {
        delete m_project;
    }
    m_project = project;
    if ( m_project ) {
        connect( m_project, &Project::projectChanged, this, &MainDocument::changed );
//        m_project->setConfig( config() );
        m_project->setSchedulerPlugins( m_schedulerPlugins );

        // For auto scheduling
        delete m_calculationCommand;
        m_calculationCommand = nullptr;
        m_currentCalculationManager = nullptr;
        m_nextCalculationManager = nullptr;
        connect(m_project, &Project::nodeAdded, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::nodeRemoved, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::relationAdded, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::relationRemoved, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::calendarChanged, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::defaultCalendarChanged, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::calendarAdded, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::calendarRemoved, this, &MainDocument::setCalculationNeeded);
        connect(m_project, &Project::scheduleManagerChanged, this, &MainDocument::slotScheduleManagerChanged);
        connect(m_project, &Project::nodeChanged, this, &MainDocument::slotNodeChanged);
        connect(m_project, &Project::sigCalculationFinished, this, &MainDocument::slotCalculationFinished);
    }
    m_aboutPage.setProject( project );
    emit changed();
}

bool MainDocument::loadOdf( KoOdfReadStore &odfStore )
{
    warnPlan<< "OpenDocument not supported, let's try native xml format";
    return loadXML( odfStore.contentDoc(), 0 ); // We have only one format, so try to load that!
}

bool MainDocument::loadXML( const KoXmlDocument &document, KoStore* )
{
    QPointer<KoUpdater> updater;
    if (progressUpdater()) {
        updater = progressUpdater()->startSubtask(1, "Plan::Part::loadXML");
        updater->setProgress(0);
        m_xmlLoader.setUpdater( updater );
    }

    QString value;
    KoXmlElement plan = document.documentElement();

    // Check if this is the right app
    value = plan.attribute( "mime", QString() );
    if ( value.isEmpty() ) {
        errorPlan << "No mime type specified!";
        setErrorMessage( i18n( "Invalid document. No mimetype specified." ) );
        return false;
    }
    if ( value == "application/x-vnd.kde.kplato" ) {
        if (updater) {
            updater->setProgress(5);
        }
        m_xmlLoader.setMimetype( value );
        QString message;
        Project *newProject = new Project(m_config, false);
        KPlatoXmlLoader loader( m_xmlLoader, newProject );
        bool ok = loader.load( plan );
        if ( ok ) {
            setProject( newProject );
            setModified( false );
            debugPlan<<newProject->schedules();
            // Cleanup after possible bug:
            // There should *not* be any deleted schedules (or with parent == 0)
            foreach ( Node *n, newProject->nodeDict()) {
                foreach ( Schedule *s, n->schedules()) {
                    if ( s->isDeleted() ) { // true also if parent == 0
                        errorPlan<<n->name()<<s;
                        n->takeSchedule( s );
                        delete s;
                    }
                }
            }
        } else {
            setErrorMessage( loader.errorMessage() );
            delete newProject;
        }
        if (updater) {
            updater->setProgress(100); // the rest is only processing, not loading
        }
        emit changed();
        return ok;
    }
    if ( value != "application/x-vnd.kde.plan" ) {
        errorPlan << "Unknown mime type " << value;
        setErrorMessage( i18n( "Invalid document. Expected mimetype application/x-vnd.kde.plan, got %1", value ) );
        return false;
    }
    QString syntaxVersion = plan.attribute( "version", PLAN_FILE_SYNTAX_VERSION );
    m_xmlLoader.setVersion( syntaxVersion );
    if ( syntaxVersion > PLAN_FILE_SYNTAX_VERSION ) {
        KMessageBox::ButtonCode ret = KMessageBox::warningContinueCancel(
                      0, i18n( "This document was created with a newer version of Plan (syntax version: %1)\n"
                               "Opening it in this version of Plan will lose some information.", syntaxVersion ),
                      i18n( "File-Format Mismatch" ), KGuiItem( i18n( "Continue" ) ) );
        if ( ret == KMessageBox::Cancel ) {
            setErrorMessage( "USER_CANCELED" );
            return false;
        }
    }
    if (updater) updater->setProgress(5);
/*
#ifdef KOXML_USE_QDOM
    int numNodes = plan.childNodes().count();
#else
    int numNodes = plan.childNodesCount();
#endif
*/
#if 0
This test does not work any longer. KoXml adds a couple of elements not present in the file!!
    if ( numNodes > 2 ) {
        //TODO: Make a proper bitching about this
        debugPlan <<"*** Error ***";
        debugPlan <<"  Children count should be maximum 2, but is" << numNodes;
        return false;
    }
#endif
    m_xmlLoader.startLoad();
    KoXmlNode n = plan.firstChild();
    for ( ; ! n.isNull(); n = n.nextSibling() ) {
        if ( ! n.isElement() ) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if ( e.tagName() == "project" ) {
            Project *newProject = new Project(m_config, true);
            m_xmlLoader.setProject( newProject );
            if ( newProject->load( e, m_xmlLoader ) ) {
                if ( newProject->id().isEmpty() ) {
                    newProject->setId( newProject->uniqueNodeId() );
                    newProject->registerNodeId( newProject );
                }
                // The load went fine. Throw out the old project
                setProject( newProject );
                // Cleanup after possible bug:
                // There should *not* be any deleted schedules (or with parent == 0)
                foreach ( Node *n, newProject->nodeDict()) {
                    foreach ( Schedule *s, n->schedules()) {
                        if ( s->isDeleted() ) { // true also if parent == 0
                            errorPlan<<n->name()<<s;
                            n->takeSchedule( s );
                            delete s;
                        }
                    }
                }
            } else {
                delete newProject;
                m_xmlLoader.addMsg( XMLLoaderObject::Errors, "Loading of project failed" );
                //TODO add some ui here
            }
        }
    }
    m_xmlLoader.stopLoad();

    if (updater) updater->setProgress(100); // the rest is only processing, not loading

    setModified( false );
    emit changed();
    return true;
}

QDomDocument MainDocument::saveXML()
{
    debugPlan;
    // Save the project
    XmlSaveContext context(m_project);
    context.save();

    return context.document;
}

QDomDocument MainDocument::saveWorkPackageXML( const Node *node, long id, Resource *resource )
{
    debugPlanWp<<resource<<node;
    QDomDocument document( "plan" );

    document.appendChild( document.createProcessingInstruction(
                "xml",
    "version=\"1.0\" encoding=\"UTF-8\"" ) );

    QDomElement doc = document.createElement( "planwork" );
    doc.setAttribute( "editor", "Plan" );
    doc.setAttribute( "mime", "application/x-vnd.kde.plan.work" );
    doc.setAttribute( "version", PLANWORK_FILE_SYNTAX_VERSION );
    doc.setAttribute( "plan-version", PLAN_FILE_SYNTAX_VERSION );
    document.appendChild( doc );

    // Work package info
    QDomElement wp = document.createElement( "workpackage" );
    if ( resource ) {
        wp.setAttribute( "owner", resource->name() );
        wp.setAttribute( "owner-id", resource->id() );
    }
    wp.setAttribute( "time-tag", QDateTime::currentDateTime().toString( Qt::ISODate ) );
    wp.setAttribute("save-url", m_project->workPackageInfo().retrieveUrl.toString(QUrl::None));
    wp.setAttribute("load-url", m_project->workPackageInfo().publishUrl.toString(QUrl::None));
    debugPlanWp<<"publish:"<<m_project->workPackageInfo().publishUrl.toString(QUrl::None);
    debugPlanWp<<"retrieve:"<<m_project->workPackageInfo().retrieveUrl.toString(QUrl::None);
    doc.appendChild( wp );

    // Save the project
    m_project->saveWorkPackageXML( doc, node, id );

    return document;
}

bool MainDocument::saveWorkPackageToStream( QIODevice *dev, const Node *node, long id, Resource *resource )
{
    QDomDocument doc = saveWorkPackageXML( node, id, resource );
    // Save to buffer
    QByteArray s = doc.toByteArray(); // utf8 already
    dev->open( QIODevice::WriteOnly );
    int nwritten = dev->write( s.data(), s.size() );
    if ( nwritten != (int)s.size() ) {
        warnPlanWp<<"wrote:"<<nwritten<<"- expected:"<< s.size();
    }
    return nwritten == (int)s.size();
}

bool MainDocument::saveWorkPackageFormat( const QString &file, const Node *node, long id, Resource *resource  )
{
    debugPlanWp <<"Saving to store";

    KoStore::Backend backend = KoStore::Zip;
#ifdef QCA2
/*    if ( d->m_specialOutputFlag == SaveEncrypted ) {
        backend = KoStore::Encrypted;
        debugPlan <<"Saving using encrypted backend.";
    }*/
#endif

    QByteArray mimeType = "application/x-vnd.kde.plan.work";
    debugPlanWp <<"MimeType=" << mimeType;

    KoStore *store = KoStore::createStore( file, KoStore::Write, mimeType, backend );
/*    if ( d->m_specialOutputFlag == SaveEncrypted && !d->m_password.isNull( ) ) {
        store->setPassword( d->m_password );
    }*/
    if ( store->bad() ) {
        setErrorMessage( i18n( "Could not create the workpackage file for saving: %1", file ) ); // more details needed?
        delete store;
        return false;
    }
    // Tell KoStore not to touch the file names


    if ( ! store->open( "root" ) ) {
        setErrorMessage( i18n( "Not able to write '%1'. Partition full?", QString( "maindoc.xml") ) );
        delete store;
        return false;
    }
    KoStoreDevice dev( store );
    if ( !saveWorkPackageToStream( &dev, node, id, resource ) || !store->close() ) {
        errorPlanWp <<"saveToStream failed";
        delete store;
        return false;
    }
    node->documents().saveToStore( store );

    debugPlanWp <<"Saving done of url:" << file;
    if ( !store->finalize() ) {
        delete store;
        return false;
    }
    // Success
    delete store;

    return true;
}

bool MainDocument::saveWorkPackageUrl( const QUrl &_url, const Node *node, long id, Resource *resource )
{
    debugPlanWp<<_url;
    QApplication::setOverrideCursor( Qt::WaitCursor );
    emit statusBarMessage( i18n("Saving...") );
    bool ret = false;
    ret = saveWorkPackageFormat( _url.path(), node, id, resource ); // kzip don't handle file://
    QApplication::restoreOverrideCursor();
    emit clearStatusBarMessage();
    return ret;
}

bool MainDocument::loadWorkPackage( Project &project, const QUrl &url )
{
    debugPlanWp<<url;
    if ( ! url.isLocalFile() ) {
        warnPlanWp<<Q_FUNC_INFO<<"TODO: download if url not local";
        return false;
    }
    KoStore *store = KoStore::createStore( url.path(), KoStore::Read, "", KoStore::Auto );
    if ( store->bad() ) {
//        d->lastErrorMessage = i18n( "Not a valid Calligra file: %1", file );
        errorPlanWp<<"bad store"<<url.toDisplayString();
        delete store;
//        QApplication::restoreOverrideCursor();
        return false;
    }
    if ( ! store->open( "root" ) ) { // "old" file format (maindoc.xml)
        // i18n( "File does not have a maindoc.xml: %1", file );
        errorPlanWp<<"No root"<<url.toDisplayString();
        delete store;
//        QApplication::restoreOverrideCursor();
        return false;
    }
    Package *package = 0;
    KoXmlDocument doc;
    QString errorMsg; // Error variables for QDomDocument::setContent
    int errorLine, errorColumn;
    bool ok = doc.setContent( store->device(), &errorMsg, &errorLine, &errorColumn );
    if ( ! ok ) {
        errorPlanWp << "Parsing error in " << url.url() << "! Aborting!" << endl
                << " In line: " << errorLine << ", column: " << errorColumn << endl
                << " Error message: " << errorMsg;
        //d->lastErrorMessage = i18n( "Parsing error in %1 at line %2, column %3\nError message: %4",filename  ,errorLine, errorColumn , QCoreApplication::translate("QXml", errorMsg.toUtf8(), 0, QCoreApplication::UnicodeUTF8));
    } else {
        package = loadWorkPackageXML( project, store->device(), doc, url );
        if ( package ) {
            package->url = url;
            m_workpackages.insert( package->timeTag, package );
            if (!m_mergedPackages.contains(package->timeTag)) {
                m_mergedPackages[package->timeTag] = package->project; // register this for next time
            }
        } else {
            ok = false;
        }
    }
    store->close();
    //###
    if ( ok && package && package->settings.documents ) {
        ok = extractFiles( store, package );
    }
    delete store;
    if ( ! ok ) {
//        QApplication::restoreOverrideCursor();
        return false;
    }
    return true;
}

Package *MainDocument::loadWorkPackageXML( Project &project, QIODevice *, const KoXmlDocument &document, const QUrl &url )
{
    QString value;
    bool ok = true;
    Project *proj = 0;
    Package *package = 0;
    KoXmlElement plan = document.documentElement();

    // Check if this is the right app
    value = plan.attribute( "mime", QString() );
    if ( value.isEmpty() ) {
        errorPlanWp<<Q_FUNC_INFO<<"No mime type specified!";
        setErrorMessage( i18n( "Invalid document. No mimetype specified." ) );
        return 0;
    } else if ( value == "application/x-vnd.kde.kplato.work" ) {
        m_xmlLoader.setMimetype( value );
        m_xmlLoader.setWorkVersion( plan.attribute( "version", "0.0.0" ) );
        proj = new Project();
        KPlatoXmlLoader loader( m_xmlLoader, proj );
        ok = loader.loadWorkpackage( plan );
        if ( ! ok ) {
            setErrorMessage( loader.errorMessage() );
            delete proj;
            return 0;
        }
        package = loader.package();
        package->timeTag = QDateTime::fromString( loader.timeTag(), Qt::ISODate );
    } else if ( value != "application/x-vnd.kde.plan.work" ) {
        errorPlanWp << "Unknown mime type " << value;
        setErrorMessage( i18n( "Invalid document. Expected mimetype application/x-vnd.kde.plan.work, got %1", value ) );
        return 0;
    } else {
        if (plan.attribute("editor") != QStringLiteral("PlanWork")) {
            warnPlanWp<<"Skipped work package file not generated with PlanWork:"<<plan.attribute("editor")<<url;
            return nullptr;
        }
        QString syntaxVersion = plan.attribute( "version", "0.0.0" );
        m_xmlLoader.setWorkVersion( syntaxVersion );
        if ( syntaxVersion > PLANWORK_FILE_SYNTAX_VERSION ) {
            KMessageBox::ButtonCode ret = KMessageBox::warningContinueCancel(
                    0, i18n( "This document was created with a newer version of PlanWork (syntax version: %1)\n"
                    "Opening it in this version of PlanWork will lose some information.", syntaxVersion ),
                    i18n( "File-Format Mismatch" ), KGuiItem( i18n( "Continue" ) ) );
            if ( ret == KMessageBox::Cancel ) {
                setErrorMessage( "USER_CANCELED" );
                return 0;
            }
        }
        m_xmlLoader.setVersion( plan.attribute( "plan-version", PLAN_FILE_SYNTAX_VERSION ) );
        m_xmlLoader.startLoad();
        proj = new Project();
        package = new Package();
        package->project = proj;
        KoXmlNode n = plan.firstChild();
        for ( ; ! n.isNull(); n = n.nextSibling() ) {
            if ( ! n.isElement() ) {
                continue;
            }
            KoXmlElement e = n.toElement();
            if ( e.tagName() == "project" ) {
                m_xmlLoader.setProject( proj );
                ok = proj->load( e, m_xmlLoader );
                if ( ! ok ) {
                    m_xmlLoader.addMsg( XMLLoaderObject::Errors, "Loading of work package failed" );
                    warnPlanWp<<"Skip workpackage:"<<"Loading project failed";
                    //TODO add some ui here
                }
            } else if ( e.tagName() == "workpackage" ) {
                package->timeTag = QDateTime::fromString( e.attribute( "time-tag" ), Qt::ISODate );
                package->ownerId = e.attribute( "owner-id" );
                package->ownerName = e.attribute( "owner" );
                debugPlan<<"workpackage:"<<package->timeTag<<package->ownerId<<package->ownerName;
                KoXmlElement elem;
                forEachElement( elem, e ) {
                    if ( elem.tagName() != "settings" ) {
                        continue;
                    }
                    package->settings.usedEffort = (bool)elem.attribute( "used-effort" ).toInt();
                    package->settings.progress = (bool)elem.attribute( "progress" ).toInt();
                    package->settings.documents = (bool)elem.attribute( "documents" ).toInt();
                }
            }
        }
        if ( proj->numChildren() > 0 ) {
            package->task = static_cast<Task*>( proj->childNode( 0 ) );
            package->toTask = qobject_cast<Task*>( m_project->findNode( package->task->id() ) );
            WorkPackage &wp = package->task->workPackage();
            if ( wp.ownerId().isEmpty() ) {
                wp.setOwnerId( package->ownerId );
                wp.setOwnerName( package->ownerName );
            }
            if (wp.ownerId() != package->ownerId) {
                warnPlanWp<<"Current owner:"<<wp.ownerName()<<"not the same as package owner:"<<package->ownerName;
            }
            debugPlanWp<<"Task set:"<<package->task->name();
        }
        m_xmlLoader.stopLoad();
    }
    if (ok && proj->id() != project.id()) {
        debugPlanWp<<"Skip workpackage:"<<"Not the correct project";
        ok = false;
    }
    if (ok && (package->task == nullptr)) {
        warnPlanWp<<"Skip workpackage:"<<"No task in workpackage file";
        ok = false;
    }
    if (ok && (package->toTask == nullptr)) {
        warnPlanWp<<"Skip workpackage:"<<"Cannot find task:"<<package->task->id()<<package->task->name();
        ok = false;
    }
    if (ok && !package->timeTag.isValid()) {
        warnPlanWp<<"Work package is not time tagged:"<<package->task->name()<<package->url;
        ok = false;
    }
    if (ok && m_mergedPackages.contains(package->timeTag)) {
        debugPlanWp<<"Skip workpackage:"<<"already merged:"<<package->task->name()<<package->url;
        ok = false; // already merged
    }
    if (!ok) {
        delete proj;
        delete package;
        return nullptr;
    }
    return package;
}

bool MainDocument::extractFiles( KoStore *store, Package *package )
{
    if ( package->task == 0 ) {
        errorPlan<<"No task!";
        return false;
    }
    foreach ( Document *doc, package->task->documents().documents() ) {
        if ( ! doc->isValid() || doc->type() != Document::Type_Product || doc->sendAs() != Document::SendAs_Copy ) {
            continue;
        }
        if ( ! extractFile( store, package, doc ) ) {
            return false;
        }
    }
    return true;
}

bool MainDocument::extractFile( KoStore *store, Package *package, const Document *doc )
{
    QTemporaryFile tmpfile;
    if ( ! tmpfile.open() ) {
        errorPlan<<"Failed to open temporary file";
        return false;
    }
    if ( ! store->extractFile( doc->url().fileName(), tmpfile.fileName() ) ) {
        errorPlan<<"Failed to extract file:"<<doc->url().fileName()<<"to:"<<tmpfile.fileName();
        return false;
    }
    package->documents.insert( tmpfile.fileName(), doc->url() );
    tmpfile.setAutoRemove( false );
    debugPlan<<"extracted:"<<doc->url().fileName()<<"->"<<tmpfile.fileName();
    return true;
}

void MainDocument::autoCheckForWorkPackages()
{
    QTimer *timer = qobject_cast<QTimer*>(sender());
    if (m_project && m_project->workPackageInfo().checkForWorkPackages) {
        checkForWorkPackages( true );
    }
    if (timer && timer->interval() != 10000) {
        timer->stop();
        timer->setInterval(10000);
        timer->start();
    }
}

void MainDocument::checkForWorkPackages( bool keep )
{
    if (m_checkingForWorkPackages || m_project == nullptr || m_project->numChildren() == 0 || m_project->workPackageInfo().retrieveUrl.isEmpty()) {
        return;
    }
    if ( ! keep ) {
        qDeleteAll( m_mergedPackages );
        m_mergedPackages.clear();
    }
    QDir dir( m_project->workPackageInfo().retrieveUrl.path(), "*.planwork" );
    m_infoList = dir.entryInfoList( QDir::Files | QDir::Readable, QDir::Time );
    checkForWorkPackage();
    return;
}

void MainDocument::checkForWorkPackage()
{
    if ( ! m_infoList.isEmpty() ) {
        m_checkingForWorkPackages = true;
        QUrl url = QUrl::fromLocalFile( m_infoList.takeLast().absoluteFilePath() );
        if (!m_skipUrls.contains(url) && !loadWorkPackage(*m_project, url)) {
            m_skipUrls << url;
            debugPlanWp<<"skip url:"<<url;
        }
        if ( ! m_infoList.isEmpty() ) {
            QTimer::singleShot ( 0, this, &MainDocument::checkForWorkPackage );
            return;
        }
        // Merge our workpackages
        if ( ! m_workpackages.isEmpty() ) {
            emit workPackageLoaded();
        }
        m_checkingForWorkPackages = false;
    }
}

void MainDocument::terminateWorkPackage( const Package *package )
{
    debugPlanWp<<package->toTask<<package->url;
    if (m_workpackages.value(package->timeTag) == package) {
        m_workpackages.remove(package->timeTag);
    }
    QFile file( package->url.path() );
    if ( ! file.exists() ) {
        warnPlanWp<<"File does not exist:"<<package->toTask<<package->url;
        return;
    }
    Project::WorkPackageInfo wpi = m_project->workPackageInfo();
    debugPlanWp<<"retrieve:"<<wpi.retrieveUrl<<"archive:"<<wpi.archiveUrl;
    bool rename = wpi.retrieveUrl == package->url.adjusted(QUrl::RemoveFilename);
    if (wpi.archiveAfterRetrieval && wpi.archiveUrl.isValid()) {
        QDir dir(wpi.archiveUrl.path());
        if ( ! dir.exists() ) {
            if ( ! dir.mkpath( dir.path() ) ) {
                //TODO message
                warnPlanWp<<"Failed to create archive directory:"<<dir.path();
                return;
            }
        }
        QFileInfo from( file );
        QString name = dir.absolutePath() + '/' + from.fileName();
        debugPlanWp<<"rename:"<<rename;
        if (rename ? !file.rename(name) : !file.copy(name)) {
            // try to create a unique name in case name already existed
            debugPlanWp<<"Archive exists, create unique file name";
            name = dir.absolutePath() + '/';
            name += from.completeBaseName() + "-%1";
            if ( ! from.suffix().isEmpty() ) {
                name += '.' + from.suffix();
            }
            int i = 0;
            bool ok = false;
            while ( ! ok && i < 1000 ) {
                ++i;
                ok = rename ? QFile::rename(file.fileName(), name.arg(i)) : QFile::copy(file.fileName(), name.arg(i));
            }
            if ( ! ok ) {
                //TODO message
                warnPlanWp<<"terminateWorkPackage: Failed to save"<<file.fileName();
            }
        }
    } else if (wpi.deleteAfterRetrieval) {
        if (rename) {
            debugPlanWp<<"removed package file:"<<file;
            file.remove();
        } else {
            debugPlanWp<<"package file not in 'from' dir:"<<file;
        }
    } else {
        warnPlanWp<<"Cannot terminate package, archive:"<<wpi.archiveUrl;
    }
}

void MainDocument::paintContent( QPainter &, const QRect &)
{
    // Don't embed this app!!!
}

void MainDocument::slotViewDestroyed()
{
}

void MainDocument::setLoadingTemplate(bool loading)
{
    m_loadingTemplate = loading;
}

void MainDocument::setLoadingSharedResourcesTemplate(bool loading)
{
    m_loadingSharedResourcesTemplate = loading;
}

bool MainDocument::completeLoading( KoStore *store )
{
    // If we get here the new project is loaded and set
    if (m_loadingSharedProject) {
        // this file is loaded by another project
        // to read resource appointments,
        // so we must not load any extra stuff
        return true;
    }
    if ( m_loadingTemplate ) {
        //debugPlan<<"Loading template, generate unique ids";
        m_project->generateUniqueIds();
        m_project->setConstraintStartTime( QDateTime(QDate::currentDate(), QTime(0, 0, 0), Qt::LocalTime) );
        m_project->setConstraintEndTime( m_project->constraintStartTime().addYears( 2 ) );
        m_project->locale()->setCurrencyLocale(QLocale::AnyLanguage, QLocale::AnyCountry);
        m_project->locale()->setCurrencySymbol(QString());
    } else if ( isImporting() ) {
        // NOTE: I don't think this is a good idea.
        // Let the filter generate ids for non-plan files.
        // If the user wants to create a new project from an old one,
        // he should use Tools -> Insert Project File

        //m_project->generateUniqueNodeIds();
    }
    if (m_loadingSharedResourcesTemplate && m_project->calendarCount() > 0) {
        Calendar *c = m_project->calendarAt(0);
        c->setTimeZone(QTimeZone::systemTimeZone());
    }
    if (m_project->useSharedResources() && !m_project->sharedResourcesFile().isEmpty() && !m_skipSharedProjects) {
        QUrl url = QUrl::fromLocalFile(m_project->sharedResourcesFile());
        if (url.isValid()) {
            insertResourcesFile(url, m_project->loadProjectsAtStartup() ? m_project->sharedProjectsUrl() : QUrl());
        }
    }
    if ( store == 0 ) {
        // can happen if loading a template
        debugPlan<<"No store";
        return true; // continue anyway
    }
    delete m_context;
    m_context = new Context();
    KoXmlDocument doc;
    if ( loadAndParse( store, "context.xml", doc ) ) {
        store->close();
        m_context->load( doc );
    } else warnPlan<<"No context";
    return true;
}

// TODO:
// Due to splitting of KoDocument into a document and a part,
// we simulate the old behaviour by registering all views in the document.
// Find a better solution!
void MainDocument::registerView( View* view )
{
    if ( view && ! m_views.contains( view ) ) {
        m_views << QPointer<View>( view );
    }
}

bool MainDocument::completeSaving( KoStore *store )
{
    foreach ( View *view, m_views ) {
        if ( view ) {
            if ( store->open( "context.xml" ) ) {
                if ( m_context == 0 ) m_context = new Context();
                QDomDocument doc = m_context->save( view );

                KoStoreDevice dev( store );
                QByteArray s = doc.toByteArray(); // this is already Utf8!
                (void)dev.write( s.data(), s.size() );
                (void)store->close();

                m_viewlistModified = false;
                emit viewlistModified( false );
            }
            break;
        }
    }
    return true;
}

bool MainDocument::loadAndParse(KoStore *store, const QString &filename, KoXmlDocument &doc)
{
    //debugPlan << "oldLoadAndParse: Trying to open " << filename;

    if (!store->open(filename))
    {
        warnPlan << "Entry " << filename << " not found!";
//        d->lastErrorMessage = i18n( "Could not find %1",filename );
        return false;
    }
    // Error variables for QDomDocument::setContent
    QString errorMsg;
    int errorLine, errorColumn;
    bool ok = doc.setContent( store->device(), &errorMsg, &errorLine, &errorColumn );
    if ( !ok )
    {
        errorPlan << "Parsing error in " << filename << "! Aborting!" << endl
            << " In line: " << errorLine << ", column: " << errorColumn << endl
            << " Error message: " << errorMsg;
/*        d->lastErrorMessage = i18n( "Parsing error in %1 at line %2, column %3\nError message: %4"
                              ,filename  ,errorLine, errorColumn ,
                              QCoreApplication::translate("QXml", errorMsg.toUtf8(), 0,
                                  QCoreApplication::UnicodeUTF8));*/
        store->close();
        return false;
    }
    debugPlan << "File " << filename << " loaded and parsed";
    return true;
}

void MainDocument::insertFile( const QUrl &url, Node *parent, Node *after )
{
    Part *part = new Part( this );
    MainDocument *doc = new MainDocument( part );
    part->setDocument( doc );
    doc->disconnect(); // doc shall not handle feedback from openUrl()
    doc->setAutoSave( 0 ); //disable
    doc->m_insertFileInfo.url = url;
    doc->m_insertFileInfo.parent = parent;
    doc->m_insertFileInfo.after = after;
    connect(doc, &KoDocument::completed, this, &MainDocument::insertFileCompleted);
    connect(doc, &KoDocument::canceled, this, &MainDocument::insertFileCancelled);

    doc->openUrl( url );
}

void MainDocument::insertFileCompleted()
{
    debugPlan<<sender();
    MainDocument *doc = qobject_cast<MainDocument*>( sender() );
    if ( doc ) {
        Project &p = doc->getProject();
        insertProject( p, doc->m_insertFileInfo.parent, doc->m_insertFileInfo.after );
        doc->documentPart()->deleteLater(); // also deletes document
    } else {
        KMessageBox::error( 0, i18n("Internal error, failed to insert file.") );
    }
}

void MainDocument::insertResourcesFile(const QUrl &url, const QUrl &projects)
{
    insertSharedProjects(projects); // prepare for insertion after shared resources
    m_sharedProjectsFiles.removeAll(url); // resource file is not a project

    Part *part = new Part( this );
    MainDocument *doc = new MainDocument( part );
    doc->m_skipSharedProjects = true; // should not have shared projects, but...
    part->setDocument( doc );
    doc->disconnect(); // doc shall not handle feedback from openUrl()
    doc->setAutoSave( 0 ); //disable
    doc->setCheckAutoSaveFile(false);
    connect(doc, &KoDocument::completed, this, &MainDocument::insertResourcesFileCompleted);
    connect(doc, &KoDocument::canceled, this, &MainDocument::insertFileCancelled);

    doc->openUrl( url );

}

void MainDocument::insertResourcesFileCompleted()
{
    debugPlanShared<<sender();
    MainDocument *doc = qobject_cast<MainDocument*>( sender() );
    if (doc) {
        Project &p = doc->getProject();
        mergeResources(p);
        m_project->setSharedResourcesLoaded(true);
        doc->documentPart()->deleteLater(); // also deletes document
        slotInsertSharedProject(); // insert shared bookings
    } else {
        KMessageBox::error( 0, i18n("Internal error, failed to insert file.") );
    }
}

void MainDocument::insertFileCancelled( const QString &error )
{
    debugPlan<<sender()<<"error="<<error;
    if ( ! error.isEmpty() ) {
        KMessageBox::error( 0, error );
    }
    MainDocument *doc = qobject_cast<MainDocument*>( sender() );
    if ( doc ) {
        doc->documentPart()->deleteLater(); // also deletes document
    }
}

void MainDocument::clearResourceAssignments()
{
    foreach (Resource *r, m_project->resourceList()) {
        r->clearExternalAppointments();
    }
}

void MainDocument::loadResourceAssignments(QUrl url)
{
    insertSharedProjects(url);
    slotInsertSharedProject();
}

void MainDocument::insertSharedProjects(const QList<QUrl> &urls)
{
    clearResourceAssignments();
    m_sharedProjectsFiles = urls;
    slotInsertSharedProject();
}

void MainDocument::insertSharedProjects(const QUrl &url)
{
    m_sharedProjectsFiles.clear();
    QFileInfo fi(url.path());
    if (!fi.exists()) {
        return;
    }
    if (fi.isFile()) {
        m_sharedProjectsFiles = QList<QUrl>() << url;
        debugPlan<<"Get all projects in file:"<<url;
    } else if (fi.isDir()) {
        // Get all plan files in this directory
        debugPlan<<"Get all projects in dir:"<<url;
        QDir dir = fi.dir();
        foreach(const QString &f, dir.entryList(QStringList()<<"*.plan")) {
            QString path = dir.canonicalPath();
            if (path.isEmpty()) {
                continue;
            }
            path += '/' + f;
            QUrl u(path);
            u.setScheme("file");
            m_sharedProjectsFiles << u;
        }
    } else {
        warnPlan<<"Unknown url:"<<url<<url.path()<<url.fileName();
        return;
    }
    clearResourceAssignments();
}

void MainDocument::slotInsertSharedProject()
{
    debugPlan<<m_sharedProjectsFiles;
    if (m_sharedProjectsFiles.isEmpty()) {
        return;
    }
    Part *part = new Part( this );
    MainDocument *doc = new MainDocument( part );
    doc->m_skipSharedProjects = true; // never load recursively
    part->setDocument( doc );
    doc->disconnect(); // doc shall not handle feedback from openUrl()
    doc->setAutoSave( 0 ); //disable
    doc->setCheckAutoSaveFile(false);
    doc->m_loadingSharedProject = true;
    connect(doc, &KoDocument::completed, this, &MainDocument::insertSharedProjectCompleted);
    connect(doc, &KoDocument::canceled, this, &MainDocument::insertSharedProjectCancelled);

    doc->openUrl(m_sharedProjectsFiles.takeFirst());
}

void MainDocument::insertSharedProjectCompleted()
{
    debugPlanShared<<sender();
    MainDocument *doc = qobject_cast<MainDocument*>( sender() );
    if (doc) {
        Project &p = doc->getProject();
        debugPlanShared<<m_project->id()<<"Loaded project:"<<p.id()<<p.name();
        if (p.id() != m_project->id() && p.isScheduled(ANYSCHEDULED)) {
            // FIXME: improve!
            // find a suitable schedule
            ScheduleManager *sm = 0;
            foreach(ScheduleManager *m, p.allScheduleManagers()) {
                if (m->isBaselined()) {
                    sm = m;
                    break;
                }
                if (m->isScheduled()) {
                    sm = m; // take the last one, more likely to be subschedule
                }
            }
            if (sm) {
                foreach(Resource *r, p.resourceList()) {
                    Resource *res = m_project->resource(r->id());
                    if (res && res->isShared()) {
                        Appointment *app = new Appointment();
                        app->setAuxcilliaryInfo(p.name());
                        foreach(const Appointment *a, r->appointments(sm->scheduleId())) {
                            *app += *a;
                        }
                        if (app->isEmpty()) {
                            delete app;
                        } else {
                            res->addExternalAppointment(p.id(), app);
                            debugPlanShared<<res->name()<<"added:"<<app->auxcilliaryInfo()<<app;
                        }
                    }
                }
            }
        }
        doc->documentPart()->deleteLater(); // also deletes document
        emit insertSharedProject(); // do next file
    } else {
        KMessageBox::error( 0, i18n("Internal error, failed to insert file.") );
    }
}

void MainDocument::insertSharedProjectCancelled( const QString &error )
{
    debugPlanShared<<sender()<<"error="<<error;
    if ( ! error.isEmpty() ) {
        KMessageBox::error( 0, error );
    }
    MainDocument *doc = qobject_cast<MainDocument*>( sender() );
    if ( doc ) {
        doc->documentPart()->deleteLater(); // also deletes document
    }
}

bool MainDocument::insertProject( Project &project, Node *parent, Node *after )
{
    debugPlan<<&project;
    // make sure node ids in new project is unique also in old project
    QList<QString> existingIds = m_project->nodeDict().keys();
    foreach ( Node *n, project.allNodes() ) {
        QString oldid = n->id();
        n->setId( project.uniqueNodeId( existingIds ) );
        project.removeId( oldid ); // remove old id
        project.registerNodeId( n ); // register new id
    }
    MacroCommand *m = new InsertProjectCmd( project, parent==0?m_project:parent, after, kundo2_i18n( "Insert project" ) );
    if ( m->isEmpty() ) {
        delete m;
    } else {
        addCommand( m );
    }
    return true;
}

// check if calendar 'c' has children that will not be removed (normally 'Local' calendars)
bool canRemoveCalendar(const Calendar *c, const QList<Calendar*> &lst)
{
    for (Calendar *cc : c->calendars()) {
        if (!lst.contains(cc)) {
            return false;
        }
        if (!canRemoveCalendar(cc, lst)) {
            return false;
        }
    }
    return true;
}

// sort parent calendars before children
QList<Calendar*> sortedRemoveCalendars(Project &shared, const QList<Calendar*> &lst) {
    QList<Calendar*> result;
    for (Calendar *c : lst) {
        if (c->isShared() && !shared.calendar(c->id())) {
            result << c;
        }
        result += sortedRemoveCalendars(shared, c->calendars());
    }
    return result;
}

bool MainDocument::mergeResources(Project &project)
{
    debugPlanShared<<&project;
    // Just in case, remove stuff not related to resources
    foreach(Node *n,  project.childNodeIterator()) {
        debugPlanShared<<"Project not empty, delete node:"<<n<<n->name();
        NodeDeleteCmd cmd(n);
        cmd.execute();
    }
    foreach(ScheduleManager *m,  project.scheduleManagers()) {
        debugPlanShared<<"Project not empty, delete schedule:"<<m<<m->name();
        DeleteScheduleManagerCmd cmd(project, m);
        cmd.execute();
    }
    foreach(Account *a, project.accounts().accountList()) {
        debugPlanShared<<"Project not empty, delete account:"<<a<<a->name();
        RemoveAccountCmd cmd(project, a);
        cmd.execute();
    }
    // Mark all resources / groups as shared
    foreach(ResourceGroup *g, project.resourceGroups()) {
        g->setShared(true);
    }
    foreach(Resource *r, project.resourceList()) {
        r->setShared(true);
    }
    // Mark all calendars shared
    foreach(Calendar *c, project.allCalendars()) {
        c->setShared(true);
    }
    // check if any shared stuff has been removed
    QList<ResourceGroup*> removedGroups;
    QList<Resource*> removedResources;
    QList<Calendar*> removedCalendars;
    QStringList removed;
    foreach(ResourceGroup *g, m_project->resourceGroups()) {
        if (g->isShared() && !project.findResourceGroup(g->id())) {
            removedGroups << g;
            removed << i18n("Group: %1", g->name());
        }
    }
    foreach(Resource *r, m_project->resourceList()) {
        if (r->isShared() && !project.findResource(r->id())) {
            removedResources << r;
            removed << i18n("Resource: %1", r->name());
        }
    }
    removedCalendars = sortedRemoveCalendars(project, m_project->calendars());
    for (Calendar *c : qAsConst(removedCalendars)) {
        removed << i18n("Calendar: %1", c->name());
    }
    if (!removed.isEmpty()) {
        KMessageBox::ButtonCode result = KMessageBox::warningYesNoCancelList(
                    0,
                    i18n("Shared resources has been removed from the shared resources file."
                         "\nSelect how they shall be treated in this project."),
                    removed,
                    xi18nc("@title:window", "Shared resources"),
                    KStandardGuiItem::remove(),
                    KGuiItem(i18n("Convert")),
                    KGuiItem(i18n("Keep"))
                    );
        switch (result) {
        case KMessageBox::Yes: // Remove
            for (Resource *r : qAsConst(removedResources)) {
                RemoveResourceCmd cmd(r->parentGroup(), r);
                cmd.redo();
            }
            for (ResourceGroup *g : qAsConst(removedGroups)) {
                if (g->resources().isEmpty()) {
                    RemoveResourceGroupCmd cmd(m_project, g);
                    cmd.redo();
                } else {
                    // we may have put local resource(s) in this group
                    // so we need to keep it
                    g->setShared(false);
                    m_project->removeResourceGroupId(g->id());
                    g->setId(m_project->uniqueResourceGroupId());
                    m_project->insertResourceGroupId(g->id(), g);
                }
            }
            for (Calendar *c : qAsConst(removedCalendars)) {
                CalendarRemoveCmd cmd(m_project, c);
                cmd.redo();
            }
            break;
        case KMessageBox::No: // Convert
            for (Resource *r : qAsConst(removedResources)) {
                r->setShared(false);
                m_project->removeResourceId(r->id());
                r->setId(m_project->uniqueResourceId());
                m_project->insertResourceId(r->id(), r);
            }
            for (ResourceGroup *g : qAsConst(removedGroups)) {
                g->setShared(false);
                m_project->removeResourceGroupId(g->id());
                g->setId(m_project->uniqueResourceGroupId());
                m_project->insertResourceGroupId(g->id(), g);
            }
            for (Calendar *c : qAsConst(removedCalendars)) {
                c->setShared(false);
                m_project->removeCalendarId(c->id());
                c->setId(m_project->uniqueCalendarId());
                m_project->insertCalendarId(c->id(), c);
            }
            break;
        case KMessageBox::Cancel: // Keep
            break;
        default:
            break;
        }
    }
    // update values of already existing objects
    QStringList l1;
    foreach(ResourceGroup *g, project.resourceGroups()) {
        l1 << g->id();
    }
    QStringList l2;
    foreach(ResourceGroup *g, m_project->resourceGroups()) {
        l2 << g->id();
    }
    debugPlanShared<<endl<<"  This:"<<l2<<endl<<"Shared:"<<l1;
    QList<ResourceGroup*> removegroups;
    foreach(ResourceGroup *g, project.resourceGroups()) {
        ResourceGroup *group = m_project->findResourceGroup(g->id());
        if (group) {
            if (!group->isShared()) {
                // User has probably created shared resources from this project,
                // so the resources exists but are local ones.
                // Convert to shared and do not load the group from shared.
                removegroups << g;
                group->setShared(true);
                debugPlanShared<<"Set group to shared:"<<group<<group->id();
            }
            group->setName(g->name());
            group->setType(g->type());
            debugPlanShared<<"Updated group:"<<group<<group->id();
        }
    }
    QList<Resource*> removeresources;
    foreach(Resource *r, project.resourceList()) {
        Resource *resource = m_project->findResource(r->id());
        if (resource) {
            if (!resource->isShared()) {
                // User has probably created shared resources from this project,
                // so the resources exists but are local ones.
                // Convert to shared and do not load the resource from shared.
                removeresources << r;
                resource->setShared(true);
                debugPlanShared<<"Set resource to shared:"<<resource<<resource->id();
            }
            resource->setName(r->name());
            resource->setInitials(r->initials());
            resource->setEmail(r->email());
            resource->setType(r->type());
            resource->setAutoAllocate(r->autoAllocate());
            resource->setAvailableFrom(r->availableFrom());
            resource->setAvailableUntil(r->availableUntil());
            resource->setUnits(r->units());
            resource->setNormalRate(r->normalRate());
            resource->setOvertimeRate(r->overtimeRate());

            QString id = r->calendar(true) ? r->calendar(true)->id() : QString();
            resource->setCalendar(m_project->findCalendar(id));

            id = r->account() ? r->account()->name() : QString();
            resource->setAccount(m_project->accounts().findAccount(id));

            resource->setRequiredIds(r->requiredIds());

            resource->setTeamMemberIds(r->teamMemberIds());
            debugPlanShared<<"Updated resource:"<<resource<<resource->id();
        }
    }
    QList<Calendar*> removecalendars;
    foreach(Calendar *c, project.allCalendars()) {
        Calendar *calendar = m_project->findCalendar(c->id());
        if (calendar) {
            if (!calendar->isShared()) {
                // User has probably created shared resources from this project,
                // so the calendar exists but are local ones.
                // Convert to shared and do not load the resource from shared.
                removecalendars << c;
                calendar->setShared(true);
                debugPlanShared<<"Set calendar to shared:"<<calendar<<calendar->id();
            }
            *calendar = *c;
            debugPlanShared<<"Updated calendar:"<<calendar<<calendar->id();
        }
    }
    debugPlanShared<<"Remove:"<<endl<<"calendars:"<<removecalendars<<endl<<"resources:"<<removeresources<<endl<<"groups:"<<removegroups;
    while (!removecalendars.isEmpty()) {
        for (int i = 0; i < removecalendars.count(); ++i) {
            Calendar *c = removecalendars.at(i);
            if (c->childCount() == 0) {
                removecalendars.removeAt(i);
                debugPlanShared<<"Delete calendar:"<<c<<c->id();
                CalendarRemoveCmd cmd(&project, c);
                cmd.execute();
            }
        }
    }
    for (Resource *r : qAsConst(removeresources)) {
        debugPlanShared<<"Delete resource:"<<r<<r->id();
        RemoveResourceCmd cmd(r->parentGroup(), r);
        cmd.execute();
    }
    for (ResourceGroup *g : qAsConst(removegroups)) {
        debugPlanShared<<"Delete group:"<<g<<g->id();
        RemoveResourceGroupCmd cmd(&project, g);
        cmd.execute();
    }
    // insert new objects
    Q_ASSERT(project.childNodeIterator().isEmpty());
    InsertProjectCmd cmd(project, m_project, 0);
    cmd.execute();
    return true;
}

void MainDocument::insertViewListItem( View */*view*/, const ViewListItem *item, const ViewListItem *parent, int index )
{
    // FIXME callers should take care that they now get a signal even if originating from themselves
    emit viewListItemAdded(item, parent, index);
    setModified( true );
    m_viewlistModified = true;
}

void MainDocument::removeViewListItem( View */*view*/, const ViewListItem *item )
{
    // FIXME callers should take care that they now get a signal even if originating from themselves
    emit viewListItemRemoved(item);
    setModified( true );
    m_viewlistModified = true;
}

void MainDocument::setModified( bool mod )
{
    debugPlan<<mod<<m_viewlistModified;
    KoDocument::setModified( mod || m_viewlistModified ); // Must always call to activate autosave
}

void MainDocument::slotViewlistModified()
{
    if ( ! m_viewlistModified ) {
        m_viewlistModified = true;
    }
    setModified( true );  // Must always call to activate autosave
}

// called after user has created a new project in welcome view
void MainDocument::slotProjectCreated()
{
    if (url().isEmpty() && !m_project->name().isEmpty()) {
        setUrl(QUrl(m_project->name() + ".plan"));
    }
    if (m_project->scheduleManagers().isEmpty()) {
        ScheduleManager *sm = m_project->createScheduleManager();
        sm->setAllowOverbooking(false);
        sm->setSchedulingMode(ScheduleManager::AutoMode);
    }
    Calendar *week = nullptr;
    if (KPlatoSettings::generateWeek()) {
        bool always = KPlatoSettings::generateWeekChoice() == KPlatoSettings::EnumGenerateWeekChoice::Always;
        bool ifnone = KPlatoSettings::generateWeekChoice() == KPlatoSettings::EnumGenerateWeekChoice::NoneExists;
        if (always || (ifnone && m_project->calendarCount() == 0)) {
            // create a calendar
            week = new Calendar(i18nc("Base calendar name", "Base"));
            m_project->addCalendar(week);

            CalendarDay vd(CalendarDay::NonWorking);

            for (int i = Qt::Monday; i <= Qt::Sunday; ++i) {
                if (m_config.isWorkingday(i)) {
                    CalendarDay wd(CalendarDay::Working);
                    TimeInterval ti(m_config.dayStartTime(i), m_config.dayLength(i));
                    wd.addInterval(ti);
                    week->setWeekday(i, wd);
                } else {
                    week->setWeekday(i, vd);
                }
            }
        }
    }
#ifdef HAVE_KHOLIDAYS
    if (KPlatoSettings::generateHolidays()) {
        bool inweek = week != 0 && KPlatoSettings::generateHolidaysChoice() == KPlatoSettings::EnumGenerateHolidaysChoice::InWeekCalendar;
        bool subcalendar = week != 0 && KPlatoSettings::generateHolidaysChoice() == KPlatoSettings::EnumGenerateHolidaysChoice::AsSubCalendar;
        bool separate = week == 0 || KPlatoSettings::generateHolidaysChoice() == KPlatoSettings::EnumGenerateHolidaysChoice::AsSeparateCalendar;

        Calendar *holiday = nullptr;
        if (inweek) {
            holiday = week;
            week->setDefault(true);
            debugPlan<<"in week";
        } else if (subcalendar) {
            holiday = new Calendar(i18n("Holidays"));
            m_project->addCalendar(holiday, week);
            holiday->setDefault(true);
            debugPlan<<"subcalendar";
        } else if (separate) {
            holiday = new Calendar(i18n("Holidays"));
            m_project->addCalendar(holiday);
            week->setDefault(true);
            debugPlan<<"separate";
        } else {
            Q_ASSERT(false); // something wrong
        }
        debugPlan<<KPlatoSettings::region();
        if (holiday == 0) {
            warnPlan<<Q_FUNC_INFO<<"Failed to generate holidays. Bad option:"<<KPlatoSettings::generateHolidaysChoice();
            return;
        }
        holiday->setHolidayRegion(KPlatoSettings::region());
    }
#else
    week->setDefault(true);
#endif
}

// creates a "new" project from current project (new ids etc)
void MainDocument::createNewProject()
{
    setEmpty();
    clearUndoHistory();
    setModified( false );
    resetURL();
    KoDocumentInfo *info = documentInfo();
    info->resetMetaData();
    info->setProperty( "title", "" );
    setTitleModified();

    m_project->generateUniqueNodeIds();
    Duration dur = m_project->constraintEndTime() - m_project->constraintStartTime();
    m_project->setConstraintStartTime( QDateTime(QDate::currentDate(), QTime(0, 0, 0), Qt::LocalTime) );
    m_project->setConstraintEndTime( m_project->constraintStartTime() +  dur );

    while ( m_project->numScheduleManagers() > 0 ) {
        foreach ( ScheduleManager *sm, m_project->allScheduleManagers() ) {
            if ( sm->childCount() > 0 ) {
                continue;
            }
            if ( sm->expected() ) {
                sm->expected()->setDeleted( true );
                sm->setExpected( 0 );
            }
            m_project->takeScheduleManager( sm );
            delete sm;
        }
    }
    foreach ( Schedule *s, m_project->schedules() ) {
        m_project->takeSchedule( s );
        delete s;
    }
    foreach ( Node *n, m_project->allNodes() ) {
        foreach ( Schedule *s, n->schedules() ) {
            n->takeSchedule( s );
            delete s;
        }
    }
    foreach ( Resource *r, m_project->resourceList() ) {
        foreach ( Schedule *s, r->schedules() ) {
            r->takeSchedule( s );
            delete s;
        }
    }
}

void MainDocument::setIsTaskModule(bool value)
{
    m_isTaskModule = value;
}

bool MainDocument::isTaskModule() const
{
    return m_isTaskModule;
}
}  //KPlato namespace
