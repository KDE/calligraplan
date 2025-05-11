/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "ReportsGeneratorView.h"

#include "reportgenerator/ReportGenerator.h"
#include <reportgenerator/ReportGeneratorFactory.h>
#include "kptdebug.h"

#include <KoIcon.h>
#include <KoXmlReader.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoComponentData.h>

#include <KActionCollection>
#include <KUrlRequester>
#include <KFile>
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KMessageBox>
#include <KConfigGroup>

#include <QAction>
#include <QHeaderView>
#include <QTreeView>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QModelIndexList>
#include <QStyledItemDelegate>
#include <QString>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QMap>


const QLoggingCategory &PLANREPORT_LOG()
{
    static const QLoggingCategory category("calligra.plan.report");
    return category;
}
#define debugPlanReport qCDebug(PLANREPORT_LOG)<<Q_FUNC_INFO

using namespace KPlato;

#define FULLPATHROLE Qt::UserRole + 123

class TemplateFileDelegate : public QStyledItemDelegate
{
public:
    TemplateFileDelegate(KoPart *part, QObject *parent);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    KoPart *m_part;
    mutable QMap<QString, QUrl> m_files;
};

TemplateFileDelegate::TemplateFileDelegate(KoPart *part, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_part(part)
{
}

QWidget *TemplateFileDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index);
    return new QComboBox(parent);
}

void TemplateFileDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    m_files.clear();
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    debugPlanReport<<cb;
    if (!cb) {
        return;
    }
    QString path = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("reports"), QStandardPaths::LocateDirectory);
    debugPlanReport<<"standardpath:"<<path;
    if (!path.isEmpty()) {
        QDir dir(path);
        const auto entries = dir.entryList(QDir::Files|QDir::QDir::NoDotAndDotDot);
        for(auto &file : entries) {
            QUrl url;
            url.setUrl(path + QLatin1Char('/') + file);
            m_files.insert(url.fileName(), url);
        }
    }
    KConfigGroup cfgGrp(m_part->componentData().config(), "Report Templates");
    if (cfgGrp.exists()) {
        const auto templates = cfgGrp.readEntry(QStringLiteral("ReportTemplatePaths")).split(QLatin1Char(','));
        for (auto &path : templates) {
            QDir dir(path);
            const auto entries = dir.entryList(QDir::Files|QDir::QDir::NoDotAndDotDot);
            for(auto &file : entries) {
                QUrl url;
                url.setUrl(path + QLatin1Char('/') + file);
                m_files.insert(url.fileName(), url);
            }
        }
    }
    cb->setEditable(true);
    cb->addItems(m_files.keys());
    QString file = index.data().toString();
    cb->setCurrentText(file);
}

void TemplateFileDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    debugPlanReport<<cb;
    if (cb) {
        QString cfile = index.data().toString();
        QString nfile = cb->currentText();
        debugPlanReport<<"template file:"<<nfile<<m_files;
        if (cfile != nfile) {
            model->setData(index, nfile);
            if (m_files.contains(nfile)) {
                nfile = m_files[nfile].url();
            }
            model->setData(index, nfile, FULLPATHROLE);
            model->setData(index, nfile, Qt::ToolTipRole);
        }
    } else debugPlanReport<<"  No combo box editor!!";
}

class FileItemDelegate : public QStyledItemDelegate
{
public:
    FileItemDelegate(QObject *parent);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    QMap<QString, QUrl> files;
};

FileItemDelegate::FileItemDelegate(QObject *parent)
: QStyledItemDelegate(parent)
{
}

QWidget *FileItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    KUrlRequester *u = new KUrlRequester(parent);
    u->setMode(KFile::File);
    return u;
}

void FileItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KUrlRequester *u = qobject_cast<KUrlRequester*>(editor);
    QString file = index.data().toString();
    if (!file.isEmpty()) {
        u->setUrl(QUrl(file));
    }
}

void FileItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KUrlRequester *u = qobject_cast<KUrlRequester*>(editor);
    if (u && index.isValid()) {
        model->setData(index, u->url().url());
    }
}

class FileNameExtensionDelegate : public QStyledItemDelegate
{
public:
    FileNameExtensionDelegate(QObject *parent);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

};

