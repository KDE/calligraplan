/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "planmodels_export.h"
#include "ReportGeneratorOdt.h"

#include "kptproject.h"
#include "kptschedule.h"
#include "kptnodeitemmodel.h"
#include "kpttaskstatusmodel.h"
#include "kptitemmodelbase.h"
#include "kptnodechartmodel.h"
#include "kptschedulemodel.h"

#include <KoXmlReader.h>
#include <KoXmlWriter.h>
#include <KoOdf.h>
#include <KoOdfWriteStore.h>
#include <KoOdfReadStore.h>
#include <KoStore.h>
#include <KoStoreDevice.h>
#include <KoXmlNS.h>
#include <KoIcon.h>

#include <KLocalizedString>

#include <QIODevice>
#include <QMimeData>
#include <QMimeDatabase>
#include <QModelIndex>
#include <QByteArray>
#include <QDomDocument>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QVariantList>
#include <QPair>

#define HeaderRole Qt::UserRole + 543

const QLoggingCategory &PLANRG_LOG()
{
    static const QLoggingCategory category("calligra.plan.reportodt");
    return category;
}
#define dbgRG qCDebug(PLANRG_LOG)<<Q_FUNC_INFO

const QLoggingCategory &PLANRG_TMP_LOG()
{
    static const QLoggingCategory category("calligra.plan.reportodt.template");
    return category;
}
#define dbgRGTmp qCDebug(PLANRG_TMP_LOG)<<Q_FUNC_INFO

const QLoggingCategory &PLANRG_TABLE_LOG()
{
    static const QLoggingCategory category("calligra.plan.reportodt.table");
    return category;
}
#define dbgRGTable qCDebug(PLANRG_TABLE_LOG)<<Q_FUNC_INFO

const QLoggingCategory &PLANRG_CHART_LOG()
{
    static const QLoggingCategory category("calligra.plan.reportodt.chart");
    return category;
}
#define dbgRGChart qCDebug(PLANRG_CHART_LOG)<<Q_FUNC_INFO

const QLoggingCategory &PLANRG_VARIABLE_LOG()
{
    static const QLoggingCategory category("calligra.plan.reportodt.variable");
    return category;
}
#define dbgRGVariable qCDebug(PLANRG_VARIABLE_LOG)<<Q_FUNC_INFO

const QLoggingCategory &PLANRG_TR_LOG()
{
    static const QLoggingCategory category("calligra.plan.reportodt.tr");
    return category;
}
#define dbgRGTr qCDebug(PLANRG_TR_LOG)<<Q_FUNC_INFO

namespace KPlato
{

ChartItemModel *findChartItemModel(QSortFilterProxyModel &model)
{
    for (QAbstractProxyModel *p = &model; p; p = qobject_cast<QAbstractProxyModel*>(p->sourceModel())) {
        ChartItemModel *c = qobject_cast<ChartItemModel*>(p->sourceModel());
        if (c) {
            return c;
        }
    }
    return nullptr;
}

bool startsWith(const QStringList &keys, const QString &key)
{
    const auto lowerKey = key.toLower();
    for (const QString &k : keys) {
        if (lowerKey.startsWith(k)) {
            return true;
        }
    }
    return false;
}

QStringList trimmed(const QStringList &lst)
{
    QStringList rlst;
    for (const QString &s : lst) {
        QString r = s.trimmed();
        if (!r.isEmpty()) {
            rlst << r;

        }
    }
    return rlst;
}

bool addDataToFile(QByteArray &buffer, const QString &destName, KoStore &to)
{
    QBuffer file(&buffer);
    if (!file.open(QIODevice::ReadOnly)) {
        dbgRG<<"Failed to open buffer";
        return false;
    }

    if (!to.open(destName)) {
        dbgRG<<"Failed to open file for writing:"<<destName;
        return false;
    }

    QByteArray data;
    data.resize(8 * 1024);

    uint total = 0;
    for (int block = 0; (block = file.read(data.data(), data.size())) > 0; total += block) {
        data.resize(block);
        if (to.write(data) != block) {
            dbgRG<<"Failed to write block of data";
            return false;
        }
        data.resize(8*1024);
    }

    to.close();
    file.close();

    return true;
}

QAbstractItemModel *translationModel()
{
    QList<QPair<QString, QString> > names;
    names << QPair<QString, QString>(QStringLiteral("Project"), i18n("Project"))
    << QPair<QString, QString>(QStringLiteral("Manager"), i18n("Manager"))
    << QPair<QString, QString>(QStringLiteral("Schedule"), i18n("Schedule"))
    << QPair<QString, QString>(QStringLiteral("BCWS"), xi18nc("@title:column Budgeted Cost of Work Scheduled", "BCWS"))
    << QPair<QString, QString>(QStringLiteral("BCWP"), xi18nc("@title:column Budgeted Cost of Work Performed", "BCWP"))
    << QPair<QString, QString>(QStringLiteral("ACWP"), xi18nc("@title:column Actual Cost of Work Performed", "ACWP"))
    << QPair<QString, QString>(QStringLiteral("SPI"), xi18nc("@title:column Schedule Performance Index", "SPI"))
    << QPair<QString, QString>(QStringLiteral("CPI"), xi18nc("@title:column Cost Performance Index", "CPI"));

    QStandardItemModel *model = new QStandardItemModel(0, names.count());
    for (int column = 0; column < names.count(); ++column) {
        model->setHeaderData(column, Qt::Horizontal, names.at(column).first, HeaderRole);
        model->setHeaderData(column, Qt::Horizontal, names.at(column).second);
    }
    return model;
}

QAbstractItemModel *projectModel()
{
    QList<QPair<QString, QString> > names;
    names << QPair<QString, QString>(QStringLiteral("Name"), i18n("Name"))
    << QPair<QString, QString>(QStringLiteral("Manager"), i18n("Manager"))
    << QPair<QString, QString>(QStringLiteral("BCWS Cost"), i18nc("Cost based Budgeted Cost of Work Scheduled", "BCWS Cost"))
    << QPair<QString, QString>(QStringLiteral("BCWP Cost"), i18nc("Cost based Budgeted Cost of Work Performed", "BCWP Cost"))
    << QPair<QString, QString>(QStringLiteral("ACWP Cost"), i18nc("Cost based Actual Cost of Work Performed", "ACWP Cost"))
    << QPair<QString, QString>(QStringLiteral("SPI Cost"), i18nc("Cost based Schedule Performance Index", "SPI Cost"))
    << QPair<QString, QString>(QStringLiteral("CPI Cost"), i18nc("Cost based Cost Performance Index", "CPI Cost"))
    << QPair<QString, QString>(QStringLiteral("BCWS Effort"), i18nc("Effort based Budgeted Cost of Work Scheduled", "BCWS Effort"))
    << QPair<QString, QString>(QStringLiteral("BCWP Effort"), i18nc("Effort based Budgeted Cost of Work Performed", "BCWP Effort"))
    << QPair<QString, QString>(QStringLiteral("ACWP Effort"), i18nc("Effort based Actual Cost of Work Performed", "ACWP Effort"))
    << QPair<QString, QString>(QStringLiteral("SPI Effort"), i18nc("Effort based Schedule Performance Index", "SPI Effort"))
    << QPair<QString, QString>(QStringLiteral("CPI Effort"), i18nc("Effort based Cost Performance Index", "CPI Effort"));

    QStandardItemModel *model = new QStandardItemModel(0, names.count());
    for (int column = 0; column < names.count(); ++column) {
        model->setHeaderData(column, Qt::Horizontal, names.at(column).first, HeaderRole);
        model->setHeaderData(column, Qt::Horizontal, names.at(column).second);
    }
    return model;
}

void initProjectModel(QAbstractItemModel *model, Project *project, ScheduleManager *sm)
{
    if (model->rowCount() == 0) {
        model->insertRow(0);
    }
    QModelIndex idx = model->index(0, 0);
    model->setData(idx, project->name());
    idx = model->index(0, 1);
    model->setData(idx, project->leader());

    PerformanceDataCurrentDateModel m(nullptr);
    m.setProject(project);
    m.setScheduleManager(sm);
    m.setNodes(QList<Node*>() << project);

    int col = 2; // column of BCWS Cost
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 5; ++c) {
            idx = model->index(0, col++);
            QModelIndex cidx = m.index(r, c);
            model->setData(idx, cidx.data());
        }
    }
}

