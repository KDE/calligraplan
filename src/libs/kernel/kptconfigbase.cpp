/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptconfigbase.h"

#include "kptlocale.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QFontMetrics>

namespace KPlato
{

ConfigBase::ConfigBase()
    : m_taskDefaults(new Task())
{
    m_locale = new Locale();
    m_readWrite = true;
    // set some reasonable defaults
    m_taskDefaults->estimate()->setType(Estimate::Type_Effort);
    m_taskDefaults->estimate()->setUnit(Duration::Unit_h);
    m_taskDefaults->estimate()->setExpectedEstimate(1.0);
    m_taskDefaults->estimate()->setPessimisticRatio(0);
    m_taskDefaults->estimate()->setOptimisticRatio(0);
}

ConfigBase::~ConfigBase()
{
    delete m_taskDefaults;
    delete m_locale;
}

void ConfigBase::setTaskDefaults(Task *task)
{
    if (m_taskDefaults != task) {
        delete m_taskDefaults;
        m_taskDefaults = task;
    }
}

QBrush ConfigBase::summaryTaskLevelColor(int level) const
{
    if (summaryTaskLevelColorsEnabled()) {
        switch (level) {
            case 1: return summaryTaskLevelColor_1();
            case 2: return summaryTaskLevelColor_2();
            case 3: return summaryTaskLevelColor_3();
            case 4: return summaryTaskLevelColor_4();
            default: break;
        }
    }
    return summaryTaskDefaultColor();
}

bool ConfigBase::summaryTaskLevelColorsEnabled() const
{
    return false;
}

QBrush ConfigBase::summaryTaskDefaultColor() const
{
    return gradientBrush(Qt::blue);
}

QBrush ConfigBase::summaryTaskLevelColor_1() const
{
    return gradientBrush(Qt::blue);
}

QBrush ConfigBase::summaryTaskLevelColor_2() const
{
    return gradientBrush(Qt::blue);
}

QBrush ConfigBase::summaryTaskLevelColor_3() const
{
    return gradientBrush(Qt::blue);
}

QBrush ConfigBase::summaryTaskLevelColor_4() const
{
    return gradientBrush(Qt::blue);
}

QBrush ConfigBase::taskNormalColor() const
{
    return gradientBrush(Qt::green);
}

QBrush ConfigBase::taskErrorColor() const
{
    return gradientBrush(Qt::yellow);
}

QBrush ConfigBase::taskCriticalColor() const
{
    return gradientBrush(Qt::red);
}

QBrush ConfigBase::taskFinishedColor() const
{
    return gradientBrush(Qt::gray);
}

QBrush ConfigBase::milestoneNormalColor() const
{
    return gradientBrush(Qt::blue);
}

QBrush ConfigBase::milestoneErrorColor() const
{
    return gradientBrush(Qt::yellow);
}

QBrush ConfigBase::milestoneCriticalColor() const
{
    return gradientBrush(Qt::red);
}

QBrush ConfigBase::milestoneFinishedColor() const
{
    return gradientBrush(Qt::gray);
}

const Locale *ConfigBase::locale() const
{
    return m_locale;
}

Locale *ConfigBase::locale()
{
    return m_locale;
}

//static
QBrush ConfigBase::gradientBrush(const QColor &c)
{
    QFontMetricsF metrics(QApplication::font());
    QLinearGradient b(0., 0., 0., metrics.height());
    b.setColorAt(0., c);
    b.setColorAt(1., c.darker());
    return QBrush(b);
}

}  //KPlato namespace