FileNameExtensionDelegate::FileNameExtensionDelegate(QObject *parent)
: QStyledItemDelegate(parent)
{
}

QWidget *FileNameExtensionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    QComboBox *cb = new QComboBox(parent);
    for (int i = 0; i < ReportsGeneratorView::addOptions().count(); ++i) {
        cb->addItem(ReportsGeneratorView::addOptions().at(i), ReportsGeneratorView::addTags().value(i));
    }
    return cb;
}

void FileNameExtensionDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    if (cb) {
        int idx = ReportsGeneratorView::addTags().indexOf(index.data(Qt::UserRole).toString());
        cb->setCurrentIndex(idx < 0 ? 0 : idx);
    }
}

void FileNameExtensionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox*>(editor);
    if (cb && index.isValid()) {
        model->setData(index, cb->currentData(), Qt::UserRole);
        model->setData(index, cb->currentText());
    }
}

QStringList ReportsGeneratorView::addOptions()
{
    return QStringList() << i18n("Nothing") << i18n("Date") << i18n("Number");
}

QStringList ReportsGeneratorView::addTags()
{
    return QStringList() << QStringLiteral("Nothing") << QStringLiteral("Date") << QStringLiteral("Number");

}

#define COLUMN_NAME 0
#define COLUMN_TEMPLATE 1
#define COLUMN_ADD 2
#define COLUMN_FILE 3

ReportsGeneratorView::ReportsGeneratorView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    debugPlan<<"----------------- Create ReportsGeneratorView ----------------------";
    setXMLFile(QStringLiteral("ReportsGeneratorViewUi.rc"));

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new QTreeView(this);
    QStandardItemModel *m = new QStandardItemModel(m_view);
    static const QMap<int, QString> headers = {
        { COLUMN_NAME, i18nc("@title:column", "Name") },
        { COLUMN_TEMPLATE, i18nc("@title:column", "Report Template") },
        { COLUMN_ADD, i18nc("@title:column", "Add") },
        { COLUMN_FILE, i18nc("@title:column", "Report File") }
    };
    m->setHorizontalHeaderLabels(headers.values());
    m->setHeaderData(COLUMN_NAME, Qt::Horizontal, xi18nc("@info:tooltip", "Report name"), Qt::ToolTipRole);
    m->setHeaderData(COLUMN_TEMPLATE, Qt::Horizontal, xi18nc("@info:tooltip", "Report template file name"), Qt::ToolTipRole);
    m->setHeaderData(COLUMN_ADD, Qt::Horizontal, xi18nc("@info:tooltip", "Information added to filename"), Qt::ToolTipRole);
    m->setHeaderData(COLUMN_FILE, Qt::Horizontal, xi18nc("@info:tooltip", "Name of the generated report file"), Qt::ToolTipRole);
    connect(m, &QStandardItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&) {
        slotEnableActions();
        Q_EMIT optionsModified(); // FIXME need undo/redo
    });

    m_view->setModel(m);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);

    connect(m_view, &QWidget::customContextMenuRequested, this, &ReportsGeneratorView::slotContextMenuRequested);
    l->addWidget(m_view);

    TemplateFileDelegate *del = new TemplateFileDelegate(part, m_view);
    m_view->setItemDelegateForColumn(COLUMN_TEMPLATE, del);

    m_view->setItemDelegateForColumn(COLUMN_FILE, new FileItemDelegate(m_view));

    m_view->setItemDelegateForColumn(COLUMN_ADD, new FileNameExtensionDelegate(m_view));

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ReportsGeneratorView::slotSelectionChanged);
    setupGui();

    setWhatsThis(
                   xi18nc("@info:whatsthis",
                          "<title>Add and generate reports</title>"
                          "<para>"
                          "Enables you to add and generate reports based on Open Document (.odf) files."
                          "</para><para>"
                          "You can create a report template using any Open Document text editor."
                          "<nl/><link url='%1'>More...</link>"
                          "</para>", QStringLiteral("plan:reports-generator-view")));
}

