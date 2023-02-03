/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2012 C. Boemann <cbo@kogmbh.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTPART_H
#define KPTPART_H

#include <KoPart.h>

#include "plan_export.h"

#include <QPointer>

class KoView;
class QStackedWidget;

/// The main namespace.
namespace KPlato
{
class MainDocument;

class PLAN_EXPORT Part : public KoPart
{
    Q_OBJECT

public:
    explicit Part(QObject *parent);

    ~Part() override;

    void setDocument(KPlato::MainDocument *document);

    KoDocument *createDocument(KoPart *part) const override;

    /// reimplemented
    KoView *createViewInstance(KoDocument *document, QWidget *parent) override;

    /// reimplemented
    KoMainWindow *createMainWindow() override;

    void configure(KoMainWindow *mw) override;

    bool openProjectTemplate(const QUrl &url) override;

    bool editProject() override;

    bool openTemplate(const QUrl& url) override;

    QWidget *createWelcomeView(KoMainWindow *parent) const override;

    void addRecentURLToAllMainWindows() override;

public Q_SLOTS:
    void openTaskModule(const QUrl& url);
    void finish();

protected Q_SLOTS:
    void slotOpenTemplate(const QUrl& url);

    void slotSettingsUpdated();

    void slotLoadSharedResources(const QString &file, const QUrl &projects, bool loadProjectsAtStartup);

private:
    KPlato::MainDocument *m_document;
};

}  //KPlato namespace
#endif
