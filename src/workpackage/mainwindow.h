/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
   SPDX-FileCopyrightText: 2000-2005 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2005 Sven L �ppken <sven@kde.org>
   SPDX-FileCopyrightText: 2008-2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KPLATOWORK_MAINWINDOW_H
#define KPLATOWORK_MAINWINDOW_H

#include "planwork_export.h"

#include <QMap>
#include <QToolButton>
#include <QLabel>

#include <kparts/mainwindow.h>

namespace KParts {
}
namespace KPlatoWork {
    class Part;
}
namespace KPlato {
    class Document;
}


/////// class KPlatoWork_MainWindow ////////

class PLANWORK_EXPORT KPlatoWork_MainWindow : public KParts::MainWindow
{
  Q_OBJECT

public:
    explicit KPlatoWork_MainWindow();
    ~KPlatoWork_MainWindow() override;

    KPlatoWork::Part *rootDocument() const { return m_part; }
    bool openDocument(const QUrl & url);

    virtual QString configFile() const;

     void editDocument(KPlatoWork::Part *part, const KPlato::Document *doc);

//     bool isEditing() const { return m_editing; }
//     bool isModified() const;

Q_SIGNALS:
    void undo();
    void redo();

public Q_SLOTS:
    virtual void slotFileClose();
    void setCaption(const QString &text, bool modified = false) override;

protected Q_SLOTS:
    bool queryClose() override;

    virtual void slotFileOpen();
    /**
     *  Saves all workpackages
     */
    virtual void slotFileSave();

protected:
     virtual bool saveDocument(bool saveas = false, bool silent = false);

private:
    KPlatoWork::Part *m_part;
};


#endif // KPLATOWORK_MAINWINDOW_H

