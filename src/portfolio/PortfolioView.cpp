/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "PortfolioView.h"
#include "PortfolioModel.h"
#include "MainDocument.h"

#include <kptproject.h>

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <KoFileDialog.h>
#include <KoNetAccess.h>

#include <KRecentFilesAction>
#include <KActionCollection>
#include <KUser>
#include <KMessageBox>
#include <KConfigGroup>

#include <QTreeView>
#include <QVBoxLayout>
#include <QDir>
#include <QStackedWidget>
#include <QStandardItemModel>

class RecentFilesModel : public QStandardItemModel
{
public:
    RecentFilesModel(QObject *parent = nullptr);
    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    void setRecentFiles(const KRecentFilesAction &recent);
    void populate(const QList<QAction*> actions);
};

RecentFilesModel::RecentFilesModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

Qt::ItemFlags RecentFilesModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f = (QStandardItemModel::flags(idx) & ~Qt::ItemIsEditable);
    return f;
}

QVariant RecentFilesModel::data(const QModelIndex &idx, int role) const
{
    switch(role) {
        case Qt::DecorationRole:
            return QIcon::fromTheme(QStringLiteral("document-open"));
        case Qt::FontRole: {
            break;
        }
        default: break;
    }
    return QStandardItemModel::data(idx, role);
}

void RecentFilesModel::setRecentFiles(const KRecentFilesAction &recent)
{
    populate(recent.actions());
}

void RecentFilesModel::populate(const QList<QAction*> actions)
{
    clear();
    setColumnCount(1);
    setRowCount(actions.count());
    setHeaderData(0, Qt::Horizontal, i18nc("@title:column", "Recent Portfolios"));
    QModelIndex idx = index(rowCount()-1, 0);
    for (const QAction *a : actions) {
        // KRecentFilesAction format: <name> [<file path>]
        QString s = a->text();
        QString file = s.mid(s.indexOf(QLatin1Char('['))+1);
        file = file.left(file.lastIndexOf(QLatin1Char(']')));
        setData(idx, s, Qt::EditRole);
        setData(idx, file, Qt::UserRole+1);
        idx = idx.sibling(idx.row()-1, idx.column());
    }
}

PortfolioView::PortfolioView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile(QStringLiteral("Portfolio_PortfolioViewUi.rc"));
    } else {
        setXMLFile(QStringLiteral("Portfolio_PortfolioViewUi_readonly.rc"));
    }
    setupGui();

    auto portfolio = qobject_cast<MainDocument*>(doc);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    m_stackedWidget = new QStackedWidget(this);
    layout->addWidget(m_stackedWidget);

    m_welcome = new QTreeView();
    m_welcome->setRootIsDecorated(false);
    m_welcome->setSelectionMode(QAbstractItemView::SingleSelection);
    auto mw = mainWindow();
    m_recentProjects = new RecentFilesModel(this);
    m_welcome->setModel(m_recentProjects);
    if (mw) {
        KSharedConfigPtr configPtr = mw->componentData().config();
        KRecentFilesAction recent(QStringLiteral("x"), nullptr);
        recent.loadEntries(configPtr->group("Recent Portfolios"));
        m_recentProjects->setRecentFiles(recent);
    }
    m_stackedWidget->addWidget(m_welcome);
    connect(m_welcome, &QAbstractItemView::activated, this, &PortfolioView::slotRecentFileActivated);

    m_view = new QTreeView(this);
    m_view->setRootIsDecorated(false);
    m_stackedWidget->addWidget(m_view);

    PortfolioModel *model = new PortfolioModel(m_view);
    model->setPortfolio(portfolio);
    m_view->setModel(model);
    model->setDelegates(m_view);
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PortfolioView::selectionChanged);
    updateActionsEnabled();

    m_stackedWidget->setCurrentWidget(portfolio->documents().isEmpty() ? m_welcome : m_view);

    connect(model, &PortfolioModel::modelReset, this, &PortfolioView::slotUpdateView);
    connect(model, &PortfolioModel::rowsInserted, this, &PortfolioView::slotUpdateView);
    connect(model, &PortfolioModel::rowsRemoved, this, &PortfolioView::slotUpdateView);

    setWhatsThis(xi18nc("@info:whatsthis",
                                   "<title>The Portfolio Content Editor</title>"
                                   "<para>"
                                   "This editor enables you to configure the content of your portfolio."
                                   "</para><para>"
                                   "Select <interface>Edit|Add</interface> or press <interface>Add...</interface> to add projects to your portfolio.<nl/>"
                                   "Select <interface>Edit|Remove</interface> or press <interface>Remove</interface> to remove selected projects."
                                   "</para><para>"
                                   "Add the projects you are managing and set Portfolio = Yes."
                                   " If you have resources that is shared between multiple projects and you want to re-schedule, you need to add all relevant projects"
                                   " and then set Portfolio = No for these projects."
                                   "<nl/><link url='%1'>More...</link>"
                                   "</para>", QStringLiteral("portfolio:content-editor")
                                   )
                      );
}

PortfolioView::~PortfolioView()
{
}

void PortfolioView::setupGui()
{
    QAction *a = new QAction(koIcon("list-add"), i18n("Add"), this);
    actionCollection()->addAction(QStringLiteral("add_project"), a);
    actionCollection()->setDefaultShortcut(a, Qt::Key_Insert);
    connect(a, &QAction::triggered, this, &PortfolioView::slotAddProject);

    a = new QAction(koIcon("list-remove"), i18n("Remove"), this);
    actionCollection()->addAction(QStringLiteral("remove_selected"), a);
    actionCollection()->setDefaultShortcut(a, Qt::Key_Delete);
    connect(a, &QAction::triggered, this, &PortfolioView::slotRemoveSelected);
}

