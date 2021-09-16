/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceUsageView.h"
#include "ResourceModel.h"
#include "MainDocument.h"

#include <ScrollableChart.h>

#include <KActionCollection>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>
#include <KToolBar>

#include <KChartBarDiagram>
#include <KChartLegend>

#include <QSpinBox>

ResourceUsageView::ResourceUsageView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    ui.setupUi(this);

    if (doc && doc->isReadWrite()) {
        setXMLFile("Portfolio_ResourceUsageViewUi.rc");
    } else {
        setXMLFile("Portfolio_ResourceUsageViewUi_readonly.rc");
    }
    setupGui();

    m_numDays.setSpecialValueText(i18n("All"));

    auto resourceModel = new ResourceModel(this);
    resourceModel->setPortfolio(qobject_cast<MainDocument*>(doc));
    ui.resourceView->setModel(resourceModel);

    m_resourceUsageModel.setPortfolio(qobject_cast<MainDocument*>(doc));

    auto bar = qobject_cast<KChart::BarDiagram*>(ui.chart->chart()->coordinatePlane()->diagram());
    bar->setType(KChart::BarDiagram::Stacked);
    ui.chart->legend()->setTitleText(i18n("Tasks"));
    ui.chart->setDataModel(&m_resourceUsageModel);

    //ui.chart->chart()->coordinatePlane()->setRubberBandZoomingEnabled(true);

    connect(ui.resourceView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ResourceUsageView::slotCurrentIndexChanged);

    connect(&m_resourceUsageModel, &ResourceUsageModel::modelReset, this, &ResourceUsageView::slotUpdateNumDays);
    connect(&m_numDays, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResourceUsageView::slotNumDaysChanged);
    slotUpdateNumDays();
}

ResourceUsageView::~ResourceUsageView()
{
}

void ResourceUsageView::setupGui()
{
    auto s = new KSelectAction(this);
    actionCollection()->addAction("diagramtypes", s);
    connect(s, &KSelectAction::indexTriggered, ui.chart, &KPlato::ScrollableChart::setDiagramFlavor);

    auto a = new QAction(i18n("Normal"), this);
    a->setObjectName("charttype_normal");
    a->setCheckable(true);
    s->addAction(a);
    a = new QAction(i18n("Stacked"), this);
    a->setObjectName("charttype_stacked");
    a->setCheckable(true);
    s->addAction(a);
    s->setCurrentAction(a);
}

void ResourceUsageView::guiActivateEvent(bool activated)
{
    auto toolBar = qobject_cast<KToolBar*>(factory()->container("resourceusageview_toolbar", this));
    if (!toolBar) {
        return;
    }
    if (activated) {
        if (!toolBar->findChildren<QSpinBox*>("resourceusageview_numDays").value(0)) {
            auto spinBox = new QSpinBox();
            spinBox->setObjectName(("resourceusageview_numDays"));
            spinBox->setValue(m_numDays.value());
            spinBox->setSpecialValueText(m_numDays.specialValueText());
            spinBox->setRange(m_numDays.minimum(), m_numDays.maximum());
            toolBar->addWidget(spinBox);
            connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), &m_numDays, &QSpinBox::setValue);
        }
    } else {
        auto spinBox = toolBar->findChildren<QSpinBox*>("resourceusageview_numDays").value(0);
        if (spinBox) {
            m_numDays.setRange(spinBox->minimum(), spinBox->maximum());
        }
    }
}

void ResourceUsageView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * ResourceUsageView::popupMenu(const QString& name)
{
    return nullptr;
}

KoPrintJob *ResourceUsageView::createPrintJob()
{
    return nullptr;
}

void ResourceUsageView::slotCurrentIndexChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    m_resourceUsageModel.setCurrentResource(current.data(RESOURCEID_ROLE).toString());
}

void ResourceUsageView::slotUpdateNumDays()
{
    int end = m_resourceUsageModel.rowCount();
    m_numDays.setMaximum(end);
    auto spinBox = findChild<QSpinBox*>("resourceusageview_numDays");
    if (spinBox) {
        spinBox->setMaximum(end);
    }

    slotNumDaysChanged(m_numDays.value());
}

void ResourceUsageView::slotNumDaysChanged(int value)
{
    ui.chart->setNumRows(value);
}