QAbstractItemModel *scheduleModel()
{
    ScheduleItemModel m;
    QStandardItemModel *model = new QStandardItemModel(0, m.columnCount());
    for (int c = 0; c < m.columnCount(); ++c) {
        model->setHeaderData(c, Qt::Horizontal, m.headerData(c, Qt::Horizontal));
        model->setHeaderData(c, Qt::Horizontal, m.headerData(c, Qt::Horizontal, Qt::EditRole), HeaderRole);
    }
    return model;
}

void initScheduleModel(QAbstractItemModel *model, Project *project, ScheduleManager *sm)
{
    ScheduleItemModel m;
    m.setProject(project);
    QModelIndex idx = m.index(sm);
    if (idx.isValid()) {
        if (model->rowCount() == 0) {
            model->insertRow(0);
        }
        for (QModelIndex i = idx; i.isValid(); i = i.sibling(i.row(), i.column() + 1)) {
            QModelIndex midx = model->index(0, i.column());
            model->setData(midx, i.data());
            dbgRGVariable<<model->headerData(midx.column(), Qt::Horizontal, HeaderRole).toString()<<'='<<i.data(Qt::UserRole+1).toString();
        }
    } else dbgRGVariable<<"Could not find schedule"<<sm;
    dbgRGVariable<<model->rowCount()<<model->columnCount();
}

QAbstractItemModel *ReportGeneratorOdt::projectsModel(ItemModelBase *base) const
{
    static_cast<NodeItemModel*>(base)->setShowProject(true);

    auto model = new QSortFilterProxyModel();
    model->setSourceModel(base);
    model->setFilterRole(Qt::EditRole);
    model->setFilterFixedString(QString::number(Node::Type_Project));
    model->setFilterKeyColumn(NodeModel::NodeType);
    return model;
}

//--------------------------------------
ReportGeneratorOdt::ReportGeneratorOdt()
    : ReportGenerator()
    , m_templateStore(nullptr)
{
    m_keys = QStringList() << QStringLiteral("table") << QStringLiteral("chart");
    m_variables = QStringList() << QStringLiteral("project") << QStringLiteral("schedule");

    m_basemodels
    << new NodeItemModel()
    << new TaskStatusItemModel()
    << new ChartItemModel()
    << new ScheduleItemModel()
    << new NodeItemModel();

    addDataModel(QStringLiteral("tasks"), m_basemodels.at(0), Qt::EditRole);
    addDataModel(QStringLiteral("taskstatus"), m_basemodels.at(1), Qt::EditRole);
    addDataModel(QStringLiteral("chart.project"), m_basemodels.at(2), Qt::EditRole);
    addDataModel(QStringLiteral("projects"), projectsModel(m_basemodels.at(4)), Qt::EditRole);
    addDataModel(QStringLiteral("schedules"), m_basemodels.at(3), Qt::EditRole);
    addDataModel(QStringLiteral("project"), projectModel(), HeaderRole);
    addDataModel(QStringLiteral("schedule"), scheduleModel(), HeaderRole);
    addDataModel(QStringLiteral("tr"), translationModel(), HeaderRole);

//     for (QAbstractItemModel *m : m_datamodels) {
//         const QString key = m_datamodels.key(m);
//         qInfo()<<key<<"columns:"<<m->columnCount();
//         for (int i = 0; i < m->columnCount(); ++i) {
//             qInfo()<<'\t'<<i<<m->headerData(i, Qt::Horizontal, m_headerrole.value(key)).toString();
//         }
//     }
}


ReportGeneratorOdt::~ReportGeneratorOdt()
{
    for (QAbstractItemModel *m : qAsConst(m_datamodels)) {
        if (!m_basemodels.contains(qobject_cast<ItemModelBase*>(m))) {
            delete m;
        }
    }
    qDeleteAll(m_basemodels);
    delete m_templateStore;
}

void ReportGeneratorOdt::addDataModel(const QString &name, QAbstractItemModel *model, int role)
{
    model->setObjectName(name);
    m_datamodels[name] = model;
    m_headerrole[name] = role;
}

bool ReportGeneratorOdt::open()
{
    m_lastError.clear();
    if (m_templateFile.isEmpty()) {
        m_lastError = i18n("Missing report template file");
        return false;
    }
    if (m_reportFile.isEmpty()) {
        m_lastError = i18n("Missing report result file");
        return false;
    }
    if (m_templateStore) {
        m_lastError = i18n("Report generator is already open");
        return false;
    }
    if (!QFile::exists(m_templateFile)) {
        m_lastError = i18n("Report template file does not exist");
        return false;
    }
    m_templateStore = KoStore::createStore(m_templateFile, KoStore::Read);
    if (!m_templateStore) {
        dbgRG<<"Failed to open store:"<<m_templateFile;
        m_lastError = i18n("Failed to open template file: %1", m_templateFile);
        return false;
    }
    for (ItemModelBase *m : qAsConst(m_basemodels)) {
        m->setProject(m_project);
        m->setScheduleManager(m_manager);
        if (qobject_cast<ChartItemModel*>(m)) {
            qobject_cast<ChartItemModel*>(m)->setNodes(QList<Node*>() << m_project);
            dbgRGChart<<"chart:"<<m_project<<m_manager<<"set nodes"<<m_project;
        }
    }
    initProjectModel(m_datamodels[QStringLiteral("project")], m_project, m_manager);
    initScheduleModel(m_datamodels[QStringLiteral("schedule")], m_project, m_manager);
    static_cast<ScheduleItemModel*>(m_datamodels[QStringLiteral("schedules")])->setProject(m_project);
    return true;
}

