/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATO_HELP_H
#define KPLATO_HELP_H

#include "kowidgetutils_export.h"

#include <QObject>
#include <QDebug>
#include <QLoggingCategory>
#include <QMap>
#include <QUrl>

class QEvent;

extern const KOWIDGETUTILS_EXPORT QLoggingCategory &PLANHELP_LOG();

#define debugPlanHelp qCDebug(PLANHELP_LOG)<<Q_FUNC_INFO
#define warnPlanHelp qCWarning(PLANHELP_LOG)
#define errorPlanHelp qCCritical(PLANHELP_LOG)

// FIXME: do not leak this
namespace KPlato
{

/**
 * Typical usage:
 * In your application:
 * <code>
 * KoDocumentEntry entry = KoDocumentEntry::queryByMimeType(PLAN_MIME_TYPE);
 * QJsonObject json = entry.metaData();
 * auto docs = json.value("X-PLAN-Documentation").toVariant().toString().split(';', Qt::SkipEmptyParts);
 * auto help = KPlato::Help::instance();
 * help->setDocs(docs);
 * help->setContentsUrl(QUrl(KPlatoSettings::documentationPath()));
 * help->setContextUrl(QUrl(KPlatoSettings::contextPath()));
 * qApp->installEventFilter(help); // this must go after filter installed by KMainWindow, so it will be called before
 * </code>
 * In your widget:
 * <code>
 * setWhatsThis(xi18nc("@info:whatsthis",
 *     "<title>Task Editor</title>"
 *     "<para>"
 *     "The Task Editor is used to create, edit, and delete tasks. "
 *     "Tasks are organized into a Work Breakdown Structure (WBS) to any depth."
 *     "</para><para>"
 *     "This view supports configuration and printing using the context menu."
 *     "<nl/><link url='%1'>More...</link>"
 *    "</para>", QStringLiteral("plan:task-editor")));
 * </code>
 * Note the "plan" keyword in the link. Allowed keywords are defined by the
 * X-PLAN-Documentation entry in the parts desktop file
 */

class KOWIDGETUTILS_EXPORT Help : public QObject
{
    Q_OBJECT
public:
    static Help *instance();
    void setContentsUrl(const QUrl &url);
    void setContextUrl(const QUrl &url);
    void setDocs(const QStringList &docs);
    QString doc(const QString &key) const;

    bool invokeContent(QUrl url);
    bool invokeContext(QUrl url);

    bool eventFilter(QObject *object, QEvent *event) override;

private:
    Help();
    ~Help() override;

    static Help* self;
    QUrl m_contentsUrl;
    QUrl m_contextUrl;
    QMap<QString, QString> m_docs;
};

} // namespace KPlato

#endif // KPLATO_HELP_H