void ReportsGeneratorView::updateReadWrite(bool rw)
{
    QStandardItemModel *m = static_cast<QStandardItemModel*>(m_view->model());
    for (int r = 0; r < m->rowCount(); ++r) {
        for (int c = 0; c < m->columnCount(); ++c) {
            QStandardItem *item = m->itemFromIndex(m->index(r, c));
            if (rw) {
                item->setFlags(item->flags() | Qt::ItemIsEditable);
            } else {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
        }
    }
    ViewBase::updateReadWrite(rw);
    updateActionsEnabled(isReadWrite());
}

void ReportsGeneratorView::setGuiActive(bool activate)
{
    debugPlanReport<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
}

void ReportsGeneratorView::slotCurrentChanged(const QModelIndex &, const QModelIndex &)
{
    slotEnableActions();
}

void ReportsGeneratorView::slotSelectionChanged()
{
    slotEnableActions();
}

QModelIndexList ReportsGeneratorView::selectedRows() const
{
    return m_view->selectionModel()->selectedRows();
}

int ReportsGeneratorView::selectedRowCount() const
{
    return selectedRows().count();
}

void ReportsGeneratorView::slotContextMenuRequested(const QPoint& pos)
{
    const auto idx = m_view->currentIndex();
    if (idx.column() == COLUMN_FILE) {
        const auto fname = idx.data().toString();
        if (!fname.isEmpty()) {
            openPopupMenu(QStringLiteral("generatereports_popup"), m_view->viewport()->mapToGlobal(pos));
        }
    }
}

void ReportsGeneratorView::slotEnableActions()
{
    updateActionsEnabled(isReadWrite());
}

void ReportsGeneratorView::updateActionsEnabled(bool on)
{
    actionAddReport->setEnabled(on);
    const auto lst = selectedRows();
    bool enable = on && lst.count() > 0;
    actionRemoveReport->setEnabled(enable);
    if (enable) {
        for (const auto idx : lst) {
            enable &= !idx.siblingAtColumn(COLUMN_TEMPLATE).data().toString().isEmpty();
            enable &= !idx.siblingAtColumn(COLUMN_FILE).data().toString().isEmpty();
            if (!enable) {
                break;
            }
        }
    }
    actionGenerateReport->setEnabled(enable);
}

void ReportsGeneratorView::setupGui()
{
    KActionCollection *coll = actionCollection();

    actionAddReport = new QAction(koIcon("list-add"), i18n("Add Report"), this);
    coll->addAction(QStringLiteral("add_report"), actionAddReport);
    coll->setDefaultShortcut(actionAddReport, Qt::CTRL | Qt::Key_I);
    connect(actionAddReport, &QAction::triggered, this, &ReportsGeneratorView::slotAddReport);

    actionRemoveReport = new QAction(koIcon("list-remove"), i18n("Remove Report"), this);
    coll->addAction(QStringLiteral("remove_report"), actionRemoveReport);
    coll->setDefaultShortcut(actionRemoveReport, Qt::CTRL | Qt::Key_D);
    connect(actionRemoveReport, &QAction::triggered, this, &ReportsGeneratorView::slotRemoveReport);

    actionGenerateReport = new QAction(koIcon("document-export"), i18n("Generate Report"), this);
    coll->addAction(QStringLiteral("generate_report"), actionGenerateReport);
    coll->setDefaultShortcut(actionGenerateReport, Qt::CTRL | Qt::Key_G);
    connect(actionGenerateReport, &QAction::triggered, this, &ReportsGeneratorView::slotGenerateReport);

    auto a = new QAction(koIcon("document-open"), i18n("Open..."), this);
    coll->addAction(QStringLiteral("open_report"), a);
    connect(a, &QAction::triggered, this, [this]() {
        const auto idx = m_view->currentIndex();
        if (idx.column() == COLUMN_FILE) {
            const auto fname = idx.data().toString();
            if (!fname.isEmpty()) {
                auto job = new KIO::OpenUrlJob(QUrl(fname), QStringLiteral("application/vnd.oasis.opendocument.text"));
                job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window()));
                job->start();
            }
        }
    });

//     createOptionAction();
}


void ReportsGeneratorView::slotOptions()
{
    debugPlanReport;
//     SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
//     dlg->addPrintingOptions();
//     connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
//     dlg->show();
//     dlg->raise();
//     dlg->activateWindow();
}

