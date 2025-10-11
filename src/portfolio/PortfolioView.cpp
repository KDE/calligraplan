/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "PortfolioView.h"
#include "PortfolioModel.h"
#include "MainDocument.h"
#include "Part.h"

#include <kptproject.h>
#include <ExtraProperties.h>

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
    setHeaderData(0, Qt::Horizontal, i18nc("@title:column", "Recent Portfolios"));
    for (const QAction *a : actions) {
        // KRecentFilesAction format: <name> [<file path>]
        QString s = a->text();
        QString file = s.mid(s.indexOf(QLatin1Char('['))+1);
        file = file.left(file.lastIndexOf(QLatin1Char(']')));
        if (!file.endsWith(QStringLiteral(".planp"))) {
            continue;
        }
        insertRow(0);
        const auto idx = index(0, 0);
        setData(idx, s, Qt::EditRole);
        setData(idx, file, Qt::UserRole+1);
    }
}

PortfolioView::PortfolioView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    setXMLFile(QStringLiteral("Portfolio_PortfolioViewUi.rc"));

    setupGui();

    auto portfolio = qobject_cast<MainDocument*>(doc);
    connect(portfolio, &MainDocument::documentInserted, this, &PortfolioView::slotUpdateView);

    ui.setupUi(this);

    m_recentProjects = new RecentFilesModel(this);
    ui.recentPortfolios->setModel(m_recentProjects);
    auto mw = mainWindow();
    if (mw) {
        KSharedConfigPtr configPtr = mw->componentData().config();
        KRecentFilesAction recent(QStringLiteral("x"), nullptr);
        recent.loadEntries(configPtr->group(QStringLiteral("Recent Portfolios")));
        m_recentProjects->setRecentFiles(recent);
    }
    connect(ui.recentPortfolios, &QAbstractItemView::activated, this, &PortfolioView::slotRecentFileActivated);

    PortfolioModel *model = new PortfolioModel(ui.treeView);
    model->setPortfolio(portfolio);
    ui.treeView->setModel(model);
    model->setDelegates(ui.treeView);
    connect(ui.treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PortfolioView::selectionChanged);
    updateActionsEnabled();

    ui.stackedWidget->setCurrentIndex(portfolio->documents().isEmpty() ? 0 : 1);

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
                                   "Add the projects you are managing and set <emphasis>Portfolio</emphasis> = <emphasis>Yes</emphasis>."
                                   "</para><para>"
                                   "<note>If you have resources that are shared between multiple projects and you want to re-schedule or inspect resource usage, you need to add all relevant projects"
                                   " and then set <emphasis>Portfolio</emphasis> = <emphasis>No</emphasis> for these projects.</note>"
                                   "</para><para>"
                                   "<link url='%1'>More...</link>"
                                   "</para>", QStringLiteral("portfolio:content-editor")
                                   )
                      );

    ui.contextHelp->setText(
        xi18nc("@info",
               "<subtitle>Context Help</subtitle>"
               "<para>"
               "Many functions have help and hints that can be displayed with <emphasis>What's this</emphasis>."
               "</para><para>"
               "Activate it by selecting the menu item <interface>Help|What's this</interface> or by pressing <shortcut>Shift+F1</shortcut>."
               "</para><para>"
               "Try it on this text!"
               "</para>"));

    ui.onlineLabel->setText(xi18nc("@info", "<subtitle>Online Resources</subtitle>"));

    ui.contextHelp->setWhatsThis(
                   xi18nc("@info:whatsthis",
                          "<title>Context help</title>"
                          "<para>"
                          "Help is available many places using <emphasis>What's This</emphasis>."
                          "<nl/>It is activated using the menu entry <interface>Help->What's this?</interface>"
                          " or the keyboard shortcut <shortcut>Shift+F1</shortcut>."
                          "</para><para>"
                          "In dialogs it is available via the <interface>?</interface> in the dialog title bar."
                          "</para><para>"
                          "If you see <link url='%1'>More...</link> in the text,"
                          " pressing it will display more information from the documentation."
                          "</para>", QStringLiteral("portfolio:context-help")));
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
    bool enable = ui.treeView->selectionModel() && (ui.treeView->selectionModel()->selectedRows().count() == 1);
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
     KoFileDialog dlg(nullptr, KoFileDialog::OpenFiles, i18n("Add Project"));
     dlg.setMimeTypeFilters(QStringList()<<PLAN_MIME_TYPE);
     const auto urls = dlg.filenames();
     for (const auto &url : urls) {
         loadProject(url);
     }
}

void PortfolioView::slotRemoveSelected()
{
    PortfolioModel *m = qobject_cast<PortfolioModel*>(ui.treeView->model());
    QList<KoDocument*> docs;
    const auto selectedRows = ui.treeView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : selectedRows) {
        KoDocument *doc = m->documentFromIndex(idx);
        if (doc) {
            docs << doc;
        }
    }
    for (KoDocument *doc : std::as_const(docs)) {
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
    if (!part) {
        KMessageBox::error(this, xi18nc("@info", "Failed to load the project. Not a valid Plan file:<nl/>%1", url.toDisplayString()),
                           i18nc("@title:window", "Could not add project"));
        return;
    }
    if (qobject_cast<Part*>(part)) {
        KMessageBox::error(this, xi18nc("@info", "Failed to open the project. This seems to be a portfolio file:<nl/>%1", url.toDisplayString()),
                           i18nc("@title:window", "Could not add project"));
        delete part;
        return;
    }
    KoDocument *doc = part->createDocument(part);
    doc->setAutoSave(0);
    doc->setProperty(BLOCKSHAREDPROJECTSLOADING, true);
    connect(doc, &KoDocument::sigProgress, mainWindow(), &KoMainWindow::slotProgress);
    connect(doc, &KoDocument::completed, this, &PortfolioView::slotLoadCompleted);
    connect(doc, &KoDocument::canceled, this, &PortfolioView::slotLoadCanceled);
    doc->openUrl(url);
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
    if (!doc->project()) {
        KMessageBox::error(this, xi18nc("@info", "Failed to load the project. Not a valid Plan file:<nl/>%1", doc->url().toDisplayString()),
                           i18nc("@title:window", "Could not add project"));
        doc->deleteLater();
    } else if (!portfolio->addDocument(doc)) {
        KMessageBox::error(this, xi18nc("@info", "The project already exists, the project will not be added.<nl/>Project: %1<nl/> Document: %2", doc->project()->name(), doc->url().toDisplayString()),
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
    MainDocument *portfolio = qobject_cast<MainDocument*>(koDocument());
    Q_ASSERT(portfolio);
    if (portfolio->isEmpty() && portfolio->documents().isEmpty()) {
        ui.stackedWidget->setCurrentIndex(0);
    } else {
        ui.stackedWidget->setCurrentIndex(1);
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

void PortfolioView::saveSettings(QDomElement &settings) const
{
    Q_UNUSED(settings)
}

void PortfolioView::loadSettings(KoXmlElement &settings)
{
    Q_UNUSED(settings)
}
