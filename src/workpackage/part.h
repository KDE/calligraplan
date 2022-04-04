/* This file is part of the KDE project
  SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
  SPDX-FileCopyrightText: 2004-2009 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
  SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
  SPDX-FileCopyrightText: 2007-2009 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOWORK_PART_H
#define KPLATOWORK_PART_H

#include "planwork_export.h"

#include <kptxmlloaderobject.h>
#include <kptnode.h>

#include <KoDocument.h>

#include <QFileInfo>
#include <QProcess>

#include <kservice.h>
#include <kparts/readwritepart.h>

class KUndo2QStack;

class KoStore;

class KProcess;

class QFileSystemWatcher;

namespace KPlato
{
    class Document;
    class MacroCommand;
}

/// The main namespace for KPlato WorkPackage Handler
namespace KPlatoWork
{

class Part;
class WorkPackage;
class View;

/**
 * DocumentChild stores info about documents opened for editing.
 * Editors can be KParts, Calligra or Other.
 */
class DocumentChild : public QObject
{
    Q_OBJECT
public:
    // The type of document this child handles
    enum DocType { Type_Unknown = 0, Type_Calligra, Type_KParts, Type_Other };

    explicit DocumentChild(WorkPackage *parent);
    
    ~DocumentChild() override;
    
    WorkPackage *parentPackage() const;
    const KPlato::Document *doc() const { return m_doc; }
    /// Set document, return true if ok, false if failure
    bool setDoc(const KPlato::Document *doc);
    /// Open @p doc from @p store
    bool openDoc(const KPlato::Document *doc, KoStore *store);
    /// Open document for editing, return true if ok, false if failure
    bool editDoc();
    bool isOpen() const { return m_process != nullptr; }
    bool isModified() const;
    bool isFileModified() const;
    
    QString fileName() const { return m_fileinfo.fileName(); }
    QString filePath() const { return m_fileinfo.canonicalFilePath(); }
    void setFileInfo(const QUrl &url);
    const QFileInfo &fileInfo() const { return m_fileinfo; }

    QUrl url() const { return QUrl::fromLocalFile(filePath()); }
    KParts::ReadWritePart *editor() const { return m_editor; }
    bool startProcess(KService::Ptr service, const QUrl &url = QUrl());
    int type() const { return m_type; }
    void setType(int type) { m_type = type; }
    
    bool saveToStore(KoStore *store);

Q_SIGNALS:
    void modified(bool);
    void fileModified(bool);
    
public Q_SLOTS:
    void setModified(bool mod);
    
protected Q_SLOTS:
    void slotEditFinished(int,  QProcess::ExitStatus);
    void slotEditError(QProcess::ProcessError status);
    
    void slotDirty(const QString &file);

    void slotUpdateModified();
    
protected:
    const KPlato::Document *m_doc;
    int m_type;
    bool m_copy;
    KProcess *m_process; // Used if m_type == Type_Other;
    KParts::ReadWritePart *m_editor; // 0 if m_type == Type_Other
    QFileInfo m_fileinfo;
    bool m_editormodified;
    bool m_filemodified;
    QFileSystemWatcher *m_fileSystemWatcher;
};

/**
 This part handles work packages.
 A work package file consists of a Project node and one Task node
 along with scheduling information and assigned resources.
*/

class PLANWORK_EXPORT Part : public KParts::ReadWritePart
{
    Q_OBJECT

public:
    /// Create a part with a view, the view will have @p parentWidget as parent
    explicit Part(QWidget *parentWidget, QObject *parent, const QVariantList & /*args*/ = QVariantList());
    /// Create a view less part, with no gui messageboxes,
    /// unless you setNoGui(false)
    explicit Part(QObject *parent = nullptr);
    ~Part() override;

    int docType(const KPlato::Document *doc) const;

    bool loadWorkPackages();
    bool loadWorkPackage(const QString &fileName);
    virtual bool loadXML(const KoXmlDocument &document, KoStore *store);
    virtual QDomDocument saveXML();
    
    bool saveAs(const QUrl &url) override;
    /// Check if we have documents open for editing before saving
    virtual bool completeSaving(KoStore* store);

    /// Extract document file from the store to disk
    QUrl extractFile(const KPlato::Document *doc);
    
    //Config &config() { return m_config; }
    
