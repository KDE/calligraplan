/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTACCOUNTSVIEWCONFIGDIALOG_H
#define KPTACCOUNTSVIEWCONFIGDIALOG_H


#include <kpagedialog.h>
#include "ui_kptaccountsviewconfigurepanelbase.h"

class KoPageLayoutWidget;

namespace KPlato
{

class AccountsviewConfigPanel;
class AccountsTreeView;
class ViewBase;
class PrintingHeaderFooter;

class AccountsviewConfigurePanelBase : public QWidget, public Ui::AccountsviewConfigurePanelBase
{
public:
  explicit AccountsviewConfigurePanelBase(QWidget *parent) : QWidget(parent) {
    setupUi(this);
  }
};


class AccountsviewConfigDialog : public KPageDialog {
    Q_OBJECT
public:
    AccountsviewConfigDialog(ViewBase *view, AccountsTreeView *treeview, QWidget *parent, bool selectPrint = false);

public Q_SLOTS:
    void slotOk();
private Q_SLOTS:
    void enableOkButton(bool enabled);
private:
    ViewBase *m_view;
    AccountsTreeView *m_treeview;
    AccountsviewConfigPanel *m_panel;
    KoPageLayoutWidget *m_pagelayout;
    PrintingHeaderFooter *m_headerfooter;
};

class AccountsviewConfigPanel : public AccountsviewConfigurePanelBase {
    Q_OBJECT
public:
    explicit AccountsviewConfigPanel(QWidget *parent);
    
public Q_SLOTS:
    void slotChanged();

Q_SIGNALS:
    void changed(bool);
};

} //KPlato namespace

#endif // KPTACCOUNTSVIEWCONFIGDIALOG_H
