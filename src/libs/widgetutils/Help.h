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
 * auto docs = json.value(QStringLiteral("X-PLAN-Documentation")).toVariant().toString().split(QLatin1Char(';'), Qt::SkipEmptyParts);
 * auto help = KPlato::Help::instance();
 * help->setDocs(docs);
 * help->setOnlineBaseUrl()); // If the docs are not in the default place
 * help->setOnline(true); // If you want the online docs, else khelpcenter will use installed docs
 * help->initiate();
 * qApp->installEventFilter(help); // This must go after filter installed by KMainWindow, so it will be called before
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

    // initiates help system
    // Checks for khelpcenter and online documentation if online is set
    void initiate();

    // Set @p b to true if you want to get the online documentation
    void setOnline(bool b);
    // Set the base url to for the online documentation
    void setOnlineBaseUrl(const QString &url);
    QString onlineBaseUrl() const;

    /// Used when protocol is help:
    /// The doc name will normally then be the application name.
    void setDocs(const QStringList &docs);
    void setDoc(const QString &id, const QString &doc);
    QString doc(const QString &id) const;

    /// Used when getting docs form docs.kde.org
    /// We try to find the correct language but unfortunately
    /// the docs are not stored exactly under a locale name.
    void setDocLanguage(const QString &doc, const QString &language);
    QString language(const QString &doc) const;

    /// Set the language to be used.
    /// Set an empty language to let the help search for documentation in your language.
    /// The language must be equal to a language listed on https://docs.kde.org.
    void setLanguage(const QString &language);

    bool invokeContent(QUrl url);
    bool invokeContext(QUrl url);

    bool eventFilter(QObject *object, QEvent *event) override;

private:
    Help();
    ~Help() override;

private:
    static Help* self;
    bool m_initiated = false;
    bool m_khelpcenter = true;
    bool m_online = false;
    QString m_onlineBaseUrl;
    QMap<QString, QString> m_docs; // QMap<id, doc> eg: QMap<plan, calligraplan>
    QString m_language;
    QMap<QString, QString> m_languages;
};

} // namespace KPlato

#endif // KPLATO_HELP_H