void ReportGeneratorOdt::close()
{
    delete m_templateStore;
    m_templateStore = nullptr;
}

bool ReportGeneratorOdt::createReport()
{
    if (!m_templateStore) {
        m_lastError = i18n("Report generator has not been correctly opened");
        return false;
    }
    // TODO get mimetype
    return createReportOdt();
}

bool ReportGeneratorOdt::createReportOdt()
{
    m_tags.clear();
    dbgRG<<"url:"<<m_templateStore->urlOfStore();

    KoOdfReadStore reader(m_templateStore);
    if (!reader.loadAndParse(m_lastError)) {
        dbgRG<<"Failed to loadAndParse:"<<m_lastError;
        return false;
    }
    // copy manifest file and store a list of file references
    KoStore *outStore = copyStore(reader, m_reportFile);
    if (!outStore) {
        dbgRG<<"Failed to copy template";
        return false;
    }
    dbgRG << '\n' << "---- treat main content.xml ----" << '\n';
    QBuffer buffer;
    KoXmlWriter *writer = createOasisXmlWriter(reader, &buffer, QStringLiteral("content.xml"), "office:document-content");

    if (!writer) {
        dbgRG<<"Failed to create content.xml writer";
        return false;
    }
    KoXmlDocument kodoc = reader.contentDoc();
    KoXmlElement parent = kodoc.documentElement();
    writeChildElements(*writer, parent);

    writer->endElement(); // office:document-content
    writer->endDocument();

    if (!outStore->addDataToFile(buffer.buffer(), QStringLiteral("content.xml"))) {
        dbgRG<<"Failed to open 'content.xml' for writing";
        m_lastError = i18n("Failed to write to store: %1", QStringLiteral("content.xml"));
        delete writer;
        delete outStore;
        return false;
    }
    buffer.close();

    if (m_manifestfiles.contains(QStringLiteral("styles.xml"))) {
        dbgRG << '\n' << "---- treat styles.xml (for master-page headers/footers) ----" << '\n';
        QBuffer buffer2;
        KoXmlWriter *styles = createOasisXmlWriter(reader, &buffer2, QStringLiteral("styles.xml"), "office:document-styles");
        if (!styles) {
            dbgRG<<"Failed to create styles.xml writer";
            return false;
        }
        KoXmlDocument stylesDoc;
        if (!reader.loadAndParse("styles.xml", stylesDoc, m_lastError)) {
            debugPlan<<"Failed to read styles.xml"<<m_lastError;
            delete writer;
            delete outStore;
            return false;
        }
        writeChildElements(*styles, stylesDoc.documentElement());
        styles->endElement(); // office:document-styles
        styles->endDocument();
        if (!outStore->addDataToFile(buffer2.buffer(), QStringLiteral("styles.xml"))) {
            dbgRG<<"Failed to open 'styles.xml' for writing";
            m_lastError = i18n("Failed to write to store: %1", QStringLiteral("styles.xml"));
            delete writer;
            delete outStore;
        }
        m_manifestfiles.removeAt(m_manifestfiles.indexOf(QStringLiteral("styles.xml")));
    }

    dbgRG << '\n' << "---- treat the embedded files ----" << '\n';
    treatEmbededObjects(reader, *outStore);
    dbgRG << '\n' << "---- copy rest of files ----" << '\n';
    for (int i = 0; i < m_manifestfiles.count(); ++i) {
        copyFile(*reader.store(), *outStore, m_manifestfiles.at(i));
    }
    if (!outStore->finalize()) {
        dbgRG<<"Failed to write store:"<<outStore->urlOfStore();
        m_lastError = i18n("Failed to write report file: %1", outStore->urlOfStore().path());
        delete writer;
        delete outStore;
        return false;
    }
    delete writer;
    delete outStore;
    dbgRG<<"finished";
    return true;
}

bool ReportGeneratorOdt::handleTextP(KoXmlWriter &writer, const KoXmlElement &textp)
{
    Q_UNUSED(writer);
    dbgRG<<"Check:"<<textp.text();
    // search for user fields
    KoXmlElement e;
    forEachElement(e, textp) {
        QString tag = e.prefix() + QLatin1Char(':') + e.localName();
        dbgRG<<"Tag:"<<tag;
        if (tag == QStringLiteral("text:user-field-get")) {
            QString field = e.attributeNS(KoXmlNS::text, "name");
            dbgRG<<"Check:"<<field<<m_userfields;
            if (m_userfields.contains(field) && startsWith(m_keys, field)) {
                m_sortedfields << field;
                m_userfields[field]->finish();
                dbgRG<<"Found:"<<tag<<field;
                return false;
            } else {
                dbgRG<<"Not found:"<<tag<<field<<m_userfields.keys()<<startsWith(m_keys, field);
            }
        } else {
            dbgRG<<"   skipping:"<<tag;
        }
    }
    return false;
}

void ReportGeneratorOdt::handleDrawFrame(KoXmlWriter &writer, const KoXmlElement &frame)
{
    Q_UNUSED(writer);
    dbgRGChart<<m_sortedfields;
    if (m_sortedfields.isEmpty()) {
        dbgRGChart<<"No fields";
        return;
    }
    KoXmlElement e = frame.namedItemNS(KoXmlNS::draw, QStringLiteral("object")).toElement();
    if (e.isNull()) {
        dbgRGChart<<"No 'object'";
        return;
    }
    QString dir = e.attributeNS(KoXmlNS::xlink, "href");
    if (dir.isEmpty()) {
        dbgRGChart<<"No dir";
        return;
    }
    if (m_sortedfields.first().startsWith(QStringLiteral("chart"))) {
        QString name = m_sortedfields.takeFirst();
        m_embededcharts[name] = dir;
        UserField *field = m_userfields[name];
        QString modelName = field->type + QLatin1Char('.') + field->dataName;
        field->setModel(dataModel(modelName), m_headerrole[modelName]);
        dbgRGChart<<"Found chart:"<<field;
        return;
    }
    if (m_sortedfields.first().startsWith(QStringLiteral("gantt"))) {
        m_embededgantts[m_sortedfields.takeFirst()] = dir;
        dbgRGChart<<"Found gantt";
        return;
    }
    dbgRGChart<<"No chart or gantt";

}

