/* This file is part of the KDE project
   Copyright (C) 2012 C. Boemann <cbo@kogmbh.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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

    void showStartUpWidget(KoMainWindow *parent) override;

    void configure(KoMainWindow *mw) override;

    bool openProjectTemplate(const QUrl &url) override;

    bool editProject() override;

    QString recentFilesGroupName() const override;

    bool openTemplate(const QUrl& url) override;

public Q_SLOTS:
    void openTaskModule(const QUrl& url);
    void finish();

protected Q_SLOTS:
    void slotOpenTemplate(const QUrl& url);

    void slotSettingsUpdated();

    void slotLoadSharedResources(const QString &file, const QUrl &projects, bool loadProjectsAtStartup);

private:
    KPlato::MainDocument *m_document;
    QPointer<QStackedWidget> startUpWidget;
    bool m_toolbarVisible;
};

}  //KPlato namespace
#endif
