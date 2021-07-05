/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOWORK_WORKPACKAGE_H
#define KPLATOWORK_WORKPACKAGE_H

#include "kptxmlloaderobject.h"
#include "kptcommand.h"
#include "kpttask.h"

#include <KoDocument.h>

#include <QFileInfo>
#include <QProcess>
#include <QDebug>

class KoStore;

class QDomDocument;

namespace KPlato
{
    class Project;
    class Document;
    class XMLLoaderObject;
}

/// The main namespace for KPlato WorkPackage Handler
namespace KPlatoWork
{

class Part;
class WorkPackage;
class DocumentChild;

/**
 A work package consists of a Project node and one Task node
 along with scheduling information and assigned resources.
*/
class WorkPackage : public QObject
{
    Q_OBJECT
public:
    explicit WorkPackage(bool fromProjectStore);
    WorkPackage(KPlato::Project *project, bool fromProjectStore);
    ~WorkPackage() override;

    /// @return Package name
    QString name() const;

    DocumentChild *findChild(const KPlato::Document *doc) const;
    /// Called when loading a work package. Saves to Project store.
    /// Asks to save/overwrite if already there.
    /// Does nothing if opened from Projects store.
    void saveToProjects(Part *part);

    bool contains(const DocumentChild* child) const { 
        return m_childdocs.contains(const_cast<DocumentChild*>(child) );
    }
    QList<DocumentChild*> childDocs() { return m_childdocs; }
    bool addChild(Part *part, const KPlato::Document *doc);
    void removeChild(DocumentChild *child);
    
    bool contains(const KPlato::Document *doc) const;

    QString nodeId() const;

    /// Load the Plan work package document
    bool loadXML(const KoXmlElement &element, KPlato::XMLLoaderObject &status);
    /// Load the old KPlato work package file format
    bool loadKPlatoXML(const KoXmlElement &element, KPlato::XMLLoaderObject &status);

    QDomDocument saveXML();
    bool saveNativeFormat(Part *part, const QString &path);
    bool saveDocumentsToStore(KoStore *store);
    bool completeSaving(KoStore *store);

    KPlato::Node *node() const;
    KPlato::Task *task() const;
    KPlato::Project *project() const { return m_project; }

    /// Remove document @p doc
    bool removeDocument(Part *part, KPlato::Document *doc);

    /// Set the file path to this package
    void setFilePath(const QString &name) { m_filePath = name; }
    /// Return the file path to this package
    QString filePath() const { return m_filePath; }

    /// Construct file path to projects store 
    QString fileName(const Part *part) const;
    /// Remove work package file
    void removeFile();

    /// Merge data from work package @p wp
    void merge(Part *part, const WorkPackage *wp, KoStore *store);

    bool isModified() const;

    int queryClose(Part *part);

    QUrl extractFile(const KPlato::Document *doc);
    QUrl extractFile(const KPlato::Document *doc, KoStore *store);

    QString id() const;

    bool isValid() const { return m_project && node(); }

    KPlato::WorkPackageSettings &settings() { return m_settings; }
    void setSettings(const KPlato::WorkPackageSettings &settings);

    QMap<const KPlato::Document*, QUrl> newDocuments() const { return m_newdocs; }
    void removeNewDocument(const KPlato::Document *doc) { m_newdocs.remove(doc); }

    QUrl sendUrl() const { return m_sendUrl; }
    QUrl fetchUrl() const { return m_fetchUrl; }

    QString wbsCode() const { return m_wbsCode; }
    void setWbsCode(const QString &wbsCode) { m_wbsCode = wbsCode; }

Q_SIGNALS:
    void modified(bool);
    void saveWorkPackage(KPlatoWork::WorkPackage*);

public Q_SLOTS:
    void setModified(bool on) { m_modified = on; }
    void projectChanged();

protected Q_SLOTS:
    void slotChildModified(bool mod);

protected:
    /// Copy file @p filename from old store @p from, to the new store @p to
    bool copyFile(KoStore *from, KoStore *to, const QString &filename);

    bool saveToStream(QIODevice * dev);

    void openNewDocument(const KPlato::Document *doc, KoStore *store);

protected:
    KPlato::Project *m_project;
    QString m_filePath;
    bool m_fromProjectStore;
    QList<DocumentChild*> m_childdocs;
    QMap<const KPlato::Document*, QUrl> m_newdocs; /// new documents that does not exists in the project store (yet)
    QUrl m_sendUrl; /// Where to put the package. If not valid, transmit by mail
    QUrl m_fetchUrl; /// Plan will store package here
    bool m_modified;
    QString m_wbsCode;

    KPlato::WorkPackageSettings m_settings;

    KPlato::ConfigBase m_config;

};

//-----------------------------
class PackageRemoveCmd : public KPlato::NamedCommand
{
public:
    PackageRemoveCmd(Part *part, WorkPackage *value, const KUndo2MagicString &name = KUndo2MagicString());
    ~PackageRemoveCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Part *m_part;
    WorkPackage *m_value;
    bool m_mine;
};

//-----------------------------
class CopySchedulesCmd : public KPlato::NamedCommand
{
public:
    CopySchedulesCmd(const KPlato::Project &fromProject, KPlato::Project &toProject,  const KUndo2MagicString &name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    void load(const QString &doc);
    void clean(const QDomDocument &doc);
    void clearSchedules();

private:
    KPlato::Project &m_project;
    QString m_olddoc;
    QString m_newdoc;
};

//-----------------------------
class ModifyWbsCodeCmd : public KPlato::NamedCommand
{
public:
    ModifyWbsCodeCmd(WorkPackage *wp, QString wbsCode,  const KUndo2MagicString &name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    WorkPackage *m_wp;
    QString m_old;
    QString m_new;
};

}  //KPlatoWork namespace

QDebug operator<<(QDebug dbg, const KPlatoWork::WorkPackage *wp);
QDebug operator<<(QDebug dbg, const KPlatoWork::WorkPackage &wp);

#endif