void PortfolioView::updateActionsEnabled()
{
    bool enable = m_view->selectionModel() && (m_view->selectionModel()->selectedRows().count() == 1);
    actionCollection()->action(QStringLiteral("remove_selected"))->setEnabled(enable);
}

void PortfolioView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    updateActionsEnabled();
}

void PortfolioView::slotAddProject()
{
    const QList<QUrl> urls = QFileDialog::getOpenFileUrls(nullptr, i18n("Add Project"), QUrl(), QStringLiteral("Plan (*.plan)"));
    for (const QUrl &url : urls) {
        loadProject(url);
    }
//     KoFileDialog dlg(nullptr, KoFileDialog::OpenFiles, "Add Project");
//     dlg.setNameFilters(QStringList()<<"Plan (*.plan)");
//     QStringList files = dlg.filenames();
//     if (!files.isEmpty()) {
//         for (const QString &file : files) {
//             loadProject(file);
//         }
//     }

}

void PortfolioView::slotRemoveSelected()
{
    PortfolioModel *m = qobject_cast<PortfolioModel*>(m_view->model());
    QList<KoDocument*> docs;
    const auto selectedRows = m_view->selectionModel()->selectedRows();
    for (const QModelIndex &idx : selectedRows) {
        KoDocument *doc = m->documentFromIndex(idx);
        if (doc) {
            docs << doc;
        }
    }
    for (KoDocument *doc : qAsConst(docs)) {
        m->portfolio()->removeDocument(doc);
    }
}

void PortfolioView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * PortfolioView::popupMenu(const QString& name)
{
    Q_UNUSED(name)
    return nullptr;
}

KoPrintJob *PortfolioView::createPrintJob()
{
    return nullptr;
}

void PortfolioView::loadProject(const QUrl &url)
{
    MainDocument *portfolio = qobject_cast<MainDocument*>(koDocument());
    Q_ASSERT(portfolio);
    KoPart *part = KoApplication::koApplication()->getPartFromUrl(url);
    Q_ASSERT(part);
    if (part) {
        KoDocument *doc = part->createDocument(part);
        doc->setAutoSave(0);
        doc->setProperty(BLOCKSHAREDPROJECTSLOADING, true);
        connect(doc, &KoDocument::sigProgress, mainWindow(), &KoMainWindow::slotProgress);
        connect(doc, &KoDocument::completed, this, &PortfolioView::slotLoadCompleted);
        connect(doc, &KoDocument::canceled, this, &PortfolioView::slotLoadCanceled);
        doc->openUrl(url);
    }
}

bool PortfolioView::hasWriteAccess(KIO::UDSEntry& entry) const
{
    const auto access = entry.numberValue(KIO::UDSEntry::UDS_ACCESS);
    const auto other = access & 07;
    const auto group = access >> 3 & 07;
    const auto user = access >> 6 & 07;
    //const auto more = access >> 9 & 07;

    const auto fileOwner = entry.stringValue(KIO::UDSEntry::UDS_USER);
    const auto fileGroup = entry.stringValue(KIO::UDSEntry::UDS_GROUP);

    KUser ruser(KUser::UseRealUserID);
    bool result = false;
    if (fileOwner == ruser.loginName()) {
        result = user & 02;
    } else if (ruser.groupNames().contains(fileGroup)) {
        result = group & 02;
    } else {
        result = other & 02;
    }
    return result;
}

void PortfolioView::slotLoadCompleted()
{
    KoDocument *doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    MainDocument *portfolio = qobject_cast<MainDocument*>(koDocument());
    disconnect(doc, &KoDocument::sigProgress, mainWindow(), &KoMainWindow::slotProgress);
    disconnect(doc, &KoDocument::completed, this, &PortfolioView::slotLoadCompleted);
    disconnect(doc, &KoDocument::canceled, this, &PortfolioView::slotLoadCanceled);

    KIO::UDSEntry entry;
    if (KIO::NetAccess::stat(doc->url(), entry, doc->documentPart()->currentMainwindow())) {
        doc->setProperty(ORIGINALMODIFICATIONTIME, entry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME));
        bool writeaccess = hasWriteAccess(entry);
        doc->setProperty(ISPORTFOLIO, writeaccess);
        if (writeaccess) {
            doc->setProperty(SCHEDULINGCONTROL, QStringLiteral("Schedule"));
        } else {
            doc->setProperty(SCHEDULINGCONTROL, QStringLiteral("Include"));
        }
        doc->setProperty(SCHEDULINGPRIORITY, 0);
    }

    if (!portfolio->addDocument(doc)) {
        KMessageBox::sorry(this, xi18nc("@info", "The project already exists.<nl/>Project: %1<nl/> Document: %2", doc->project()->name(), doc->url().toDisplayString()),
                           i18nc("@title:window", "Could not add project"));
        doc->deleteLater();
    } else {
        auto manager = portfolio->findBestScheduleManager(doc);
        if (manager) {
            portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, manager->name());
        }
    }
}

void PortfolioView::slotLoadCanceled()
{
    KoDocument *doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    doc->deleteLater();
}

void PortfolioView::slotUpdateView()
{
    if (koDocument()->isEmpty()) {
        m_stackedWidget->setCurrentWidget(m_welcome);
    } else {
        m_stackedWidget->setCurrentWidget(m_view);
    }
    updateActionsEnabled();
}

void PortfolioView::slotRecentFileActivated(const QModelIndex &idx)
{
    QUrl url = QUrl::fromUserInput(idx.data(Qt::UserRole+1).toString());
    if (url.isValid()) {
        auto mw = mainWindow();
        if (mw) {
            mw->slotFileOpenRecent(url);
        }
    }
}
