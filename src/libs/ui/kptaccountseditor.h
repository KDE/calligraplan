/* This file is KoDocument of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTACCOUNTSEDITOR_H
#define KPTACCOUNTSEDITOR_H

#include "planui_export.h"

#include <kptviewbase.h>
#include "kptaccountsmodel.h"

#include <kpagedialog.h>

class KoPageLayoutWidget;
class KoDocument;

class QPoint;


namespace KPlato
{

class Project;
class Account;
class AccountTreeView;

class AccountseditorConfigDialog : public KPageDialog {
    Q_OBJECT
public:
    AccountseditorConfigDialog(ViewBase *view, AccountTreeView *treeview, QWidget *parent, bool selectPrint = false);

public Q_SLOTS:
    void slotOk();

private:
    ViewBase *m_view;
    AccountTreeView *m_treeview;
    KoPageLayoutWidget *m_pagelayout;
    PrintingHeaderFooter *m_headerfooter;
};

class PLANUI_EXPORT AccountTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit AccountTreeView(QWidget *parent);

    AccountItemModel *model() const { return static_cast<AccountItemModel*>(TreeViewBase::model()); }

    Project *project() const { return model()->project(); }
    void setProject(Project *project) { model()->setProject(project); }

    Account *currentAccount() const;
    Account *selectedAccount() const;
    QList<Account*> selectedAccounts() const;
    
Q_SIGNALS:
    void currentIndexChanged(const QModelIndex&);
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);
    void selectedIndexesChanged(const QModelIndexList&);
    
protected Q_SLOTS:
    void slotHeaderContextMenuRequested(const QPoint &pos);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

protected:
    void currentChanged(const QModelIndex & current, const QModelIndex & previous) override;
    void contextMenuEvent (QContextMenuEvent * event) override;
    
};

class PLANUI_EXPORT AccountsEditor : public ViewBase
{
    Q_OBJECT
public:
    AccountsEditor(KoPart *part, KoDocument *document, QWidget *parent);
    
    void setupGui();
    Project *project() const override { return m_view->project(); }
    void draw(Project &project) override;
    void draw() override;

    AccountItemModel *model() const { return m_view->model(); }
    
    void updateReadWrite(bool readwrite) override;

    virtual Account *currentAccount() const;
    
    KoPrintJob *createPrintJob() override;

    bool loadContext(const KoXmlElement &context) override;
    void saveContext(QDomElement &context) const override;

Q_SIGNALS:
    void addAccount(KPlato::Account *account);
    void deleteAccounts(const QList<KPlato::Account*>&);
    
public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;
    void slotEditCopy() override;

protected:
    void updateActionsEnabled(bool on);
    void insertAccount(Account *account, Account *parent, int row);

protected Q_SLOTS:
    void slotOptions() override;
    
private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotHeaderContextMenuRequested(const QPoint &pos) override;

    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&);
    void slotEnableActions(bool on);

    void slotAddAccount();
    void slotAddSubAccount();
    void slotDeleteSelection();

    void slotAccountsOk();

private:
    AccountTreeView *m_view;

    QAction *actionAddAccount;
    QAction *actionAddSubAccount;
    QAction *actionDeleteSelection;

};

}  //KPlato namespace

#endif