void ReportGeneratorOdt::treatText(KoXmlWriter &writer, const KoXmlText &text)
{
    dbgRG<<"  text node:"<<text.data();
    writer.addTextNode(text.data());
}

void ReportGeneratorOdt::treatTable(KoXmlWriter &writer, const KoXmlElement &tableElement)
{
    const QString name = m_sortedfields.value(0);
    if (name.startsWith(QStringLiteral("table"))) {
        dbgRGTable<<"   treat table expansion:"<<name;
        m_sortedfields.takeFirst();
        m_activefields << name;
        UserField *field = m_userfields[name];
        field->seqNr = USERFIELD_NONE;
        dbgRGTable<<field;
        writer.startElement("table:table");
        writeElementAttributes(writer, tableElement);
        writeChildElements(writer, tableElement);
        writer.endElement();
        m_activefields.removeLast();
    } else {
        dbgRGTable<<"   just a table";
        writer.startElement("table:table");
        writeElementAttributes(writer, tableElement);
        writeChildElements(writer, tableElement);
        writer.endElement();
    }
}

bool ReportGeneratorOdt::treatTableHeaderRows(KoXmlWriter &writer, const KoXmlElement &headerRowElement)
{
    Q_UNUSED(writer);
    Q_UNUSED(headerRowElement);

    if (m_activefields.isEmpty() || m_userfields[m_activefields.last()]->type != QStringLiteral("table")) {
        return false;
    }
    dbgRGTable;
    UserField *field = m_userfields[m_activefields.last()];
    field->seqNr = USERFIELD_HEADER; // we are in header row
    return false;
}

bool ReportGeneratorOdt::treatTableRow(KoXmlWriter &writer, const KoXmlElement &rowElement)
{
    if (m_activefields.isEmpty() || m_userfields[m_activefields.last()]->type != QStringLiteral("table")) {
        return false;
    }
    UserField *field = m_userfields[m_activefields.last()];
    dbgRGTable<<field->seqNr;
    if (field->seqNr == USERFIELD_NONE) {
        // there is no header row, so start with data rows directly
        field->seqNr = USERFIELD_DATA;
    }
    if (field->seqNr == USERFIELD_HEADER) {
        // header row
        writer.startElement("table:table-row");
        writeElementAttributes(writer, rowElement);
        writeChildElements(writer, rowElement);
        writer.endElement();
        field->seqNr = USERFIELD_DATA; // next is first row
    } else {
        dbgRGTable<<" add rows:"<<field->rowCount();
        for (field->begin(); field->next();) {
            writer.startElement("table:table-row");
            writeElementAttributes(writer, rowElement);
            writeChildElements(writer, rowElement);
            writer.endElement();
        }
        field->finish();
    }
    return true;
}

ReportGeneratorOdt::UserField *ReportGeneratorOdt::findUserField(const KoXmlElement &decl) const
{
    UserField *field = nullptr;
    QString name = decl.attributeNS(KoXmlNS::text, "name"); // eg: table1 or table1.Type or project.name or tr.bcws

    field = m_userfields.value(name); // if Variable or Translation
    if (!field) {
        QStringList lst = name.split(QLatin1Char('.'));
        QMap<QString, UserField*>::const_iterator it;
        for (it = m_userfields.constBegin(); it != m_userfields.constEnd(); ++it) {
            if (lst.first().startsWith(it.key())) {
                field = it.value();
                break;
            }
        }
    }
    return field;
}

void ReportGeneratorOdt::treatUserFieldGet(KoXmlWriter &writer, const KoXmlElement &e)
{
    dbgRG<<e.text();
    UserField *field = findUserField(e);
    if (field) {
        writer.startElement("text:span");
        if (field->variant() == UserField::Rows) {
            QString name = e.attributeNS(KoXmlNS::text, "name"); // eg: table1.type
            QString value = e.text(); // eg: type or name
            QString data = field->data(value);
            writer.addTextNode(data);
            dbgRGTable<<"rows:"<<name<<value<<'='<<data;
        } else if (field->variant() == UserField::Header) {
            QString name = e.attributeNS(KoXmlNS::text, "name"); // eg: table1.type
            QString value = e.text(); // eg: type or BCWS Cost
            QString data = field->headerData(value);
            writer.addTextNode(data);
            dbgRGTable<<"header row:"<<name<<value<<'='<<data;
        } else if (field->variant() == UserField::Variable) {
            QString name = e.attributeNS(KoXmlNS::text, "name"); // eg: project.name
            dbgRGVariable<<"variable:"<<name<<field->columns.value(0)<<field->data(field->columns.value(0));
            writer.addTextNode(field->data(field->columns.value(0))); // a variable has only one column
        } else if (field->variant() == UserField::Translation) {
            QString name = e.attributeNS(KoXmlNS::text, "name"); // eg: tr.bcws
            QString data = i18n(field->dataName.toLatin1().constData());
            if (field->headerNames.contains(field->dataName)) {
                data = field->data(field->dataName);
            }
            dbgRGTr<<"translation:"<<name<<field->dataName<<'='<<data;
            writer.addTextNode(data);
        }
        writer.endElement();
    } else {
        // this is not a report field, just copy verbatim
        writer.startElement("text:user-field-get");
        writeElementAttributes(writer, e);
        dbgRG<<"no active user field";
        writer.endElement();
    }
}

