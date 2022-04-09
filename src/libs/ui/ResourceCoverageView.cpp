/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceCoverageView.h"

#include "kptresource.h"
#include "kptresourceappointmentsmodel.h"
#include "kptproject.h"
#include "kptschedule.h"

#include <KChartChart>
#include <KChartAbstractCoordinatePlane>
#include <KChartBarDiagram>
#include <KChartLineDiagram>
#include <KChartLegend>
#include <KChartPalette>

#include <KSelectionProxyModel>

#include <QSplitter>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QBrush>
#include <QPixmap>
#include <QTableView>
#include <QDebug>

namespace KPlato
{
class FilterModel : public QSortFilterProxyModel
{
    bool m_colorsEnabled;
    KChart::Palette m_palette;
public:
    FilterModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
        , m_colorsEnabled(false)
        , m_palette(KChart::Palette::subduedPalette())
    {}
    ~FilterModel() {}

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override {
        QVariant v;
        switch (role) {
            case KChart::DatasetBrushRole: {
                if (m_colorsEnabled) {
                    v = m_palette.getBrush(idx.row());
                } else {
                    v = QSortFilterProxyModel::data(idx, role);
                }
                break;
            }
            case KChart::DatasetPenRole: {
                if (m_colorsEnabled) {
                    v = m_palette.getBrush(idx.row()).color();
                } else {
                    v = QSortFilterProxyModel::data(idx, role);
                }
                break;
            }
            case Qt::DecorationRole:
                if (m_colorsEnabled && idx.column() == 0) {
                    QPixmap p(16,16);
                    p.fill(m_palette.getBrush(idx.row()).color());
                    v = p;
                } else {
                    v = QSortFilterProxyModel::data(idx, role);
                }
                break;
            default: {
                v = QSortFilterProxyModel::data(idx, role);
                break;
            }
        }
        return v;
    }
    bool filterAcceptsRow(int source_column, const QModelIndex &source_parent) const override {
        Q_UNUSED(source_column)
        return !source_parent.isValid();
    }
    bool filterAcceptsColumn(int source_column, const QModelIndex &/*source_parent*/) const override {
        if (!m_shownColumns.isEmpty()) {
            return m_shownColumns.contains(source_column);
        }
        return !m_hiddenColumns.contains(source_column);
    }
    void setShownColumns(const QList<int> &cols) {
        m_shownColumns = cols;
    }
    void setHiddenColumns(const QList<int> &cols) {
        m_hiddenColumns = cols;
    }
    void setColorsEnabled(bool enable) {
        m_colorsEnabled = enable;
    }
    void reset() {
        beginResetModel();
        endResetModel();
    }
private:
    QList<int> m_shownColumns;
    QList<int> m_hiddenColumns;
};

class ValuesModel : public QSortFilterProxyModel
{
public:
    ValuesModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}
    ~ValuesModel() {}

    QVariant headerData(int section, Qt::Orientation o, int role = Qt::DisplayRole) const override {
        if (o == Qt::Horizontal) {
            return QAbstractProxyModel::headerData(section, o, role); // on purpose, clazy:exclude=skipped-base-method
        } else { // Qt::Vertical
            QModelIndex idx;
            if (sourceModel()) {
                idx = sourceModel()->index(section, 0);
            }
            return idx.data(role);
        }
        return QVariant();
    }
    QVariant data(const QModelIndex &idx, int role) const override {
        QVariant v;
        switch (role) {
            case Qt::ToolTipRole: {
                const QString name = headerData(idx.row(), Qt::Vertical, Qt::DisplayRole).toString();
                const QString avail = idx.data(Role::Maximum).toString();
                const QString booked = idx.data(Qt::EditRole).toString();
                v = xi18nc("@info:tooltip", "<title>%1</title><para>Available: %2<nl/>Booked: %3</para>", name, avail, booked);
                break;
            }
            default: {
                int rl = role == Qt::DisplayRole ? Qt::EditRole : role;
                v = QSortFilterProxyModel::data(idx, rl);
                break;
            }
        }
        return v;
    }
    bool filterAcceptsColumn(int source_column, const QModelIndex &/*source_parent*/) const override {
        return source_column > 0; // column 0 is moved to vertical header
    }
    void reset() {
        beginResetModel();
        endResetModel();
    }
};
class SwapModel : public QAbstractProxyModel
{
public:
    SwapModel(QObject *parent = nullptr) : QAbstractProxyModel(parent) {}
    ~SwapModel() {}

