/* This file is part of the KDE project
 * Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * Copyright (C) 2004 - 2010 Dag Andersen <dag.andersen@kdemail.net>
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

    /// @return a schedule manager in the following priority:
    /// 1) The manager set with property "schedulemanager-name"
    /// 2) The baselined schedule
    /// 3) The first scheduled manager
    /// 4) The first unscheduled manager
    /// 5) nullptr
    KPlato::ScheduleManager *scheduleManager(const KoDocument *doc) const;
    KPlato::ScheduleManager *findBestScheduleManager(const KoDocument *doc) const;

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
