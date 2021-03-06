/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_MAINDOCUMENT_H
#define PLANPORTFOLIO_MAINDOCUMENT_H

#include "planportfolio_export.h"

#include <MimeTypes.h>
#include "KoDocument.h"

#include <QDomDocument>


#define PLANPORTFOLIO_FILE_SYNTAX_VERSION "1.0.0"


namespace KPlato {
    class ScheduleManager;
}

class PLANPORTFOLIO_EXPORT MainDocument : public KoDocument
{
    Q_OBJECT

public:
    explicit MainDocument(KoPart *part);
    ~MainDocument() override;

    void initEmpty() override;

    /// reimplemented from KoDocument
    QByteArray nativeFormatMimeType() const override { return PLANPORTFOLIO_MIME_TYPE; }
    /// reimplemented from KoDocument
    QByteArray nativeOasisMimeType() const override { return QByteArray(); }
    /// reimplemented from KoDocument
    QStringList extraNativeMimeTypes() const override { return QStringList(); }

    void setReadWrite(bool rw) override;
    void configChanged();

    void setDocumentProperty(KoDocument *doc, const char *name, const QVariant &value);
    bool addDocument(KoDocument *doc);
    void removeDocument(KoDocument *doc);
    QList<KoDocument*> documents() const;

    int supportedSpecialFormats() const override { return SaveAsDirectoryStore; }

    bool loadXML(const KoXmlDocument &document, KoStore *store) override;
    QDomDocument saveXML() override;

    bool saveOdf(SavingContext &/*documentContext */) override { return false; }
    bool loadOdf(KoOdfReadStore & odfStore) override;

    /**
     * Returns true during loading (openUrl can be asynchronous)
     */
    bool isLoading() const override;

    void emitChanged();
    void emitDocumentChanged(KoDocument *doc);

    /// @return a schedule manager in the following priority:
    /// 1) The manager set with property "schedulemanager-name"
    /// 2) The baselined schedule
    /// 3) The first scheduled manager
    /// 4) The first unscheduled manager
    /// 5) nullptr
    KPlato::ScheduleManager *scheduleManager(const KoDocument *doc) const;
    KPlato::ScheduleManager *findBestScheduleManager(const KoDocument *doc) const;

    QMap<QString, KPlato::SchedulerPlugin*> schedulerPlugins() const override;
    KPlato::SchedulerPlugin *schedulerPlugin(const QString &key) const;

    using KoDocument::setModified;
public Q_SLOTS:
    void setModified(bool mod) override;

Q_SIGNALS:
    void changed();
    void documentChanged(KoDocument *doc, int index);
    void projectChanged(KoDocument *doc);
    void documentAboutToBeInserted(int row);
    void documentInserted();
    void documentAboutToBeRemoved(int row);
    void documentRemoved();

protected Q_SLOTS:
    void slotProjectChanged();
    void slotProjectDocumentLoaded();

protected:
    bool completeLoading(KoStore* store) override;
    /// Save kplato specific files
    bool completeSaving(KoStore* store) override;

private:
    QList<KoDocument*> m_documents;
};

#endif
