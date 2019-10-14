/* This file is part of the KDE project
 * Copyright (C) 2007 Dag Andersen <danders@get2net>
 * Copyright (C) 2011, 2012 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2019 Dag Andersen <danders@get2net>
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

// clazy:excludeall=qstring-arg
#include "kptaccountseditor.h"

#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptaccount.h"
#include "kptdatetime.h"
#include "Help.h"
#include "kptdebug.h"

#include <KoDocument.h>
#include <KoPageLayoutWidget.h>
#include <KoIcon.h>

#include <QList>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QHeaderView>

#include <KLocalizedString>
#include <kactioncollection.h>


namespace KPlato
{


AccountseditorConfigDialog::AccountseditorConfigDialog( ViewBase *view, AccountTreeView *treeview, QWidget *p, bool selectPrint)
    : KPageDialog(p),
    m_view( view ),
    m_treeview( treeview )
{
    setWindowTitle( i18n("Settings") );

    QTabWidget *tab = new QTabWidget();

    QWidget *w = ViewBase::createPageLayoutWidget( view );
    tab->addTab( w, w->windowTitle() );
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT( m_pagelayout );

    m_headerfooter = ViewBase::createHeaderFooterWidget( view );
    m_headerfooter->setOptions( view->printingOptions() );
    tab->addTab( m_headerfooter, m_headerfooter->windowTitle() );

    KPageWidgetItem *page = addPage( tab, i18n( "Printing" ) );
    page->setHeader( i18n( "Printing Options" ) );
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect( this, &QDialog::accepted, this, &AccountseditorConfigDialog::slotOk);
}

void AccountseditorConfigDialog::slotOk()
{
    debugPlan;
    m_view->setPageLayout( m_pagelayout->pageLayout() );
    m_view->setPrintingOptions( m_headerfooter->options() );
}


//--------------------
AccountTreeView::AccountTreeView( QWidget *parent )
    : TreeViewBase( parent )
{
    setDragPixmap(koIcon("account").pixmap(32));
    header()->setContextMenuPolicy( Qt::CustomContextMenu );
    setModel( new AccountItemModel( this ) );
    setSelectionModel( new QItemSelectionModel( model() ) );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
//     setSelectionBehavior( QAbstractItemView::SelectRows );
    
    connect( header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderContextMenuRequested(QPoint)) );
}

void AccountTreeView::slotHeaderContextMenuRequested( const QPoint &pos )
{
    debugPlan<<header()->logicalIndexAt(pos)<<" at"<<pos;
}

void AccountTreeView::contextMenuEvent ( QContextMenuEvent *event )
{
    debugPlan;
    emit contextMenuRequested( indexAt(event->pos()), event->globalPos() );
}

void AccountTreeView::selectionChanged( const QItemSelection &sel, const QItemSelection &desel )
{
    debugPlan<<sel.indexes().count();
    foreach( const QModelIndex &i, selectionModel()->selectedIndexes() ) {
        debugPlan<<i.row()<<","<<i.column();
    }
    QTreeView::selectionChanged( sel, desel );
    emit selectionChanged( selectionModel()->selectedIndexes() );
}

void AccountTreeView::currentChanged( const QModelIndex & current, const QModelIndex & previous )
{
    debugPlan;
    QTreeView::currentChanged( current, previous );
    emit currentChanged( current );
    // possible bug in qt: in QAbstractItemView::SingleSelection you can select multiple items/rows
    selectionModel()->select( current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
}

Account *AccountTreeView::currentAccount() const
{
    return model()->account( currentIndex() );
}

Account *AccountTreeView::selectedAccount() const
{
    QModelIndexList lst = selectionModel()->selectedRows();
    if ( lst.count() == 1 ) {
        return model()->account( lst.first() );
    }
    return 0;
}

QList<Account*> AccountTreeView::selectedAccounts() const
{
    QList<Account*> lst;
    foreach ( const QModelIndex &i, selectionModel()->selectedRows() ) {
        Account *a = model()->account( i );
        if ( a ) {
            lst << a;
        }
    }
    return lst;
}


//-----------------------------------
AccountsEditor::AccountsEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    if (doc && doc->isReadWrite()) {
        setXMLFile("AccountsEditorUi.rc");
    } else {
        setXMLFile("AccountsEditorUi_readonly.rc");
    }

    setupGui();

    QVBoxLayout * l = new QVBoxLayout( this );
    l->setMargin( 0 );
    m_view = new AccountTreeView( this );
    connect(this, &ViewBase::expandAll, m_view, &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &TreeViewBase::slotCollapse);

    l->addWidget( m_view );
    m_view->setEditTriggers( m_view->editTriggers() | QAbstractItemView::EditKeyPressed );

    m_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_view->setDropIndicatorShown( false );
    m_view->setDragEnabled ( true );
    m_view->setAcceptDrops( false );
    m_view->setAcceptDropsOnView( false );

    connect( model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand );
    
    connect( m_view, SIGNAL(currentChanged(QModelIndex)), this, SLOT(slotCurrentChanged(QModelIndex)) );

    connect( m_view, SIGNAL(selectionChanged(QModelIndexList)), this, SLOT(slotSelectionChanged(QModelIndexList)) );
    
    connect( m_view, SIGNAL(contextMenuRequested(QModelIndex,QPoint)), this, SLOT(slotContextMenuRequested(QModelIndex,QPoint)) );
    connect( m_view, SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)) );

    Help::add(this,
              xi18nc("@info:whatsthis",
                     "<title>Cost Breakdown Structure Editor</title>"
                     "<para>"
                     "The Cost Breakdown Structure (CBS) consists of accounts"
                     " organized into a tree structure."
                     " Accounts can be tied to tasks or resources."
                     " Usually there will be two top accounts, one for aggregating costs from tasks"
                     " and one for aggregating costs from resources."
                     "</para><para>"
                     "This view supports printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", Help::page("Cost_Breakdown_Structure_Editor")));
}

void AccountsEditor::updateReadWrite( bool readwrite )
{
    m_view->setReadWrite( readwrite );
}

void AccountsEditor::draw( Project &project )
{
    m_view->setProject( &project );
}

void AccountsEditor::draw()
{
}

void AccountsEditor::setGuiActive( bool activate )
{
    debugPlan<<activate;
    updateActionsEnabled( true );
    ViewBase::setGuiActive( activate );
    if ( activate ) {
        if ( !m_view->currentIndex().isValid() ) {
            m_view->selectionModel()->setCurrentIndex(m_view->model()->index( 0, 0 ), QItemSelectionModel::NoUpdate);
        }
        slotSelectionChanged( m_view->selectionModel()->selectedRows() );
    }
}

void AccountsEditor::slotContextMenuRequested( const QModelIndex &index, const QPoint& pos )
{
    debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    m_view->setContextMenuIndex(index);
    slotHeaderContextMenuRequested( pos );
    m_view->setContextMenuIndex(QModelIndex());
}

void AccountsEditor::slotHeaderContextMenuRequested( const QPoint &pos )
{
    debugPlan;
    QList<QAction*> lst = contextActionList();
    if ( ! lst.isEmpty() ) {
        QMenu::exec( lst, pos,  lst.first() );
    }
}

Account *AccountsEditor::currentAccount() const
{
    return m_view->currentAccount();
}

void AccountsEditor::slotCurrentChanged(  const QModelIndex &curr )
{
    debugPlan<<curr.row()<<","<<curr.column();
    //slotEnableActions( curr.isValid() );
}

void AccountsEditor::slotSelectionChanged( const QModelIndexList& list)
{
    debugPlan<<list.count();
    updateActionsEnabled( true );
}

void AccountsEditor::slotEnableActions( bool on )
{
    updateActionsEnabled( on );
}

void AccountsEditor::updateActionsEnabled(  bool on )
{
    QList<Account*> lst = m_view->selectedAccounts();
    bool one = lst.count() == 1;
    bool more = lst.count() > 1;
    actionAddAccount->setEnabled( on && !more );
    actionAddSubAccount->setEnabled( on && one );

    bool baselined = project() ? project()->isBaselined() : false;
    actionDeleteSelection->setEnabled( on && one && ! baselined );
}

void AccountsEditor::setupGui()
{
    actionAddAccount  = new QAction(koIcon("document-new"), xi18nc("@action:inmenu", "Add Account"), this);
    actionCollection()->addAction("add_account", actionAddAccount );
    actionCollection()->setDefaultShortcut(actionAddAccount, Qt::CTRL + Qt::Key_I);
    connect( actionAddAccount, &QAction::triggered, this, &AccountsEditor::slotAddAccount );

    actionAddSubAccount  = new QAction(koIcon("document-new"), xi18nc("@action:inmenu", "Add Subaccount"), this);
    actionCollection()->addAction("add_subaccount", actionAddSubAccount );
    actionCollection()->setDefaultShortcut(actionAddSubAccount, Qt::SHIFT + Qt::CTRL + Qt::Key_I);
    connect( actionAddSubAccount, &QAction::triggered, this, &AccountsEditor::slotAddSubAccount );

    actionDeleteSelection  = new QAction(koIcon("edit-delete"), xi18nc("@action:inmenu", "Delete"), this);
    actionCollection()->addAction("delete_selection", actionDeleteSelection );
    actionCollection()->setDefaultShortcut(actionDeleteSelection, Qt::Key_Delete);
    connect( actionDeleteSelection, &QAction::triggered, this, &AccountsEditor::slotDeleteSelection );

    createOptionActions(ViewBase::OptionExpand | ViewBase::OptionCollapse | ViewBase::OptionPrint | ViewBase::OptionPrintPreview | ViewBase::OptionPrintPdf | ViewBase::OptionPrintConfig);
}

void AccountsEditor::slotOptions()
{
    debugPlan;
    AccountseditorConfigDialog *dlg = new AccountseditorConfigDialog( this, m_view, this );
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void AccountsEditor::slotAddAccount()
{
    debugPlan;
    int row = -1;
    Account *parent = m_view->selectedAccount(); // sibling
    if ( parent ) {
        row = parent->parent() ? parent->parent()->indexOf( parent ) : project()->accounts().indexOf( parent );
        if ( row >= 0 ) {
            ++row;
        }
        parent = parent->parent();
    }
    insertAccount( new Account(), parent, row );
}

void AccountsEditor::slotAddSubAccount()
{
    debugPlan;
    insertAccount( new Account(), m_view->selectedAccount(), -1 );
}

void AccountsEditor::insertAccount( Account *account, Account *parent, int row )
{
    QModelIndex i = m_view->model()->insertAccount( account, parent, row );
    if ( i.isValid() ) {
        QModelIndex p = m_view->model()->parent( i );
        if (parent) debugPlan<<" parent="<<parent->name()<<":"<<p.row()<<","<<p.column();
        debugPlan<<i.row()<<","<<i.column();
        if ( p.isValid() ) {
            m_view->setExpanded( p, true );
        }
        m_view->selectionModel()->select( i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect );
        m_view->selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
        m_view->edit( i );
    }
}

void AccountsEditor::slotDeleteSelection()
{
    debugPlan;
    m_view->model()->removeAccounts( m_view->selectedAccounts() );
}

void AccountsEditor::slotAccountsOk()
{
     debugPlan<<"Account Editor : slotAccountsOk";
     //QModelList
     

     //QModelIndex i = m_view->model()->insertGroup( g );
     
}

KoPrintJob *AccountsEditor::createPrintJob()
{
    return m_view->createPrintJob( this );
}

bool AccountsEditor::loadContext(const KoXmlElement &context)
{
    m_view->loadContext(model()->columnMap(), context);
    return true;
}

void AccountsEditor::saveContext(QDomElement &context) const
{
    m_view->saveContext(model()->columnMap(), context);
}

void AccountsEditor::slotEditCopy()
{
    m_view->editCopy();
}

} // namespace KPlato
