/* This file is part of the KDE project
 * Copyright (C) 2009 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ICALENDAREXPORT_H
#define ICALENDAREXPORT_H


#include <KoFilter.h>

#define USE_KCALCORE

#ifdef USE_KCALCORE
#include <kcalcore/calendar.h>
#endif

#include <QObject>
#include <QVariantList>

class QFile;
class QByteArray;


namespace KPlato
{
class Project;
class Node;
}

class ICalendarExport : public KoFilter
{

    Q_OBJECT

public:
    ICalendarExport(QObject* parent, const QVariantList &);
    ~ICalendarExport() override {}

    KoFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to) override;

protected:
    KoFilter::ConversionStatus convert(const KPlato::Project &project, QFile &file);
#ifdef USE_KCALCORE
    void createTodos(KCalCore::Calendar::Ptr cal, const KPlato::Node *node, long id, KCalCore::Todo::Ptr parent = KCalCore::Todo::Ptr());
#endif

private:
    QString doNode(const KPlato::Node *node, long sid);
    QString createTodo(const KPlato::Node &node, long sid);
    QString doDescription(const QString &description);

private:
    long m_scheduleId;
    bool m_includeProject;
    bool m_includeSummarytasks;
    QList<QString> m_descriptions;
};

#endif // ICALENDAREXPORT_H
