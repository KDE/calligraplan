/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
   SPDX-FileCopyrightText: 2000-2005 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2005, 2006 Sven Lüppken <sven@kde.org>
   SPDX-FileCopyrightText: 2008-2009, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "mainwindow.h"
#include "part.h"
#include "view.h"

#include "kptdocuments.h"

#include <QSplitter>
#include <QLabel>
#include <QWidget>
#include <QFileDialog>
#include <QApplication>

#include <kundo2qstack.h>

#include <assert.h>
//#include "koshellsettings.h"

#include <KoDocument.h>

#include <KLocalizedString>
#include <kmessagebox.h>
#include <kactioncollection.h>

#include <ktoolinvocation.h>
#include <KIO/StatJob>
#include <kxmlguiwindow.h>

#include <KoDocumentInfo.h>
#include <KoView.h>
#include <KoFilterManager.h>

#include "debugarea.h"

KPlatoWork_MainWindow::KPlatoWork_MainWindow()
    : KParts::MainWindow()
{
    debugPlanWork<<this;

    m_part = new KPlatoWork::Part(this, this);

    KStandardAction::quit(qApp, SLOT(quit()), actionCollection());
 
    KStandardAction::open(this, SLOT(slotFileOpen()), actionCollection());

//     KStandardAction::save(this, SLOT(slotFileSave()), actionCollection());

    QAction *a = KStandardAction::undo(m_part->undoStack(), SLOT(undo()), actionCollection());
    a->setEnabled(false);
    connect(m_part->undoStack(), &KUndo2QStack::canUndoChanged, a, &QAction::setEnabled);

    a = KStandardAction::redo(m_part->undoStack(), SLOT(redo()), actionCollection());
    a->setEnabled(false);
    connect(m_part->undoStack(), &KUndo2QStack::canRedoChanged, a, &QAction::setEnabled);
    
    setCentralWidget(m_part->widget());
    setupGUI(KXmlGuiWindow::ToolBar | KXmlGuiWindow::Keys | KXmlGuiWindow::StatusBar | KXmlGuiWindow::Save);
    createGUI(m_part);
    connect(m_part, &KPlatoWork::Part::captionChanged, this, &KPlatoWork_MainWindow::setCaption);
}


KPlatoWork_MainWindow::~KPlatoWork_MainWindow()
{
    debugPlanWork;
}


void KPlatoWork_MainWindow::setCaption(const QString &, bool modified)
{
    KParts::MainWindow::setCaption(QString(), modified);
}

bool KPlatoWork_MainWindow::openDocument(const QUrl & url)
{
    // TODO: m_part->openUrl will find out about this as well, no?
    KIO::StatJob* statJob = KIO::stat(url);
    statJob->setSide(KIO::StatJob::SourceSide);

    const bool isUrlReadable = statJob->exec();

    if (! isUrlReadable) {
        KMessageBox::error(nullptr, i18n("The file %1 does not exist.", url.url()));
//        d->recent->removeUrl(url); //remove the file from the recent-opened-file-list
//        saveRecentFiles();
        return false;
    }
    return m_part->openUrl(url);
}

QString KPlatoWork_MainWindow::configFile() const
{
  //return readConfigFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation "koshell/koshell_shell.rc"));
  return QString(); // use UI standards only for now
}

//called from slotFileSave(), slotFileSaveAs(), queryClose(), slotEmailFile()
bool KPlatoWork_MainWindow::saveDocument(bool saveas, bool silent)
{
    debugPlanWork<<saveas<<silent;
    KPlatoWork::Part *doc = rootDocument();
    if (doc == nullptr) {
        return true;
    }
    return doc->saveWorkPackages(silent);
}


bool KPlatoWork_MainWindow::queryClose()
{
    KPlatoWork::Part *part = rootDocument();
    if (part == nullptr) {
        return true;
    }
    return part->queryClose();
}

void KPlatoWork_MainWindow::slotFileClose()
{
    if (queryClose()) {
    }
}

void KPlatoWork_MainWindow::slotFileSave()
{
    saveDocument();
}

void KPlatoWork_MainWindow::slotFileOpen()
{
    const QUrl file = QFileDialog::getOpenFileUrl(nullptr, QString(), QUrl(), QStringLiteral("*.planwork"));
    if (! file.isEmpty()) {
        openDocument(file);
    }
}