void ReportsGeneratorView::slotAddReport()
{
    debugPlanReport;
    QAbstractItemModel *m = m_view->model();
    int row = m->rowCount();
    m->insertRow(row);
    QModelIndex idx = m->index(row, COLUMN_NAME);
    m->setData(idx, i18n("New report"));
    QModelIndex add = m->index(row, COLUMN_ADD);
    m->setData(add, ReportsGeneratorView::addOptions().at(0));
    m->setData(add, ReportsGeneratorView::addTags().at(0), Qt::UserRole);

    m_view->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    m_view->edit(idx);
    Q_EMIT optionsModified();
}

void ReportsGeneratorView::slotRemoveReport()
{
    debugPlanReport<<selectedRows();
    QAbstractItemModel *m = m_view->model();
    QModelIndexList lst = selectedRows();
    if (lst.isEmpty()) {
        return;
    }
    // Assumption: model is flat
    // We must do this in descending row order
    QMap<int, QModelIndex> map;
    for (int i = 0; i < lst.count(); ++i) {
        map.insert(-lst.at(i).row(), lst.at(i)); // sort descending
    }
    for (const QModelIndex &idx : map) {
        Q_ASSERT(!idx.parent().isValid()); // must be flat
        m->removeRow(idx.row(), idx.parent());
    }
    Q_EMIT optionsModified();
}

void ReportsGeneratorView::slotGenerateReport()
{
    debugPlanReport;
    QAbstractItemModel *model = m_view->model();
    const QModelIndexList indexes = selectedRows();
    for (const QModelIndex &idx : indexes) {
        QString name = model->index(idx.row(), COLUMN_NAME).data().toString();
        QString tmp = model->index(idx.row(), COLUMN_TEMPLATE).data(FULLPATHROLE).toString();
        QString file = QUrl::fromUserInput(model->index(idx.row(), COLUMN_FILE).data().toString()).toLocalFile(); // get rid of possible scheme
        if (tmp.isEmpty()) {
            QMessageBox::information(this, xi18nc("@title:window", "Generate Report"),
                                     xi18n("Failed to generate %1."
                                          "<nl/>Template file name is empty.", name));
            continue;
        }
        if (!QFile::exists(tmp)) {
            QMessageBox::information(this, xi18nc("@title:window", "Generate Report"),
                                     xi18n("Failed to generate %1."
                                          "<nl/>Template file does not exist:<nl/>%2", name, tmp));
            continue;
        }
        if (file.isEmpty()) {
            debugPlanReport<<"No files for report:"<<name<<tmp<<file;
            QMessageBox::information(this, xi18nc("@title:window", "Generate Report"),
                                     xi18n("Failed to generate %1."
                                          "<nl/>Report file name is empty.", name));
            continue;
        }
        QString addition = model->index(idx.row(), COLUMN_ADD).data(Qt::UserRole).toString();
        if (addition == QStringLiteral("Date")) {
            int dotpos = file.lastIndexOf(QLatin1Char('.'));
            QString date = QDate::currentDate().toString();
            file = file.insert(dotpos, date.prepend(QLatin1Char('-')));
        } else if (addition == QStringLiteral("Number")) {
            int dotpos = file.lastIndexOf(QLatin1Char('.'));
            QString fn = file;
            for (int i = 1; QFile::exists(fn); ++i) {
                fn.insert(dotpos, QString::number(i).prepend(QLatin1Char('-')));
                if (!QFile::exists(fn)) {
                    file = fn;
                    break;
                }
                fn = file;
            }
        }
        // warn if file exists
        if (QFile::exists(file)) {
            auto ret = KMessageBox::warningContinueCancel(this, xi18nc("@info", "File exists:<nl/>%1", file), i18nc("@title:window", "Report Generation"));
            if (ret != KMessageBox::Continue) {
                return;
            }
        }
        generateReport(tmp, file);
    }
}

