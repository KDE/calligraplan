/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004, 2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptconfig.h"

#include "calligraplansettings.h"
#include "kptconfigskeleton.h"
#include "kptfactory.h"
#include "kptproject.h"
#include "kptdebug.h"

#include <QBrush>


namespace KPlato
{

Config::Config()
    : ConfigBase()
{
    debugPlan<<"Leader:"<<KPlatoSettings::leader();
}

Config::~Config()
{
}

void Config::readConfig()
{
}
void Config::saveSettings()
{
    if (! m_readWrite) {
        return;
    }
    KPlatoSettings::self()->save();
}

bool Config::isWorkingday(int day) const
{
    switch (day) {
        case Qt::Monday: return KPlatoSettings::monday(); break;
        case Qt::Tuesday: return KPlatoSettings::tuesday(); break;
        case Qt::Wednesday: return KPlatoSettings::wednesday(); break;
        case Qt::Thursday: return KPlatoSettings::thursday(); break;
        case Qt::Friday: return KPlatoSettings::friday(); break;
        case Qt::Saturday: return KPlatoSettings::saturday(); break;
        case Qt::Sunday: return KPlatoSettings::sunday(); break;
        default: break;
    };
    return false;
}
QTime Config::dayStartTime(int day) const
{
    switch (day) {
        case Qt::Monday: return QTime::fromString(KPlatoSettings::mondayStart()); break;
        case Qt::Tuesday: return QTime::fromString(KPlatoSettings::tuesdayStart()); break;
        case Qt::Wednesday: return QTime::fromString(KPlatoSettings::wednesdayStart()); break;
        case Qt::Thursday: return QTime::fromString(KPlatoSettings::thursdayStart()); break;
        case Qt::Friday: return QTime::fromString(KPlatoSettings::fridayStart()); break;
        case Qt::Saturday: return QTime::fromString(KPlatoSettings::saturdayStart()); break;
        case Qt::Sunday: return QTime::fromString(KPlatoSettings::sundayStart()); break;
        default: break;
    };
    return QTime();

}

int Config::dayLength(int day) const
{
    QTime start = dayStartTime(day);
    QTime end;
    int value = 0;
    switch (day) {
        case Qt::Monday:
            end = QTime::fromString(KPlatoSettings::mondayEnd());
            break;
        case Qt::Tuesday:
            end = QTime::fromString(KPlatoSettings::tuesdayEnd());
            break;
        case Qt::Wednesday:
            end = QTime::fromString(KPlatoSettings::wednesdayEnd());
            break;
        case Qt::Thursday:
            end = QTime::fromString(KPlatoSettings::thursdayEnd());
            break;
        case Qt::Friday:
            end = QTime::fromString(KPlatoSettings::fridayEnd());
            break;
        case Qt::Saturday:
            end = QTime::fromString(KPlatoSettings::saturdayEnd());
            break;
        case Qt::Sunday:
            end = QTime::fromString(KPlatoSettings::sundayEnd());
            break;
        default: break;
    };
    value = start.msecsTo(end);
    if (value < 0) {
        value = (24*60*60*1000) + value;
    } else if (value == 0 && start == QTime(0, 0)) {
        value = 24*60*60*1000;
    }
    return value;

}


void Config::setDefaultValues(Project &project) const
{
    project.setLeader(KPlatoSettings::manager());
    project.setUseSharedResources(KPlatoSettings::useSharedResources());
    project.setSharedResourcesFile(KPlatoSettings::sharedResourcesFile());
    project.setDescription(KPlatoSettings::projectDescription());

    StandardWorktime *v = project.standardWorktime();
    Q_ASSERT(v);
    if (v) {
        v->setYear(KPlatoSettings::hoursPrYear());
        v->setMonth(KPlatoSettings::hoursPrMonth());
        v->setWeek(KPlatoSettings::hoursPrWeek());
        v->setDay(KPlatoSettings::hoursPrDay());
    }

    Project::WorkPackageInfo wpi;
    wpi.checkForWorkPackages = KPlatoSettings::checkForWorkPackages();
    wpi.retrieveUrl = KPlatoSettings::retrieveUrl();
    wpi.deleteAfterRetrieval = KPlatoSettings::deleteFile();
    wpi.archiveAfterRetrieval = !KPlatoSettings::deleteFile();
    wpi.archiveUrl = KPlatoSettings::saveUrl();
#if 0 // not used atm
    wpi.publishUrl = KPlatoSettings::publishUrl();
#endif
    project.setWorkPackageInfo(wpi);

    QList<QUrl> urls;
    const QStringList paths = taskModulePaths();
    for (const QString &s : paths) {
        urls << QUrl::fromUserInput(s);
    }
    project.setTaskModules(urls, useLocalTaskModules());
}

void Config::setDefaultValues(Task &task)
{
    task.setLeader(KPlatoSettings::leader());
    task.setDescription(KPlatoSettings::description());
    task.setConstraint((Node::ConstraintType) KPlatoSettings::constraintType());

    // avoid problems with start <= end & end >= start
    task.setConstraintStartTime(DateTime());
    task.setConstraintEndTime(DateTime());
    switch (KPlatoSettings::startTimeUsage()) {
        case KPlatoSettings::EnumStartTimeUsage::CurrentdateTime:
            task.setConstraintStartTime(DateTime(QDateTime::currentDateTime()));
            break;
        case KPlatoSettings::EnumStartTimeUsage::CurrentDate:
            task.setConstraintStartTime(DateTime(QDate::currentDate(), KPlatoSettings::constraintStartTime().time()));
            break;
        case KPlatoSettings::EnumStartTimeUsage::SpecifiedDateTime: //fall through
        default:
            task.setConstraintStartTime(DateTime(KPlatoSettings::constraintStartTime()));
            break;
    }
    switch (KPlatoSettings::endTimeUsage()) {
        case KPlatoSettings::EnumEndTimeUsage::CurrentdateTime:
            task.setConstraintEndTime(DateTime(QDateTime::currentDateTime()));
            break;
        case KPlatoSettings::EnumEndTimeUsage::CurrentDate:
            task.setConstraintEndTime(DateTime(QDate::currentDate(), KPlatoSettings::constraintEndTime().time()));
            break;
        case KPlatoSettings::EnumEndTimeUsage::SpecifiedDateTime: //fall through
        default:
            task.setConstraintEndTime(DateTime(KPlatoSettings::constraintEndTime()));
            break;
    }

    task.estimate()->setType((Estimate::Type) KPlatoSettings::estimateType());
    task.estimate()->setUnit((Duration::Unit) KPlatoSettings::unit());
    task.estimate()->setExpectedEstimate(KPlatoSettings::expectedEstimate());
    task.estimate()->setRisktype(static_cast<Estimate::Risktype>(KPlatoSettings::risk()));
    task.estimate()->setPessimisticRatio(KPlatoSettings::pessimisticRatio());
    task.estimate()->setOptimisticRatio(KPlatoSettings::optimisticRatio());
}

int Config::minimumDurationUnit() const
{
    return KPlatoSettings::minimumDurationUnit();
}

int Config::maximumDurationUnit() const
{
    return KPlatoSettings::maximumDurationUnit();
}

QBrush Config::summaryTaskDefaultColor() const
{
    QColor c = KPlatoSettings::summaryTaskDefaultColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

bool Config::summaryTaskLevelColorsEnabled() const
{
    return KPlatoSettings::summaryTaskLevelColorsEnabled();
}

QBrush Config::summaryTaskLevelColor_1() const
{
    QColor c = KPlatoSettings::summaryTaskLevelColor_1();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::summaryTaskLevelColor_2() const
{
    QColor c = KPlatoSettings::summaryTaskLevelColor_2();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::summaryTaskLevelColor_3() const
{
    QColor c = KPlatoSettings::summaryTaskLevelColor_3();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::summaryTaskLevelColor_4() const
{
    QColor c = KPlatoSettings::summaryTaskLevelColor_4();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::taskNormalColor() const
{
    QColor c = KPlatoSettings::taskNormalColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::taskErrorColor() const
{
    QColor c = KPlatoSettings::taskErrorColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::taskCriticalColor() const
{
    QColor c = KPlatoSettings::taskCriticalColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::taskFinishedColor() const
{
    QColor c = KPlatoSettings::taskFinishedColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::milestoneNormalColor() const
{
    QColor c = KPlatoSettings::milestoneNormalColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::milestoneErrorColor() const
{
    QColor c = KPlatoSettings::milestoneErrorColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::milestoneCriticalColor() const
{
    QColor c = KPlatoSettings::milestoneCriticalColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QBrush Config::milestoneFinishedColor() const
{
    QColor c = KPlatoSettings::milestoneFinishedColor();
    if (KPlatoSettings::colorGradientType() == KPlatoSettings::EnumColorGradientType::Linear) {
        return gradientBrush(c);
    }
    return c;
}

QString Config::documentationPath() const
{
    return KPlatoSettings::documentationPath();
}

QString Config::contextPath() const
{
    return KPlatoSettings::contextPath();
}

QString Config::contextLanguage() const
{
    return KPlatoSettings::contextLanguage();
}

bool Config::useLocalTaskModules() const
{
    return KPlatoSettings::useLocalTaskModules();
}

QStringList Config::taskModulePaths() const
{
    return KPlatoSettings::taskModulePaths();
}

QStringList Config::projectTemplatePaths() const
{
    return KPlatoSettings::projectTemplatePaths();
}

}  //KPlato namespace
