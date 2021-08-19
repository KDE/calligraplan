/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTMAINDOCUMENT_H
#define KPTMAINDOCUMENT_H

#include "plan_export.h"

#include "kptpackage.h"
#include "kpttask.h"
#include "kptconfig.h"
#include "kptwbsdefinition.h"
#include "kptxmlloaderobject.h"

#include <MimeTypes.h>
#include "KoDocument.h"

#include <QFileInfo>
#include <QDomDocument>



class KDirWatch;

/// The main namespace.
namespace KPlato
{

class DocumentChild;
class Project;
class Context;
class SchedulerPlugin;
class ViewListItem;
class View;

class Package;

class PLAN_EXPORT MainDocument : public KoDocument
{
    Q_OBJECT

public:
    explicit MainDocument(KoPart *part, bool loadSchedulerPlugins = true);
    ~MainDocument() override;

    void initEmpty() override;

    /// reimplemented from KoDocument
    QByteArray nativeFormatMimeType() const override { return PLAN_MIME_TYPE; }
    /// reimplemented from KoDocument
    QByteArray nativeOasisMimeType() const override { return ""; }
    /// reimplemented from KoDocument
    QStringList extraNativeMimeTypes() const override
    {
        return QStringList() << PLAN_MIME_TYPE;
    }

    void setReadWrite(bool rw) override;
    void configChanged();

    void setProject(Project *project);
    Project *project() const override { return m_project; }
    Project &getProject() { return *m_project; }
    const Project &getProject() const { return * m_project; }
    QString projectName() const override { return m_project->name(); }

    /**
     * Return the set of SupportedSpecialFormats that the kplato wants to
     * offer in the "Save" file dialog.
     * Note: SaveEncrypted is not supported.
     */
    int supportedSpecialFormats() const override { return SaveAsDirectoryStore; }

    // The load and save functions. Look in the file kplato.dtd for info
    bool loadXML(const KoXmlDocument &document, KoStore *store) override;
    QDomDocument saveXML() override;
    /// Save a workpackage file containing @p node with schedule identity @p id, owned by @p resource
    QDomDocument saveWorkPackageXML(const Node *node, long id, Resource *resource = nullptr);

    bool saveOdf(SavingContext &/*documentContext */) override { return false; }
    bool loadOdf(KoOdfReadStore & odfStore) override;

    Config &config() { return m_config; }
    Context *context() const { return m_context; }

    WBSDefinition &wbsDefinition() { return m_project->wbsDefinition(); }

    const XMLLoaderObject &xmlLoader() const { return m_xmlLoader; }

    DocumentChild *createChild(KoDocument *doc, const QRect &geometry = QRect());

    bool saveWorkPackageToStream(QIODevice * dev, const Node *node, long id, Resource *resource = nullptr);
    bool saveWorkPackageFormat(const QString &file, const Node *node, long id, Resource *resource = nullptr);
    bool saveWorkPackageUrl(const QUrl & _url, const Node *node, long id, Resource *resource = nullptr  );

    /// Load the workpackage from @p url into @p project. Return true if successful, else false.
    bool loadWorkPackage(Project &project, const QUrl &url);
    Package *loadWorkPackageXML(Project& project, QIODevice*, const KoXmlDocument& document, const QUrl& url);
    QMap<QDateTime, Package*> workPackages() const { return m_workpackages; }
    void clearWorkPackages() {
        qDeleteAll(m_workpackages);
        m_workpackages.clear();
        m_checkingForWorkPackages = false;
    }

    void insertFile(const QUrl &url, Node *parent, Node *after = nullptr);
    bool insertProject(Project &project, Node *parent, Node *after);
    bool mergeResources(Project &project);

    bool extractFiles(KoStore *store, Package *package);
    bool extractFile(KoStore *store, Package *package, const Document *doc);

    void registerView(View *view);

    /// Create a new project from this project
    /// Generates new project id and task ids
    /// Keeps resource- and calendar ids
    void createNewProject();

    bool isTaskModule() const;

    /**
     * Returns true during loading (openUrl can be asynchronous)
     */
    bool isLoading() const override;

    /// Insert resource assignments from @p project
    Q_INVOKABLE void insertSharedResourceAssignments(const KPlato::Project *project);

    QMap<QString, KPlato::SchedulerPlugin*> schedulerPlugins() const override;
    void setSchedulerPlugins(QMap<QString, KPlato::SchedulerPlugin*> &plugins);