bool ReportsGeneratorView::generateReport(const QString &templateFile, const QString &file)
{
    ReportGeneratorFactory rgf;
    QScopedPointer<ReportGenerator> rg(rgf.createReportGenerator(QStringLiteral("odt")));
    rg->setTemplateFile(templateFile);
    rg->setReportFile(file);
    rg->setProject(project());
    rg->setScheduleManager(scheduleManager());
    if (!rg->initiate()) {
        debugPlanReport<<"Failed to initiate report generator";
        QMessageBox::warning(this, i18n("Failed to open report generator"), rg->lastError());
        return false;
    }
    if (!rg->createReport()) {
        QMessageBox::warning(this, i18n("Failed to create report"), rg->lastError());
        return false;
    }
    if (QMessageBox::question(this, xi18nc("@title:window", "Report Generation"), xi18nc("@info", "Report file generated:<nl/><filename>%1</filename>", file), QMessageBox::Open|QMessageBox::Close, QMessageBox::Close) == QMessageBox::Open) {
        QUrl url(file);
        url.setScheme(QStringLiteral("file"));
        debugPlanReport<<"open:"<<url;
        auto job = new KIO::OpenUrlJob(url, QStringLiteral("application/vnd.oasis.opendocument.text"));
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window()));
        job->start();
    }
    return true;
}

bool ReportsGeneratorView::loadContext(const KoXmlElement &context)
{
    debugPlanReport;
    m_view->header()->setStretchLastSection((bool)(context.attribute(QStringLiteral("stretch-last-column"), QString::number(1)).toInt()));
    KoXmlElement e = context.namedItem("sections").toElement();
    if (!e.isNull()) {
        QHeaderView *h = m_view->header();
        QString s = QString::fromLatin1("section-%1");
        for (int i = 0; i < h->count(); ++i) {
            if (e.hasAttribute(s.arg(i))) {
                int index = e.attribute(s.arg(i), QString::number(-1)).toInt();
                if (index >= 0 && index < h->count()) {
                    h->moveSection(h->visualIndex(index), i);
                }
            }
        }
    }
    KoXmlElement parent = context.namedItem("data").toElement();
    if (!parent.isNull()) {
        debugPlanReport<<"Load data";
        int row = 0;
        QAbstractItemModel *model = m_view->model();
        forEachElement(e, parent) {
            if (e.tagName() != QStringLiteral("row")) {
                continue;
            }
            model->insertRow(row);
            QString name = e.attribute("name");
            QString tmp = e.attribute("template");
            QString file = e.attribute("file");
            QString add = e.attribute("add");
            QModelIndex idx = model->index(row, COLUMN_NAME);
            model->setData(idx, name);
            idx = model->index(row, COLUMN_TEMPLATE);
            model->setData(idx, tmp, FULLPATHROLE);
            model->setData(idx, tmp, Qt::ToolTipRole);
            model->setData(idx, QUrl(tmp).fileName());
            idx = model->index(row, COLUMN_FILE);
            model->setData(idx, file);
            idx = model->index(row, COLUMN_ADD);
            model->setData(idx, add, Qt::UserRole);
            model->setData(idx, ReportsGeneratorView::addOptions().value(ReportsGeneratorView::addTags().indexOf(add)));
            ++row;
        }
    }
    ViewBase::loadContext(context);
    for (int c = 0; c < m_view->header()->count(); ++c) {
        m_view->resizeColumnToContents(c);
    }
    updateReadWrite(isReadWrite());
    return true;
}

void ReportsGeneratorView::saveContext(QDomElement &context) const
{
    debugPlanReport;
    context.setAttribute(QStringLiteral("stretch-last-column"), QString::number(m_view->header()->stretchLastSection()));
    QDomElement e = context.ownerDocument().createElement(QStringLiteral("sections"));
    context.appendChild(e);
    QHeaderView *h = m_view->header();
    for (int i = 0; i < h->count(); ++i) {
        e.setAttribute(QStringLiteral("section-%1").arg(i), h->logicalIndex(i));
    }
    QDomElement data = context.ownerDocument().createElement(QStringLiteral("data"));
    context.appendChild(data);
    const QAbstractItemModel *model = m_view->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        e = data.ownerDocument().createElement(QStringLiteral("row"));
        data.appendChild(e);
        QModelIndex idx = model->index(row, COLUMN_NAME);
        e.setAttribute(QStringLiteral("name"), idx.data().toString());
        idx = model->index(row, COLUMN_TEMPLATE);
        e.setAttribute(QStringLiteral("template"), idx.data(FULLPATHROLE).toString());
        idx = model->index(row, COLUMN_FILE);
        e.setAttribute(QStringLiteral("file"), idx.data().toString());
        idx = model->index(row, COLUMN_ADD);
        e.setAttribute(QStringLiteral("add"), idx.data(Qt::UserRole).toString());
    }
    ViewBase::saveContext(context);
}
