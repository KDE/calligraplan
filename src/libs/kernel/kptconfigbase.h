/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCONFIGBASE_H
#define KPTCONFIGBASE_H

#include "kpttask.h"


namespace KPlato
{
class Locale;

class PLANKERNEL_EXPORT ConfigBase : public QObject
{
    Q_OBJECT
public:
    ConfigBase();
    ~ConfigBase() override;

    void setReadWrite(bool readWrite) { m_readWrite = readWrite; }
    Task &taskDefaults() const {
        const_cast<ConfigBase*>(this)->setDefaultValues(*m_taskDefaults);
        return *m_taskDefaults;
    }

    void setTaskDefaults(Task *);

    virtual void setDefaultValues(Project &) const {}
    virtual void setDefaultValues(Task &) {}
    virtual QPair<int, int> durationUnitRange() const { return QPair<int, int>(); }
    virtual int minimumDurationUnit() const { return Duration::Unit_m; }
    virtual int maximumDurationUnit() const { return Duration::Unit_Y; }

    QBrush summaryTaskLevelColor(int level) const;
    virtual bool summaryTaskLevelColorsEnabled() const;
    virtual QBrush summaryTaskDefaultColor() const;
    virtual QBrush summaryTaskLevelColor_1() const;
    virtual QBrush summaryTaskLevelColor_2() const;
    virtual QBrush summaryTaskLevelColor_3() const;
    virtual QBrush summaryTaskLevelColor_4() const;

    virtual QBrush taskNormalColor() const;
    virtual QBrush taskErrorColor() const;
    virtual QBrush taskCriticalColor() const;
    virtual QBrush taskFinishedColor() const;

    virtual QBrush milestoneNormalColor() const;
    virtual QBrush milestoneErrorColor() const;
    virtual QBrush milestoneCriticalColor() const;
    virtual QBrush milestoneFinishedColor() const;

    const Locale *locale() const;
    Locale *locale();

    virtual QString documentationPath() const { return QString(); }
    virtual QString contextPath() const { return QString(); }
    virtual QString contextLanguage() const { return QString(); }

    virtual bool useLocalTaskModules() const { return true; }
    virtual QStringList taskModulePaths() const { return QStringList(); }

    virtual QStringList projectTemplatePaths() const { return QStringList(); }

    static QBrush gradientBrush(const QColor &c);

protected:
    bool m_readWrite;

private:
    Task *m_taskDefaults;

    Locale *m_locale;
};

}  //KPlato namespace

#endif // CONFIGBASE_H