void ReportGeneratorOdt::handleUserFieldDecls(KoXmlWriter &writer, const KoXmlElement &decls)
{
    // load declarations
    for (KoXmlNode node = decls.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (!node.isElement()) {
            continue;
        }
        KoXmlElement e = node.toElement();
        QByteArray tag = (QString(e.prefix() + QLatin1Char(':')) + e.localName()).toUtf8();
        if (tag != "text:user-field-decl") {
            continue;
        }
        QString name = e.attributeNS(KoXmlNS::text, "name");
        if (name.isEmpty()) {
            dbgRG<<"  User field name is empty";
            continue;
        }
        QString value = e.attributeNS(KoXmlNS::office, "string-value");
        if (value.isEmpty()) {
            dbgRG<<"  User field value is empty:"<<name;
        }
        QStringList tags = name.split(QLatin1Char('.'));
        dbgRG<<"  decl="<<name<<tags;
        if (tags.first() == QStringLiteral("tr")) {
            UserField *field = new UserField();
            field->name = name;
            m_userfields[field->name] = field;

            field->dataName = value;
            field->seqNr = USERFIELD_TRANSLATION;
            field->columns << value;
            field->setModel(dataModel(QStringLiteral("tr")), m_headerrole[QStringLiteral("tr")]);
            dbgRGTr<<"    added translation"<<field->name<<field->dataName;
        } else if (m_variables.contains(tags.first())) {
            Q_ASSERT(tags.count() >= 2);
            Q_ASSERT(!m_userfields.contains(tags.first()));

            UserField *field = new UserField();
            field->name = name;
            m_userfields[field->name] = field;

            field->dataName = tags.first();
            field->seqNr = USERFIELD_VARIABLE;
            QStringList vl = value.split(QLatin1Char(';'));
            if (!vl.isEmpty()) {
                field->columns << vl.takeFirst();
                field->properties = vl;
            }
            field->setModel(dataModel(field->dataName), m_headerrole[field->dataName]);
            dbgRGVariable<<"    added variable"<<field->name<<field->columns<<field->properties;
        } else {
            for (const QString &k : qAsConst(m_keys)) {
                const QString vname = tags.first();
                if (!vname.startsWith(k)) {
                    continue;
                }
                if (tags.count() == 1) {
                    // this is the main definition (eg: name: "table1", value: "project; values=bcws effort, bcwp effort, acwp effort;")
                    if (!m_userfields.contains(vname)) {
                        m_userfields[vname] = new UserField();
                    }
                    UserField *field = m_userfields[vname];
                    field->name = vname;
                    field->type = k;
                    QStringList vl = trimmed(value.toLower().split(QLatin1Char(';'), Qt::SkipEmptyParts));
                    field->dataName = vl.takeFirst();
                    field->properties += vl;
                    field->setModel(dataModel(field->dataName), m_headerrole[field->dataName]);
                    if (k == QStringLiteral("chart")) {
                        dbgRGChart<<"  "<<"added tag:"<<field<<field->properties;
                    } else {
                        dbgRG<<"  "<<"added tag:"<<field<<field->properties;
                    }
                } else {
                    // this is the fields column definitions (eg: name: "table1.type" value: "<not used>")
                    if (!m_userfields.contains(vname)) {
                        m_userfields[vname] = new UserField();
                        UserField *field = m_userfields[vname];
                        field->name = vname;
                    }
                    UserField *field = m_userfields[vname];
                    field->columns << value.trimmed().toLower();
                    Q_ASSERT(field->name == vname);
                    dbgRG<<"  "<<"added column:"<<field->name<<field->columns;
                }
            }
        }
    }
    writer.startElement("text:user-field-decls");
    writeElementAttributes(writer, decls);
    writeChartElements(writer, decls);
    writer.endElement();

}

void ReportGeneratorOdt::writeElementAttributes(KoXmlWriter &writer, const KoXmlElement &element, const QStringList &exclude)
{
    const auto elements = element.attributeFullNames();
    for (const QPair<QString, QString> &a : elements) {
        QString prefix = QLatin1String(KoXmlNS::nsURI2NS(a.first));
        if (prefix.isEmpty()) {
            dbgRG<<"  Skipping unknown namespace:"<<a.first<<a.second;
            continue;
        }
        QString attr = QString(prefix + QLatin1Char(':')) + a.second;
        if (exclude.contains(attr)) {
            continue;
        }
        m_tags << attr.toUtf8(); // save
//         dbgRGa.first<<a.second<<"->"<<attr;
        //dbgRG<<" : "<<m_tags.last().constData()<<'='<<e.attributeNS(a.first, a.second);
        writer.addAttribute(m_tags.last().constData(), element.attributeNS(a.first, a.second));
    }
}

void ReportGeneratorOdt::writeChildElements(KoXmlWriter &writer, const KoXmlElement &parent)
{
    //dbgRG<<parent.prefix()<<QLatin1Char(':')<<parent.localName();
    for (KoXmlNode node = parent.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (node.isText()) {
            writer.addTextNode(node.toText().data());
            continue;
        }
//         dbgRG<<node.prefix()<<node.localName()<<node.nodeType();
        KoXmlElement e = node.toElement();
        if (e.isNull()) {
            continue;
        }
//         dbgRG<<"  "<<e.prefix()<<e.localName() << e.attributeFullNames();
        QByteArray tag = (QString(e.prefix() + QLatin1Char(':')) + e.localName()).toUtf8();
        m_tags << tag; // make sure tags survives until we are finished
        if (tag == "text:user-field-decls") {
            handleUserFieldDecls(writer, e);
            continue;
        }
        if (tag == "text:user-field-decl") {
            dbgRG<<"Should we get here?"<<tag;
            continue; // treated by handleUserFieldDecls()
        }
        if (tag == "table:table") {
            treatTable(writer, e);
            continue;
        }
        if (tag == "table:table-header-rows") {
            if (treatTableHeaderRows(writer, e)) {
                continue; // header rows treats its own children
            }
        }
        if (tag == "table:table-row") {
            if (treatTableRow(writer, e)) {
                continue;
            }
        }
        if (tag == "text:user-field-get") {
            treatUserFieldGet(writer, e);
            continue;
        }
        if (tag == "text:p") {
            // check for/handle keywords
            if (handleTextP(writer, e)) {
                dbgRG<<"Skip:"<<tag;
                continue;
            }
        }
        if (tag == "draw:frame") {
            handleDrawFrame(writer, e);
        }
        writer.startElement(tag.constData());
        writeElementAttributes(writer, e);
        writeChildElements(writer, e);
        writer.endElement();
    }
}

bool ReportGeneratorOdt::copyFile(KoStore &from, KoStore &to, const char *file)
{
    return copyFile(from, to, QString::fromLatin1(file));
}
bool ReportGeneratorOdt::copyFile(KoStore &from, KoStore &to, const QString &file)
{
    QByteArray data;
    bool ok = from.extractFile(file, data);
    if (!ok) {
        dbgRG<<"Failed to extract:"<<file;
        m_lastError = i18n("Failed to extract file: %1", file);
    }
    if (ok) {
        ok = addDataToFile(data, file, to);
        if (!ok) {
            dbgRG<<"Failed to add file:"<<file;
            m_lastError = i18n("Failed to add file to store: %1", file);
        }
    }
    if (ok) {
        dbgRG<<"Added:"<<file;
    }
    return ok;
}

