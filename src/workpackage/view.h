/* This file is part of the KDE project
  SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
  SPDX-FileCopyrightText: 2002-2009, 2011 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOWORK_VIEW
#define KPLATOWORK_VIEW

#include <KoView.h>

#include <QMap>
#include <QStackedWidget>

class QPrinter;
class QPrintDialog;
class QMenu;
class QActionGroup;

class KActionCollection;

class KoStore;
class KoView;

class KPlatoWork_MainWindow;

namespace KPlato
{

class Document;
class ScheduleManager;

class Node;
class Schedule;

}

/// Then namespace for the KPlato work package handler
namespace KPlatoWork
{

class Part;
class View;
class AbstractView;
class TaskWorkPackageView;
class TaskWPGanttView;

//-------------
class View : public QStackedWidget
{
    Q_OBJECT

public:
    explicit View(Part* part, QWidget *parent, KActionCollection *collection);
    ~View() override;

    Part *part() const;

    virtual void setupPrinter(QPrinter &printer, QPrintDialog &printDialog);
    virtual void print(QPrinter &printer, QPrintDialog &printDialog);

    QMenu *popupMenu(const QString& name, const QPoint &pos);

//    virtual ViewAdaptor* dbusObject();

    virtual bool loadContext();
    virtual void saveContext() const;

    KPlato::ScheduleManager *currentScheduleManager() const;
    long currentScheduleId() const;
    
    TaskWorkPackageView *createTaskWorkPackageView();
    TaskWPGanttView *createGanttView();

    KPlatoWork_MainWindow *kplatoWorkMainWindow() const;

    KPlato::Node *currentNode() const;
    KPlato::Document *currentDocument() const;

Q_SIGNALS:
    void currentScheduleManagerChanged(KPlato::ScheduleManager *sm);
    void openInternalDocument(KoStore *);
    void sigUpdateReadWrite(bool);

    void viewDocument(KPlato::Document *doc);

public Q_SLOTS:
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();

    void slotConfigure();

    void slotPopupMenu(const QString& menuname, const QPoint &pos);

    void slotTaskProgress();
    void slotTaskCompletion();

protected Q_SLOTS:
    void slotCurrentChanged(int index);
    void slotProgressChanged(int value);

    void slotEditDocument();
    void slotEditDocument(KPlato::Document *doc);
    void slotViewDocument();
    void slotRemoveDocument();

    void slotSendPackage();
    void slotTaskDescription();
    void slotRemoveCurrentPackage();
    void slotRemoveSelectedPackages();
    void slotSelectionChanged();

    void slotViewList();
    void slotViewGantt();

protected:
    virtual void updateReadWrite(bool readwrite);

    QAction *addScheduleAction(KPlato::Schedule *sch);
    void setLabel();
    AbstractView *currentView() const;

private:
    void createViews();
    
private:
    Part *m_part;

    QActionGroup *m_scheduleActionGroup;
    QMap<QAction*, KPlato::Schedule*> m_scheduleActions;
    KPlato::ScheduleManager *m_manager;
    
    bool m_readWrite;
    
    // ------ Edit
    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
    QAction *actionRemoveSelectedPackages;

    // ------ View
    QAction *actionViewList;
    QAction *actionViewGantt;

    // ------ Settings
    QAction *actionConfigure;

    QAction *actionViewDocument;
    QAction *actionEditDocument;
    QAction *actionRemoveDocument;

    QAction *actionSendPackage;
    QAction *actionPackageSettings;
    QAction *actionTaskCompletion;
    QAction *actionViewDescription;
    QAction *actionRemoveCurrentPackage;
};

} //KplatoWork namespace

#endif
