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
#include <KoXmlReader.h>

#include <QDomDocument>


static const QLatin1String PLANPORTFOLIO_FILE_SYNTAX_VERSION("1.0.0");


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
    QByteArray nativeFormatMimeType() const override { return PLANPORTFOLIO_MIME_TYPE.latin1(); }
    /// reimplemented from KoDocument
    QByteArray nativeOasisMimeType() const override { return QByteArray(); }
    /// reimplemented from KoDocument
    QStringList extraNativeMimeTypes() const override { return QStringList(); }

    void setReadWrite(bool rw) override;
    void configChanged();

    /// Set property @p name in document p doc to @p value
    /// If the property is changed, return true, else false.
    bool setDocumentProperty(KoDocument *doc, const char *name, const QVariant &value);
    bool addDocument(KoDocument *doc);
    void removeDocument(KoDocument *doc);
    QList<KoDocument*> documents() const;

    bool loadXML(const KoXmlDocument &document, KoStore *store) override;
    QDomDocument saveXML() override;

    bool saveOdf(SavingContext &/*documentContext */) override { return false; }
    bool loadOdf(KoOdfReadStore & odfStore) override;

    /**
     * @brief Loads a document from a store.
     *
     * The url() is not changed
     *
     * @param store The store to load from
     * @param url An internal url, like tar:/1/2
     */
    virtual bool loadFromStore(KoStore *store, const QString& url) override;

    /**
     * @brief Saves a sub-document to a store.
     *
     * The external url() is not changed
     *
     * @param store The store to save to
     * @param doc The document to save
     * @param path An internal url to load from
     */
    bool saveDocumentToStore(KoStore *_store, KoDocument *doc);

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

    bool isChildrenModified() const;

    bool isModified() const override;

    const KoXmlDocument& xmlDocument() const;

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
    void documentModified();
    void saveSettings(QDomDocument &xml);

protected Q_SLOTS:
    void slotProjectChanged();
    void slotProjectDocumentLoaded();
    void slotProjectDocumentCanceled();
    void slotDocumentModified(bool mod);

protected:
    bool completeLoading(KoStore* store) override;
    /// Save kplato specific files
    bool completeSaving(KoStore* store) override;

    bool isEqual(const char *s1, const char *s2) const;

private:
    QList<KoDocument*> m_documents;
    KoXmlDocument m_xmlDocument;
};

#endif