KoStore *ReportGeneratorOdt::copyStore(KoOdfReadStore &reader, const QString &outfile)
{
    if (!reader.store()->hasFile("META-INF/manifest.xml")) {
        dbgRG<<"No manifest file";
        return nullptr;
    }
    KoXmlDocument manifest;
    if (!reader.loadAndParse("META-INF/manifest.xml", manifest, m_lastError)) {
        dbgRG<<"Failed to read manifest:"<<m_lastError;
        return nullptr;
    }
    QUrl url = QUrl::fromUserInput(outfile);
    if (!url.isLocalFile()) {
        // FIXME: KoStore only handles local files
        dbgRG<<"KoStore only handles local files:"<<url;
        m_lastError = i18n("Report generator can only generate local files");
        return nullptr;
    }
    const auto path = url.toLocalFile();
    KoStore *out = KoStore::createStore(path, KoStore::Write);
    if (!out) {
        dbgRG<<"Failed to create store";
        m_lastError = i18n("Failed to open report file: %1", url.path());
        return nullptr;
    }
    // This should go first, see OpenDocument v1.2 part 3: Packages
    if (reader.store()->hasFile("mimetype")) {
        if (!copyFile(*reader.store(), *out, "mimetype")) {
            if (!m_lastError.isEmpty()) {
                m_lastError.prepend(QLatin1String("\n\n"));
            }
            m_lastError.prepend(i18n("Failed to load mimetype file"));
            delete out;
            return nullptr;
        }
    }
    if (!copyFile(*reader.store(), *out, "META-INF/manifest.xml")) {
        if (!m_lastError.isEmpty()) {
            m_lastError.prepend(QLatin1String("\n\n"));
        }
        m_lastError.prepend(i18n("Failed to write manifest file"));
        delete out;
        return nullptr;
    }

    KoXmlElement e;
    forEachElement(e, manifest.documentElement()) {
        dbgRG<<e.tagName()<<e.attributeNames();
        QString file;
        if (e.hasAttribute("full-path")) {
            file = e.attribute("full-path");
            if (file.isEmpty() || file == QStringLiteral("content.xml") || file.endsWith(QStringLiteral("/"))) {
                file.clear();
                continue;
            }
        }
        if (!file.isEmpty()) {
            m_manifestfiles << file;
        }
    }
    return out;
}

KoXmlWriter *ReportGeneratorOdt::createOasisXmlWriter(KoOdfReadStore &reader, QBuffer *buffer, const QString fileName, const char *rootElementName)
{
    if (!reader.store()) {
        m_lastError = i18n("No store backend");
        dbgRG<<"No store";
        return nullptr;
    }
    dbgRGTmp<<fileName<<rootElementName<<"has file:"<<reader.store()->hasFile(fileName);
    if (reader.store()->isOpen()) {
        reader.store()->close();
    }
    if (!reader.store()->open(fileName)) {
        dbgRG << "Entry " << fileName << " not found!";
        m_lastError = xi18nc("@info", "Failed to open file <filename>%1</filename> from store.", fileName);
        return nullptr;
    }
    if (!reader.store()->device()->isOpen()) {
        reader.store()->device()->open(QIODevice::ReadOnly);
    }
    KoXmlWriter *writer = nullptr;
    QXmlStreamReader xml(reader.store()->device());
    xml.setNamespaceProcessing(true);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement && !xml.namespaceDeclarations().isEmpty()) {
            writer = new KoXmlWriter(buffer);
            writer->startDocument(rootElementName);
            writer->startElement(rootElementName);
            writer->addAttribute("xmlns:calligra", KoXmlNS::calligra);

            // Note: windows needs this type of iteration
            QXmlStreamNamespaceDeclarations dcl = xml.namespaceDeclarations();
            for (int ns = 0; ns < dcl.count(); ++ns) {
                dbgRGTmp<<"add namespace:"<<(QStringLiteral("xmlns:") + dcl[ns].prefix())<<dcl[ns].namespaceUri();
                writer->addAttribute((QStringLiteral("xmlns:") + dcl[ns].prefix()).toLatin1().constData(), dcl[ns].namespaceUri().toUtf8());
            }
            QXmlStreamAttributes attr = xml.attributes();
            for (int a = 0; a < attr.count(); ++a) {
                dbgRGTmp<<"add attribute:"<<attr[a].qualifiedName().toString()<<attr[a].value().toString();
                writer->addAttribute(attr[a].qualifiedName().toLatin1().constData(), attr[a].value().toUtf8());
            }
        }
    }
    reader.store()->close();
    if (!writer) {
        dbgRG<<"Failed to find a start element with namespace declarations in"<<fileName;
        m_lastError = xi18nc("@info", "Missing namespace declarations:<nl/><filename>%1</filename>", fileName);
    }
    return writer;
}

void ReportGeneratorOdt::treatEmbededObjects(KoOdfReadStore &reader, KoStore &outStore)
{
    dbgRGChart;
    {QMap<QString, QString>::const_iterator it;
    for (it = m_embededcharts.constBegin(); it != m_embededcharts.constEnd(); ++it) {
        treatChart(reader, outStore, it.key(), it.value());
    }}
    {QMap<QString, QString>::const_iterator it;
    for (it = m_embededgantts.constBegin(); it != m_embededgantts.constEnd(); ++it) {
        treatGantt(reader, outStore, it.key(), it.value());
    }}
}

void ReportGeneratorOdt::treatChart(KoOdfReadStore &reader, KoStore &outStore, const QString &name, const QString &dir)
{
    dbgRGChart<<name<<dir;
    if (!m_userfields.contains(name)) {
        dbgRGChart<<"No user field with name:"<<name;
        return;
    }
    QString file = dir + QStringLiteral("/content.xml");
    file = file.remove(QStringLiteral("./"));
    dbgRGChart<<file<<m_manifestfiles;
    QString err;
    KoXmlDocument doc;
    if (!reader.loadAndParse(file, doc, err)) {
        dbgRGChart<<err;
        return;
    }
    m_activefields << name;
    UserField *field = m_userfields[name];
    if (field->dataName == QStringLiteral("project")) {
        ChartItemModel *m = findChartItemModel(field->model);
        if (m) {
            m->setNodes(QList<Node*>() << m_project);
        }
        if (field->properties.isEmpty()) {
            // default: take all
            for (const QString &c : qAsConst(field->headerNames)) {
                field->columns << c;
            }
        } else {
            QStringList values;
            for (const QString &p : qAsConst(field->properties)) {
                if (p.startsWith(QStringLiteral("values"))) {
                    QStringList vl = p.split(QStringLiteral("="));
                    Q_ASSERT(vl.count() > 1);
                    Q_ASSERT(vl.at(0) == QStringLiteral("values"));
                    const auto strings = vl.at(1).split(QLatin1Char(','));
                    for (const auto &v : strings) {
                        values << v.toLower().trimmed();
                    }
                }
            }
            field->columns = values;
        }
        dbgRGChart<<field;

    }
    QBuffer buffer;
    KoXmlWriter *writer = createOasisXmlWriter(reader, &buffer, file, "office:document-content");

    writeChartElements(*writer, doc.documentElement());

    writer->endElement(); // office:document-content
    writer->endDocument();

    dbgRGChart<<writer->toString();
    if (!outStore.addDataToFile(buffer.buffer(), file)) {
        dbgRGChart<<"Failed to open"<<file<<"for writing";
        m_lastError = i18n("Failed to write to store: %1", file);
    }

    m_manifestfiles.removeAt(m_manifestfiles.indexOf(file));
    dbgRGChart<<m_manifestfiles;
}