    QModelIndex parent(const QModelIndex &idx) const override {
        Q_UNUSED(idx)
        return QModelIndex();
    }
    bool hasChildren(const QModelIndex &idx) const override {
        return rowCount(idx) > 0;
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        int c = sourceModel() ? sourceModel()->columnCount(parent) : 0;
        return c;
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        int r = sourceModel() ? sourceModel()->rowCount(parent) : 0;
        return r;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        Qt::Orientation o = Qt::Horizontal;
        if (orientation == Qt::Horizontal) {
            o = Qt::Vertical;
        }
        QVariant v;
        if (sourceModel()) {
            v = sourceModel()->headerData(section, o, role);
        }
        return v;
    }
    QModelIndex index(int r, int c, const QModelIndex &p = QModelIndex()) const override {
        Q_UNUSED(p)
        return createIndex(r, c, nullptr);
    }
    QModelIndex mapToSource(const QModelIndex &idx) const override {
        return sourceModel() ? sourceModel()->index(idx.column(), idx.row()) : QModelIndex();
    }
    QModelIndex mapFromSource(const QModelIndex &idx) const override {
        return createIndex(idx.column(), idx.row());
    }
    void reset() {
        beginResetModel();
        endResetModel();
    }
};
class AvailableModel : public QAbstractProxyModel
{
public:
    AvailableModel(QObject *parent = nullptr) : QAbstractProxyModel(parent) {}
    ~AvailableModel() {}

    QModelIndex parent(const QModelIndex &idx) const override {
        Q_UNUSED(idx)
        return QModelIndex();
    }
    bool hasChildren(const QModelIndex &idx) const override {
        return rowCount(idx) > 0;
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        int c = sourceModel() ? sourceModel()->rowCount(parent) : 0;
        return c;
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        int r = sourceModel() ? sourceModel()->columnCount(parent) : 0;
        return r;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        QVariant v = sourceModel() ? sourceModel()->headerData(section, orientation, role) : QVariant();
        return v;
    }
    QVariant data(const QModelIndex &idx, int role) const override {
        QVariant v;
        switch (role) {
            case KChart::DatasetBrushRole: {
                v = QBrush(QAbstractProxyModel::data(idx, role).value<QBrush>().color(), Qt::Dense5Pattern);
                break;
            }
            case KChart::DatasetPenRole:
            case KChart::DataValueLabelAttributesRole:
            case KChart::ThreeDAttributesRole:
            case KChart::LineAttributesRole:
            case KChart::ThreeDLineAttributesRole:
            case KChart::BarAttributesRole:
            case KChart::StockBarAttributesRole:
            case KChart::ThreeDBarAttributesRole:
            case KChart::PieAttributesRole:
            case KChart::ThreeDPieAttributesRole:
            case KChart::ValueTrackerAttributesRole:
            case KChart::DataHiddenRole:
                break;
            case Qt::ToolTipRole: {
                const QString name = headerData(idx.row(), Qt::Horizontal, Qt::DisplayRole).toString();
                const QString avail = idx.data(Role::Maximum).toString();
                const QString booked = idx.data(Qt::EditRole).toString();
                v = xi18nc("@info:tooltip", "<title>%1</title><para>Available: %2<nl/>Booked: %3</para>", name, avail, booked);
                break;
            }
            default: {
                int rl = role == Qt::DisplayRole ? Role::Maximum : role;
                v = QAbstractProxyModel::data(idx, rl);
                break;
            }
        }
        return v;
    }
    QModelIndex index(int r, int c, const QModelIndex &p = QModelIndex()) const override {
        Q_UNUSED(p)
        return createIndex(r, c, nullptr);
    }
    QModelIndex mapToSource(const QModelIndex &idx) const override {
        return sourceModel() ? sourceModel()->index(idx.row(), idx.column()) : QModelIndex();
    }
    QModelIndex mapFromSource(const QModelIndex &idx) const override {
        return createIndex(idx.row(), idx.column());
    }
    void reset() {
        beginResetModel();
        endResetModel();
    }
};
}

