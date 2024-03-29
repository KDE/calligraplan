/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef REPORTGENERATORODT_H
#define REPORTGENERATORODT_H

#include "ReportGenerator.h"

#include <QModelIndexList>

#include <KoXmlReaderForward.h>

#include <QDomDocument>
#include <QSortFilterProxyModel>

class QIODevice;
class QString;
class QBuffer;

class KoXmlWriter;
class KoStore;
class KoOdfWriteStore;
class KoOdfReadStore;

#define USERFIELD_TRANSLATION -3
#define USERFIELD_VARIABLE -2
#define USERFIELD_NONE -1
#define USERFIELD_HEADER 0
#define USERFIELD_DATA 1

namespace KPlato
{

class Project;
class ScheduleManager;
class ItemModelBase;


class PLANMODELS_EXPORT ReportGeneratorOdt : public ReportGenerator
{
public:
    explicit ReportGeneratorOdt();
    ~ReportGeneratorOdt() override;

    bool createReport() override;

protected:
    bool initiateInternal() override;
    bool createReportOdt();
    bool handleTextP(KoXmlWriter &writer, const KoXmlElement &textp);
    void handleDrawFrame(KoXmlWriter &writer, const KoXmlElement &frame);
    void treatText(KoXmlWriter &writer, const KoXmlText &text);
    void treatTable(KoXmlWriter &writer, const KoXmlElement &tableElement);
    bool treatTableHeaderRows(KoXmlWriter &writer, const KoXmlElement &headerRowElement);
    bool treatTableRow(KoXmlWriter &writer, const KoXmlElement &rowElement);
    void treatUserFieldGet(KoXmlWriter &writer, const KoXmlElement &e);
    void writeElementAttributes(KoXmlWriter &writer, const KoXmlElement &element, const QStringList &exclude = QStringList());
    void writeChildElements(KoXmlWriter &writer, const KoXmlElement &parent);
    bool copyFile(KoStore &from, KoStore &to, const QString &file);
    bool copyFile(KoStore &from, KoStore &to, const char *file);
    KoStore *copyStore(KoOdfReadStore &reader, const QString &outfile);
    KoXmlWriter *createOasisXmlWriter(KoOdfReadStore &reader, QBuffer *buffer, const QString fileName, const char *rootElementName);

    void treatEmbededObjects(KoOdfReadStore &reader, KoStore &outStore);
    void treatChart(KoOdfReadStore &reader, KoStore &outStore, const QString &name, const QString &file);
    void treatGantt(KoOdfReadStore &reader, KoStore &outStore, const QString &name, const QString &file);
    void writeChartElements(KoXmlWriter &writer, const KoXmlElement &parent);

    void listChildNodes(const QDomNode &parent);

    void handleUserFieldDecls(KoXmlWriter &writer, const KoXmlElement &decls);
    QAbstractItemModel *dataModel(const QString &name) const;

    QAbstractItemModel *projectsModel(ItemModelBase *base) const;

public:
    class UserField {
    public:
        enum Variant { None, Header, Rows, Variable, Translation };
        UserField() : seqNr(USERFIELD_NONE), serieNr(0) {}
        int variant() const;
        void begin();
        bool next();
        void finish();
        QString headerData(const QString &columnName) const;
        QString data(const QString &columnName) const;

        int rowCount();
        int columnCount() const {return columns.count();}
        void setModel(QAbstractItemModel *model, int role);
        int column(const QString &columnName) const;

        QString name; // Name of this field (eg: table1)
        QString type; // Type of field (eg: table)
        QString dataName; // Name associated with the data (eg: tasks)
        QStringList properties; // Info on how to handle the data (eg: values=bcws,bcwp,acwp)
        QStringList headerNames; // Lowercase list of all header names
        QStringList columns; // Lowercase list of headernames that shall be used
        QSortFilterProxyModel model;
        int seqNr; // A sequence number used to tabulate column names (eg: seqNr=2: table1.name2)
        int serieNr; // A chart needs to know which data series it works with
        QString hasLabels; // A chart needs to know if data shall contain legends/labels

        QModelIndex currentIndex;
    };

    UserField *findUserField(const KoXmlElement &decl) const;

private:
    KoStore *m_templateStore;
    QStringList m_manifestfiles;

    QList<QString> m_sortedfields;
    QList<QString> m_activefields;

    QMap<QString, UserField*> m_userfields;

    QMap<QString, QString> m_embededcharts;
    QMap<QString, QString> m_embededgantts;

    QList<QByteArray> m_tags; // cache tags for survival
};

QDebug operator<<(QDebug dbg, ReportGeneratorOdt::UserField *f);
QDebug operator<<(QDebug dbg, ReportGeneratorOdt::UserField &f);

} //namespace KPlato

#endif // REPORTGENERATOR_H