    /// Open Calligra document for editing
//     DocumentChild *openCalligraDocument(KMimeType::Ptr mimetype, const KPlato::Document *doc);
    /// Open KParts document for editing
//     DocumentChild *openKPartsDocument(KService::Ptr service, const KPlato::Document *doc);
    /// Open document for editing, return true if ok, false if failure
    bool editWorkpackageDocument(const KPlato::Document *doc);
    /// Open document for editing, return true if ok, false if failure
    bool editOtherDocument(const KPlato::Document *doc);
    /// Remove the document @p doc from its workpackage
    bool removeDocument(KPlato::Document *doc);
    /// Remove the child document
//    void removeChildDocument(DocumentChild *child);
    /// Find the child that handles document @p doc
    DocumentChild *findChild(const KPlato::Document *doc) const;
    /// Add @p child document to work package @p wp
//    void addChild(WorkPackage *wp, DocumentChild *child);

    QMap<QString, WorkPackage*> workPackages() const { return m_packageMap; }
    /// Number of workpackages
    int workPackageCount() const { return m_packageMap.count(); }
    /// Work package at index
    WorkPackage *workPackage(int index) const {
        const QList<WorkPackage*> &lst = m_packageMap.values();
        return lst.value(index);
    }
    /// Work package containing node
    WorkPackage *workPackage(KPlato::Node *node) const { 
        return m_packageMap.value(node->projectNode()->id() + node->id());
    }
    int indexOf(WorkPackage *package) const {
        const QList<WorkPackage*> &lst = m_packageMap.values();
        return lst.indexOf(package);
    }
    void addWorkPackage(WorkPackage *wp);
    void removeWorkPackage(WorkPackage *wp);
    void removeWorkPackage(KPlato::Node *node, KPlato::MacroCommand *m = nullptr);
    void removeWorkPackages(const QList<KPlato::Node*> &nodes);

    /// Find the work package that handles document @p doc
    WorkPackage *findWorkPackage(const KPlato::Document *doc) const;
    /// Find the work package that handles document child @p child
    WorkPackage *findWorkPackage(const DocumentChild *child) const;
    /// Find the work package that handles @p node
    WorkPackage *findWorkPackage(const KPlato::Node *node) const;

    /// Save all work packages
    bool saveWorkPackages(bool silent);

    KPlato::Node *node() const;
    
    bool queryClose() override;

    bool openFile() override;
    bool saveFile() override;

    KUndo2QStack *undoStack() const { return m_undostack; }
    int commandIndex() const { return m_undostack->index(); }

    void setNoGui(bool nogui);

public Q_SLOTS:
    /**
     * Called by the undo stack when the document is saved or all changes has been undone
     * @param clean if the document's undo stack is clean or not
     */
    virtual void setDocumentClean(bool clean);

    void setModified(bool mod) override;
    void saveModifiedWorkPackages();
    void saveWorkPackage(KPlatoWork::WorkPackage *wp);
    void addCommand(KUndo2Command *cmd);

    void viewWorkpackageDocument(KPlato::Document *doc);

Q_SIGNALS:
    void changed();
    void workPackageAdded(KPlatoWork::WorkPackage *package, int index);
    void workPackageRemoved(KPlatoWork::WorkPackage *wp, int index);
    void captionChanged(const QString&, bool);

protected:
    /// Load the old kplato format
    bool loadKPlatoXML(const KoXmlDocument &document, KoStore *store);
    /// Adds work package @p wp to the list of workpackages.
    /// If it already exists, the user is asked if it shall be merged with the existing one.
    bool setWorkPackage(WorkPackage *wp, KoStore *store = nullptr);
    bool completeLoading(KoStore *store);

    bool loadAndParse(KoStore* store, const QString& filename, KoXmlDocument& doc);
    bool loadNativeFormatFromStore(const QString& file);
    bool loadNativeFormatFromStoreInternal(KoStore * store);
    
    bool viewDocument(const QUrl &filename);

private:
    View *m_view;
    KPlato::XMLLoaderObject m_xmlLoader;
    //Config m_config;
    
    QMap<QString, WorkPackage*> m_packageMap;

    bool m_modified;
    bool m_loadingFromProjectStore;

    KUndo2QStack *m_undostack;
    bool m_nogui;
};


}  //KPlatoWork namespace

#endif