    bool openLocalFile(const QString &localFileName);

    using KoDocument::setModified;
public Q_SLOTS:
    void setModified(bool mod) override;

    /// Inserts an item into all other views than @p view
    void insertViewListItem(KPlato::View *view, const KPlato::ViewListItem *item, const KPlato::ViewListItem *parent, int index);
    /// Removes the view list item from all other views than @p view
    void removeViewListItem(KPlato::View *view, const KPlato::ViewListItem *item);
    /// View selector has been modified
    void slotViewlistModified();
    /// Check for workpackages
    /// If @p keep is true, packages that has been refused will not be checked for again
    void checkForWorkPackages(bool keep);
    /// Remove @p package
    void terminateWorkPackage(const KPlato::Package *package);

    void setLoadingTemplate(bool);
    void setLoadingSharedResourcesTemplate(bool);
    void setSavingTemplate(bool);

    void setSkipSharedResourcesAndProjects(bool skip);

    void insertResourcesFile(const QUrl &url);
    void slotProjectCreated();

    /// Prepare for insertion of resource assignments of shared resources from the project(s) in @p urls
    void insertSharedProjects(const QList<QUrl> &urls);

    /// Prepare for insertion of resource assignments of shared resources from the project(s) in @p url
    void insertSharedProjects(const QUrl &url);

    void slotInsertSharedProject();
    void insertSharedProjectCompleted();
    void insertSharedProjectCancelled(const QString&);

    /// Clear resource assignments of shared resources
    void clearResourceAssignments();
    /// Load resource assignments of shared resources from the project(s) in @p url
    void loadResourceAssignments(QUrl url);

    void setIsTaskModule(bool value);

    void autoCheckForWorkPackages();

Q_SIGNALS:
    void changed();
    void workPackageLoaded();
    void viewlistModified(bool);
    void viewListItemAdded(const KPlato::ViewListItem *item, const KPlato::ViewListItem *parent, int index);
    void viewListItemRemoved(const KPlato::ViewListItem *item);

    void insertSharedProject();

protected:
    /// Load kplato specific files
    bool completeLoading(KoStore* store) override;
    /// Save kplato specific files
    bool completeSaving(KoStore* store) override;

    // used by insert file
    struct InsertFileInfo {
        QUrl url;
        Node *parent;
        Node *after;
    } m_insertFileInfo;


protected Q_SLOTS:
    void slotViewDestroyed();
    void addSchedulerPlugin(const QString&, KPlato::SchedulerPlugin *plugin);

    void checkForWorkPackage();

    void insertFileCompleted();
    void insertResourcesFileCompleted();
    void insertFileCancelled(const QString&);

    void slotNodeChanged(KPlato::Node*, int);
    void slotScheduleManagerChanged(KPlato::ScheduleManager *sm, int property);
    void setCalculationNeeded();
    void slotCalculationFinished(KPlato::Project *project, KPlato::ScheduleManager *sm);
    void slotStartCalculation();

    void setTaskModulesWatch();
    void taskModuleDirChanged();

private:
    bool loadAndParse(KoStore* store, const QString& filename, KoXmlDocument& doc);

    void loadSchedulerPlugins();

private:
    Project *m_project;
    QWidget* m_parentWidget;

    Config m_config;
    Context *m_context;

    XMLLoaderObject m_xmlLoader;
    bool m_loadingTemplate;
    bool m_loadingSharedResourcesTemplate;
    bool m_savingTemplate;

    QMap<QString, SchedulerPlugin*> m_schedulerPlugins;
    QMap<QDateTime, Package*> m_workpackages;
    QFileInfoList m_infoList;
    QList<QUrl> m_skipUrls;
    QMap<QDateTime, Project*> m_mergedPackages;

    QDomDocument m_reports;

    bool m_viewlistModified;
    bool m_checkingForWorkPackages;

    QList<QPointer<View> > m_views;

    bool m_loadingSharedProject;
    QList<QUrl> m_sharedProjectsFiles;
    bool m_skipSharedProjects;

    bool m_isLoading;

    bool m_isTaskModule;

    KUndo2Command* m_calculationCommand;
    ScheduleManager* m_currentCalculationManager;
    ScheduleManager* m_nextCalculationManager;

    KDirWatch *m_taskModulesWatch;
};


}  //KPlato namespace

#endif
