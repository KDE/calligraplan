/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "SummaryView.h"
#include "SummaryModel.h"
#include "MainDocument.h"
#include "ScheduleManagerDelegate.h"
#include "ProjectsModel.h"
#include "PlanGroupDebug.h"

#include <kptproject.h>
#include <kptnodechartmodel.h>
#include <kpttaskdescriptiondialog.h>
#include <kptcommand.h>

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <KoFileDialog.h>
#include <kundo2command.h>
#include <KoPageLayoutDialog.h>

#include <KRecentFilesAction>
#include <KActionCollection>
#include <KXMLGUIFactory>

#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>


SummaryView::SummaryView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KPlato::ViewBase(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    // Disable, not used
    m_printingOptions.headerOptions.group = false;
    m_printingOptions.footerOptions.group = false;

    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_view = new KPlato::TreeViewBase(this);
    m_view->setRootIsDecorated(false);
    layout->addWidget(m_view);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_view->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_view->header(), &QHeaderView::customContextMenuRequested, this, &SummaryView::slotHeaderContextMenuRequested);
    connect(m_view, &QTreeView::customContextMenuRequested, this, &SummaryView::slotContextMenuRequested);
    connect(m_view, &QTreeView::doubleClicked, this, &SummaryView::itemDoubleClicked);


    SummaryModel *model = new SummaryModel(m_view);
    model->setPortfolio(qobject_cast<MainDocument*>(doc));
    m_view->setModel(model);

    const auto header = m_view->header();
    header->moveSection(10, 12); // CPI Cost
    header->moveSection(1, model->columnCount()-1); // Description
    const QList<int> hide = QList<int>()
    << KPlato::ChartItemModel::BCWSCost
    << KPlato::ChartItemModel::BCWPCost
    << KPlato::ChartItemModel::ACWPCost
    << KPlato::ChartItemModel::BCWSEffort
    << KPlato::ChartItemModel::BCWPEffort
    << KPlato::ChartItemModel::ACWPEffort
    << KPlato::ChartItemModel::SPICost;
    for (int c : hide) {
        header->setSectionHidden(model->proxyColumnForExtraColumn(c), true);
    }
    m_view->setItemDelegateForColumn(2/*Schedule*/, new ScheduleManagerDelegate(m_view));

    setWhatsThis(xi18nc("@info:whatsthis",
                        "<title>Projects Summary View</title>"
                        "<para>"
                        "This view summarizes performance data for the projects in your portfolio."
                        "</para><para>"
                        "Performance indexes are shown for both cost based and effort based calculations."
                        "</para><para>"
                        "<nl/><link url='%1'>More...</link>"
                        "</para>", QStringLiteral("portfolio:summary")
                    )
                );
}

SummaryView::~SummaryView()
{
}

void SummaryView::setupGui()
{
    setXMLFile(QStringLiteral("Portfolio_SummaryViewUi.rc"));

    auto a  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("project_description"), a);
    connect(a, &QAction::triggered, this, &SummaryView::slotDescription);

    createOptionActions(KPlato::ViewBase::OptionPrint | KPlato::ViewBase::OptionPrintPreview | KPlato::ViewBase::OptionPrintPdf | KPlato::ViewBase::OptionPrintConfig);
}

void SummaryView::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    debugPortfolio<<idx;
    if (idx.column() == 1 /*Description*/) {
        slotDescription();
    }
}

void SummaryView::slotHeaderContextMenuRequested(const QPoint &pos)
{
    debugPortfolio<<"Context menu"<<pos;
    if (!factory()) {
        debugPortfolio<<"No factory";
        return;
    }
    QString name;
    name = QStringLiteral("summaryview_popup");
    auto menu = static_cast<QMenu*>(factory()->container(name, this));
    if (menu->isEmpty()) {
        debugPortfolio<<"Menu is empty";
        return;
    }
    menu->exec(m_view->viewport()->mapToGlobal(pos));
}

void SummaryView::slotContextMenuRequested(const QPoint &pos)
{
    debugPortfolio<<"Context menu"<<pos;
    if (!factory()) {
        debugPortfolio<<"No factory";
        return;
    }
    QString name;
    if (m_view->indexAt(pos).isValid()) {
        name = QStringLiteral("summaryview_project_popup");
    } else {
        name = QStringLiteral("summaryview_popup");
    }
    auto menu = static_cast<QMenu*>(factory()->container(name, this));
    if (menu->isEmpty()) {
        debugPortfolio<<"Menu is empty";
        return;
    }
    menu->exec(m_view->viewport()->mapToGlobal(pos));
}

void SummaryView::slotDescription()
{
    auto idx = m_view->selectionModel()->currentIndex();
    if (!idx.isValid()) {
        debugPortfolio<<"No current project";
        return;
    }
    auto doc = m_view->model()->data(idx, DOCUMENT_ROLE).value<KoDocument*>();
    auto project = doc->project();
    KPlato::TaskDescriptionDialog dia(*project, this, m_readWrite);
    if (dia.exec() == QDialog::Accepted) {
        auto m = dia.buildCommand();
        if (m) {
            doc->addCommand(m);
        }
    }
}

void SummaryView::slotOptions()
{
    KoPageLayoutDialog dlg(this, pageLayout());
    if (dlg.exec() == QDialog::Accepted) {
        setPageLayout(dlg.pageLayout());
    }
}

void SummaryView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu *SummaryView::popupMenu(const QString& name)
{
    Q_UNUSED(name)
    return nullptr;
}

KoPrintJob *SummaryView::createPrintJob()
{
    // TODO create some page header/footer
    return m_view->createPrintJob(this);
}

void SummaryView::saveSettings(QDomElement &settings) const
{
    saveContext(settings);
}

void SummaryView::loadSettings(KoXmlElement &settings)
{
    loadContext(settings);
}