void ReportGeneratorOdt::treatGantt(KoOdfReadStore &reader, KoStore &outStore, const QString &name, const QString &file)
{
    Q_UNUSED(reader);
    Q_UNUSED(outStore);
    Q_UNUSED(name);
    Q_UNUSED(file);
    dbgRGChart;
}

void ReportGeneratorOdt::writeChartElements(KoXmlWriter &writer, const KoXmlElement &parent)
{
    dbgRGChart<<"writeChartElements: parent="<<parent.prefix()<<','<<parent.localName();
    for (KoXmlNode node = parent.firstChild(); !node.isNull(); node = node.nextSibling()) {
        if (node.isText()) {
            writer.addTextNode(node.toText().data());
            continue;
        }
        KoXmlElement e = node.toElement();
        if (e.isNull()) {
            continue;
        }
        //dbgRGChart<<"  "<<e.namespaceURI()<<e.prefix()<<e.localName() << e.attributeFullNames();
        if (QLatin1String(KoXmlNS::nsURI2NS(e.namespaceURI())).isEmpty()) {
            continue; // skip unknown namespace
        }
        QByteArray tag = (e.prefix() + QLatin1Char(':') + e.localName()).toUtf8();
        m_tags << tag; // make sure tags survives until we are finished
        //dbgRGChart<<"  handle element:"<<tag;

        if (tag == "chart:plot-area") {
            UserField *field = m_userfields[m_activefields.last()];
            field->hasLabels = e.attributeNS(KoXmlNS::chart, "data-source-has-labels");

            writer.startElement(tag.constData());
            writeElementAttributes(writer, e);
            writeChartElements(writer, e);
            writer.endElement();
        } else if (tag == "chart:categories") {
            // assume this is x-axis
            UserField *field = m_userfields[m_activefields.last()];
            int columns = field->columnCount();
            if (columns > 0) {
                writer.startElement(tag.constData());
                writeElementAttributes(writer, e, QStringList() << QStringLiteral("table:cell-range-address"));

                int startRow = 1;
                if (field->hasLabels == QStringLiteral("both") || field->hasLabels == QStringLiteral("primary-x")) {
                    ++startRow;
                }
                QString end = QStringLiteral("local-table.$A$%1:$A$%2").arg(startRow).arg(startRow+field->rowCount()-1);
                writer.addAttribute("table:cell-range-address", end);

                writeChartElements(writer, e);
                writer.endElement();
            }
        } else if (tag == "chart:series") {
            UserField *field = m_userfields[m_activefields.last()];
            int columns = field->columnCount();
            if (columns > 0 && field->serieNr < columns) {
                int startRow = 1;
                if (field->hasLabels == QStringLiteral("both") || field->hasLabels == QStringLiteral("primary-x")) {
                    ++startRow;
                }
                char startColumn = 'A';
                if (field->hasLabels == QStringLiteral("both") || field->hasLabels == QStringLiteral("primary-y")) {
                    ++startColumn;
                }
                writer.startElement(tag.constData());
                writeElementAttributes(writer, e, QStringList() << QStringLiteral("chart:values-cell-range-address")<<QStringLiteral("chart:label-cell-address"));

                QChar c(startColumn + field->serieNr);

                QString lab = QStringLiteral("local-table.$%1$1").arg(c);
                writer.addAttribute("chart:values-cell-address", lab);

                QString end = QStringLiteral("local-table.$%1$%2:$%1$%3").arg(c).arg(startRow).arg(startRow+field->rowCount()-1);
                writer.addAttribute("chart:values-cell-range-address", end);

                writer.startElement("chart:data-point");
                writer.addAttribute("chart:repeated", field->rowCount());
                writer.endElement();

                writer.endElement();

                ++field->serieNr;
            }
        } else if (tag == "chart:data-point") {
            // do nothing handled under chart:series
        } else if (tag == "table:table-header-columns") {
            writer.startElement(tag.constData());
            writer.startElement("table:table-column"); // just an empty tag
            writer.endElement();
            writer.endElement();
        } else if (tag == "table:table-columns") {
            writer.startElement(tag.constData());
            writeChartElements(writer, e);
            writer.endElement();
        } else if (tag == "table:table-column") {
            int columns = m_userfields[m_activefields.last()]->columnCount();
            writer.startElement(tag.constData());
            writer.addAttribute("table:number-columns-repeated", columns);
            writer.endElement();
        } else if (tag == "table:table-header-rows") {
            writer.startElement(tag.constData());
            writer.startElement("table:table-row");
            // first column not used, just an empty cell
            writer.startElement("table:table-cell");
            writer.startElement("text:p");
            writer.endElement();
            writer.endElement();
            // write legends
            UserField *field = m_userfields[m_activefields.last()];
            for (const QString &name : qAsConst(field->columns)) {
                QString value = field->headerData(name);
                writer.startElement("table:table-cell");
                writer.addAttribute("office:value-type", "string");
                writer.startElement("text:p");
                writer.addTextNode(value);
                writer.endElement();
                writer.endElement();
            }
            writer.endElement();
            writer.endElement();
        } else if (tag == "table:table-rows") {
            writer.startElement(tag.constData());
            UserField *field = m_userfields[m_activefields.last()];
            int columns = field->columnCount();
            if (columns > 0) {
                int rows = field->model.rowCount();
                for (int r = 0; r < rows; ++r) {
                    writer.startElement("table:table-row");
                    // first x-axis labels
                    QDate date = QDate(field->model.headerData(r, Qt::Vertical, Qt::EditRole).toDate());
                    // NOTE: 1899-12-30 is the reference date, but could it depend on style?
                    int day = QDate(1899, 12, 30).daysTo(date);
                    writer.startElement("table:table-cell");
                    writer.addAttribute("office:value", day);
                    writer.addAttribute("office:value-type", "float");
                    writer.startElement("text:p");
                    writer.addTextNode(QString::number(day));
                    writer.endElement();
                    writer.endElement();
                    // then the data
                    for (const QString &name : qAsConst(field->columns)) {
                        QVariant value = field->model.index(r, field->column(name)).data();
                        writer.startElement("table:table-cell");
                        writer.addAttribute("office:value-type", "float");
                        writer.addAttribute("office:value", value.toDouble());
                        writer.startElement("text:p");
                        writer.addTextNode(QString::number(value.toDouble()));
                        writer.endElement();
                        writer.endElement();
                    }
                    writer.endElement();
                }
            }
            writer.endElement();
        } else if (tag == "chart:legend") {
            writer.startElement(tag.constData());
            if (!e.hasAttributeNS(KoXmlNS::chart, "legend-align") && e.hasAttributeNS(KoXmlNS::chart, "legend-position")) {
                // lowriter does not specify this attribute
                // If legend-position is start, end, top or bottom
                // we need to have legend-align == center so that words
                // repositions the legend correctly
                QStringList lst = QStringList() << QStringLiteral("start") << QStringLiteral("end") << QStringLiteral("top") << QStringLiteral("bottom");
                if (lst.contains(e.attributeNS(KoXmlNS::chart, "legend-position"))) {
                    writer.addAttribute("chart:legend-align", "center");
                    dbgRGChart<<"adding legend-align";
                }
            }
            writeElementAttributes(writer, e);
            writeChartElements(writer, e);
            writer.endElement();
        } else {
            writer.startElement(tag.constData());
            writeElementAttributes(writer, e);
            writeChartElements(writer, e);
            writer.endElement();
        }
    }
}

