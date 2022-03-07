/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007-2008 Adam Pigg (adam@piggz.co.uk)
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
// clazy:excludeall=qstring-arg
#include "PlanReportScriptText.h"

#include <QFile>
#include <QTextStream>

namespace Scripting
{

Text::Text(PlanReportItemText* t)
{
    m_text = t;
}


Text::~Text()
{
}

QString Text::source() const
{
    return m_text->itemDataSource();
}

void Text::setSource(const QString& s)
{
    m_text->m_controlSource->setValue(s);
}

int Text::horizontalAlignment() const
{
    const QString a = m_text->m_horizontalAlignment->value().toString().toLower();

    if (a == QStringLiteral("left")) {
        return -1;
    }
    if (a == QStringLiteral("center")) {
        return 0;
    }
    if (a == QStringLiteral("right")) {
        return 1;
    }
    return -1;
}
void Text::setHorizonalAlignment(int a)
{
    switch (a) {
    case -1:
        m_text->m_horizontalAlignment->setValue(QStringLiteral("left"));
        break;
    case 0:
        m_text->m_horizontalAlignment->setValue(QStringLiteral("center"));
        break;
    case 1:
        m_text->m_horizontalAlignment->setValue(QStringLiteral("right"));
        break;
    default:
        m_text->m_horizontalAlignment->setValue(QStringLiteral("left"));
        break;
    }
}

int Text::verticalAlignment() const
{
    const QString a = m_text->m_horizontalAlignment->value().toString().toLower();

    if (a == QStringLiteral("top")) {
        return -1;
    }
    if (a == QStringLiteral("middle")) {
        return 0;
    }
    if (a == QStringLiteral("bottom")) {
        return 1;
    }
    return -1;
}
void Text::setVerticalAlignment(int a)
{
    switch (a) {
    case -1:
        m_text->m_verticalAlignment->setValue(QStringLiteral("top"));
        break;
    case 0:
        m_text->m_verticalAlignment->setValue(QStringLiteral("middle"));
        break;
    case 1:
        m_text->m_verticalAlignment->setValue(QStringLiteral("bottom"));
        break;
    default:
        m_text->m_verticalAlignment->setValue(QStringLiteral("middle"));
        break;
    }
}

QColor Text::backgroundColor() const
{
    return m_text->m_backgroundColor->value().value<QColor>();
}
void Text::setBackgroundColor(const QColor& c)
{
    m_text->m_backgroundColor->setValue(QColor(c));
}

QColor Text::foregroundColor() const
{
    return m_text->m_foregroundColor->value().value<QColor>();
}
void Text::setForegroundColor(const QColor& c)
{
    m_text->m_foregroundColor->setValue(QColor(c));
}

int Text::backgroundOpacity() const
{
    return m_text->m_backgroundOpacity->value().toInt();
}
void Text::setBackgroundOpacity(int o)
{
    m_text->m_backgroundOpacity->setValue(o);
}

QColor Text::lineColor() const
{
    return m_text->m_lineColor->value().value<QColor>();
}
void Text::setLineColor(const QColor& c)
{
    m_text->m_lineColor->setValue(QColor(c));
}

int Text::lineWeight() const
{
    return m_text->m_lineWeight->value().toInt();
}
void Text::setLineWeight(int w)
{
    m_text->m_lineWeight->setValue(w);
}

int Text::lineStyle() const
{
    return m_text->m_lineStyle->value().toInt();
}
void Text::setLineStyle(int s)
{
    if (s < 0 || s > 5) {
        s = 1;
    }
    m_text->m_lineStyle->setValue(s);
}

QPointF Text::position() const
{
    return m_text->m_pos.toPoint();
}
void Text::setPosition(const QPointF& p)
{
    m_text->m_pos.setPointPos(p);
}

QSizeF Text::size() const
{
    return m_text->m_size.toPoint();
}
void Text::setSize(const QSizeF& s)
{
    m_text->m_size.setPointSize(s);
}

void Text::loadFromFile(const QString &fn)
{
    QFile file(fn);
    //kreportpluginDebug() << "Loading from" << fn;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_text->m_controlSource->setValue(tr("$Unable to read %1").arg(fn));
        return;
    }
    QTextStream in(&file);
    QString data = in.readAll();
    /*
    while (!in.atEnd()) {
      QString line = in.readLine();
      process_line(line);
    }*/
    m_text->m_controlSource->setValue(QVariant(QStringLiteral("$") + data));
}

}