using namespace KPlato;

ResourceCoverageView::ResourceCoverageView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    setXMLFile(QStringLiteral("ResourceCoverageViewUi.rc"));

    QSplitter *splitter = new QSplitter(this);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(splitter);

    // base data model
    m_treeView = new TreeViewBase(this);
    m_baseModel = new ResourceAppointmentsItemModel(this);

    FilterModel *data = new FilterModel(this);
    data->setHiddenColumns(QList<int>() << 1); // remove total usage
    data->setColorsEnabled(true);
    data->setSourceModel(m_baseModel);

    // get resource name (only) in the left view
    FilterModel *tvm = new FilterModel(this);
    tvm->setShownColumns(QList<int>() << 0);
    tvm->setSourceModel(data);

    m_treeView->setModel(tvm);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setRootIsDecorated(false);
    splitter->addWidget(m_treeView);

    // selected resources
    KSelectionProxyModel *selection = new KSelectionProxyModel(m_treeView->selectionModel(), this);
    selection->setSourceModel(data);


    ValuesModel *vmodel = new ValuesModel(this); // moves row 0 (resource name) to vertical header
    vmodel->setSourceModel(selection);

    m_usageModel = new SwapModel(this); // swaps rows/columns to fit chart
    m_usageModel->setSourceModel(vmodel);

    // models to get resource availability
    m_availModel = new AvailableModel(this);
    m_availModel->setSourceModel(m_usageModel);

    m_chart = new KChart::Chart(this);

    KChart::CartesianAxis *xaxis = new KChart::CartesianAxis();
    xaxis->setPosition(KChart::CartesianAxis::Bottom);
    KChart::CartesianAxis *yaxis = new KChart::CartesianAxis();
    yaxis->setPosition(KChart::CartesianAxis::Left);

    KChart::BarDiagram *avail = new KChart::BarDiagram;
    avail->setModel(m_availModel);
    m_chart->coordinatePlane()->addDiagram(avail);
    avail->addAxis(xaxis);
    avail->addAxis(yaxis);

    // resource coverage (painted on top of avail)
    KChart::BarDiagram *usage = new KChart::BarDiagram;
    usage->setModel(m_usageModel);
    m_chart->coordinatePlane()->addDiagram(usage);

    KChart::Legend *legend = new KChart::Legend(usage);
    legend->setPosition(KChartEnums::PositionEast);
    m_chart->addLegend(legend);

    splitter->addWidget(m_chart);

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ResourceCoverageView::selectionChanged);
}
ResourceCoverageView::~ResourceCoverageView()
{
}
void ResourceCoverageView::selectionChanged()
{
    // updates chart
    m_usageModel->reset();
    m_availModel->reset();
}
void ResourceCoverageView::setProject(Project *project)
{
    m_baseModel->setProject(project);
    ViewBase::setProject(project);
}

void ResourceCoverageView::setScheduleManager(ScheduleManager *sm)
{
    m_baseModel->setScheduleManager(sm);
    ViewBase::setScheduleManager(sm);
}

QList<Resource*> ResourceCoverageView::selectedResources() const
{
    QList<Resource*> resources;
    return resources;
}

void ResourceCoverageView::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event)
}