void ReportGeneratorOdt::listChildNodes(const QDomNode &parent)
{
    QDomNodeList lst = parent.childNodes();
    for (int i = 0; i < lst.count(); ++i) {
        if (lst.at(i).isElement()) {
            QDomElement e = lst.at(i).toElement();
            dbgRG<<"Element:"<<e.tagName()<<"value="<<e.text();
            QDomNamedNodeMap map = e.attributes();
            for (int j = 0; j < map.count(); ++j) {
                QDomAttr attr = map.item(j).toAttr();
                dbgRG<<"  "<<attr.name()<<attr.value();
            }
        }
        listChildNodes(lst.at(i));
    }
}

QAbstractItemModel *ReportGeneratorOdt::dataModel(const QString &name) const
{
    dbgRG<<name<<m_datamodels;
    return m_datamodels.value(name);
}

void ReportGeneratorOdt::UserField::setModel(QAbstractItemModel *model, int role)
{
    headerNames.clear();
    this->model.setSourceModel(model);
    for (int c = 0; c < this->model.columnCount(); ++c) {
        headerNames << this->model.headerData(c, Qt::Horizontal, role).toString().toLower();
    }
}

int ReportGeneratorOdt::UserField::column(const QString &columnName) const
{
    QStringList l = columnName.split(QLatin1Char('.'));
    int c = l.isEmpty() ? -1 : headerNames.indexOf(l.last().toLower());
    dbgRGTable<<"  column:"<<columnName<<'='<<c;
    return c;
}

void ReportGeneratorOdt::UserField::begin()
{
    currentIndex = QModelIndex();
}

bool ReportGeneratorOdt::UserField::next()
{
    if (model.hasChildren(currentIndex)) {
        currentIndex = model.index(0, column(columns.value(0)), currentIndex);
    } else {
        do {
            QModelIndex idx = currentIndex.sibling(currentIndex.row()+1, column(columns.value(0)));
            if (idx.isValid()) {
                currentIndex = idx;
                break;
            }
            currentIndex = model.parent(currentIndex);
        } while (currentIndex.isValid());
    }
    return currentIndex.isValid();
}

void ReportGeneratorOdt::UserField::finish()
{
    currentIndex = QModelIndex();
    if (seqNr > USERFIELD_NONE) {
        seqNr = USERFIELD_NONE;
    }
}

QString ReportGeneratorOdt::UserField::data(const QString &column) const
{
    QModelIndex idx;
    if (currentIndex.isValid()) {
        idx = model.index(currentIndex.row(), this->column(column), currentIndex.parent());
    } else {
        idx = model.index(0, this->column(column));
    }
    QString s = QStringLiteral("No data");
    if (idx.isValid()) {
        s = model.data(idx).toString();
    }
    dbgRG<<column<<'='<<s;
    return s;
}

QString ReportGeneratorOdt::UserField::headerData(const QString &columnName) const
{
    return model.headerData(column(columnName), Qt::Horizontal).toString();
}

int ReportGeneratorOdt::UserField::rowCount()
{
    int rows = 0;
    for (begin(); next(); ++rows) {}
    return rows;
}

int ReportGeneratorOdt::UserField::variant() const
{
    int t = Rows;
    switch (seqNr) {
        case USERFIELD_TRANSLATION: t = Translation; break;
        case USERFIELD_VARIABLE: t = Variable; break;
        case USERFIELD_NONE: t = None; break;
        case USERFIELD_HEADER: t = Header; break;
        default: break;
    }
    return t;
}

QDebug operator<<(QDebug dbg, ReportGeneratorOdt::UserField *f)
{
    if (f) {
        return operator<<(dbg, *f);
    }
    dbg << "UserField(0x0)";
    return dbg;
}
QDebug operator<<(QDebug dbg, ReportGeneratorOdt::UserField &f)
{
    dbg.nospace() << "UserField[";
    switch (f.variant()) {
        case ReportGeneratorOdt::UserField::Header: dbg << QStringLiteral("H: ") + f.name + QLatin1Char('.') + f.dataName; break;
        case ReportGeneratorOdt::UserField::Rows: dbg << QStringLiteral("R: ") + f.name + QLatin1Char('.') + f.dataName << "seqNr: " << f.seqNr; break;
        case ReportGeneratorOdt::UserField::Variable: dbg << QStringLiteral("V: ") + f.name; break;
        case ReportGeneratorOdt::UserField::Translation: dbg << QStringLiteral("T: ") + f.name; break;
        default: dbg << QStringLiteral("U: ") + f.name; break;
    }
//     if (!f.columns.isEmpty()) {
//         dbg << '\n' <<"Columns: " << f.columns;
//     }
//     if (!f.headerNames.isEmpty()) {
//         dbg << '\n' << "Headers: " << f.headerNames;
//     }
//     if (!f.columns.isEmpty() || !f.headerNames.isEmpty()) {
//         dbg << '\n';
//     }
    dbg << ']';
    return dbg.space();
}

} //namespace KPlato
