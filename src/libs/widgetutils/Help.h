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
 * <code>
 * Help::add(this, xi18nc("@info:whatsthis",
 *     "<title>Task Editor</title>"
 *     "<para>"
 *     "The Task Editor is used to create, edit, and delete tasks. "
 *     "Tasks are organized into a Work Breakdown Structure (WBS) to any depth."
 *     "</para><para>"
 *     "This view supports configuration and printing using the context menu."
 *     "<nl/><link url='%1'>More...</link>"
 *    "</para>", Help::page("Task_Editor")));
 * </code>
 */

class KOWIDGETUTILS_EXPORT Help
{
public:
    Help(const QString &docpath, const QString &language = QString());
    static void add(QWidget *widget, const QString &text);
    static QString page(const QString &page = QString());
    static void invoke(const QString &page);
    static void invoke(const QUrl &url);

protected:
    ~Help();

private:
    static Help* self;
    QString m_docpath;
};


class KOWIDGETUTILS_EXPORT WhatsThisClickedEventHandler : public QObject
{
    Q_OBJECT
public:
    WhatsThisClickedEventHandler(QObject *parent=nullptr);

    bool eventFilter(QObject *object, QEvent *event) override;

};


} // namespace KPlato

#endif // KPLATO_HELP_H
