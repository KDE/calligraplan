/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009, 2011 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCOMMONSTRINGS_H
#define KPTCOMMONSTRINGS_H

#include "planmodels_export.h"

class QString;

namespace KPlato
{

struct PLANMODELS_EXPORT ToolTip
{
    static QString nodeName();
    static QString nodeType();
    static QString nodeResponsible();
    static QString allocation();
    static QString nodeConstraint();
    static QString nodeConstraintStart();
    static QString nodeConstraintEnd();
    static QString nodeDescription();
    static QString nodeWBS();
    static QString nodeLevel();
    static QString nodeRisk();
    static QString nodePriority();
    static QString nodeRunningAccount();
    static QString nodeStartupAccount();
    static QString nodeStartupCost();
    static QString nodeShutdownAccount();
    static QString nodeShutdownCost();

    static QString nodeStartTime();
    static QString nodeEndTime();
    static QString nodeEarlyStart();
    static QString nodeEarlyFinish();
    static QString nodeLateStart();
    static QString nodeLateFinish();
    
    static QString nodeDuration();
    static QString nodeVarianceDuration();
    static QString nodeOptimisticDuration();
    static QString nodePessimisticDuration();

    static QString nodePositiveFloat();
    static QString nodeNegativeFloat();
    static QString nodeFreeFloat();
    static QString nodeStartFloat();
    static QString nodeFinishFloat();
    static QString nodeAssignment();

    static QString nodeStatus();
    static QString nodeCompletion();
    static QString nodePlannedEffortTo();
    static QString nodeActualEffortTo();
    static QString nodeRemainingEffort();
    static QString nodePlannedCostTo();
    static QString nodeActualCostTo();
    static QString completionStartedTime();
    static QString completionStarted();
    static QString completionFinishedTime();
    static QString completionFinished();
    static QString completionStatusNote();
    
    static QString estimateExpected();
    static QString estimateVariance();
    static QString estimateOptimistic();
    static QString estimatePessimistic();
    static QString estimateType();
    static QString estimateCalendar();
    static QString estimate();
    static QString optimisticRatio();
    static QString pessimisticRatio();
    static QString riskType();

    static QString nodeSchedulingStatus();
    static QString nodeNotScheduled();
    static QString nodeAssignmentMissing();
    static QString nodeResourceOverbooked();
    static QString nodeResourceUnavailable();
    static QString nodeConstraintsError();
    static QString nodeEffortNotMet();
    static QString nodeSchedulingError();

    static QString nodeBCWS();
    static QString nodeBCWP();
    static QString nodeACWP();
    static QString nodePerformanceIndex();

    static QString resourceName();
    static QString resourceOrigin();
    static QString resourceType();
    static QString resourceInitials();
    static QString resourceEMail();
    static QString resourceCalendar();
    static QString resourceUnits();
    static QString resourceAvailableFrom();
    static QString resourceAvailableUntil();
    static QString resourceNormalRate();
    static QString resourceOvertimeRate();
    static QString resourceFixedCost();
    static QString resourceAccount();

    static QString resourceGroupName();
    static QString resourceGroupOrigin();
    static QString resourceGroupType();
    static QString resourceGroupUnits();
    static QString resourceGroupCoordinator();

    static QString accountName();
    static QString accountDescription();

    static QString scheduleName();
    static QString scheduleState();
    static QString scheduleOverbooking();
    static QString scheduleDistribution();
    static QString scheduleCalculate();
    static QString scheduleStart();
    static QString scheduleFinish();
    static QString schedulingDirection();
    static QString scheduleScheduler();
    static QString scheduleGranularity();
    static QString scheduleMode();

    static QString documentUrl();
    static QString documentType();
    static QString documentStatus();
    static QString documentSendAs();

    static QString calendarName();
    static QString calendarTimeZone();

    static QString relationParent();
    static QString relationChild();
    static QString relationType();
    static QString relationLag();

}; //namespace ToolTip

struct PLANMODELS_EXPORT WhatsThis
{
    static QString  nodeNegativeFloat();
    static QString  nodeFreeFloat();
    static QString  nodeStartFloat();
    static QString  nodeFinishFloat();

    static QString scheduleOverbooking();
    static QString scheduleDistribution();
    static QString schedulingDirection();
    static QString scheduleScheduler();

}; //namespace WhatsThis

} //namespace KPlato

#endif
