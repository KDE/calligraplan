/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

package plan;

import java.io.IOException;
import java.io.OutputStream;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.text.ParsePosition;
import java.util.TimeZone;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;

import net.sf.mpxj.ConstraintType;
import net.sf.mpxj.DateRange;
import net.sf.mpxj.Day;
import net.sf.mpxj.DayType;
import net.sf.mpxj.Duration;
import net.sf.mpxj.ProjectCalendar;
import net.sf.mpxj.ProjectCalendarContainer;
import net.sf.mpxj.ProjectCalendarException;
import net.sf.mpxj.ProjectCalendarHours;
import net.sf.mpxj.ProjectCalendarDateRanges;
import net.sf.mpxj.ProjectFile;
import net.sf.mpxj.ProjectProperties;
import net.sf.mpxj.Relation;
import net.sf.mpxj.RelationType;
import net.sf.mpxj.Resource;
import net.sf.mpxj.ResourceContainer;
import net.sf.mpxj.ResourceType;
import net.sf.mpxj.Rate;
import net.sf.mpxj.ResourceAssignment;
import net.sf.mpxj.ResourceAssignmentContainer;
import net.sf.mpxj.ScheduleFrom;
import net.sf.mpxj.TimeUnit;
import net.sf.mpxj.Task;
import net.sf.mpxj.TaskType;
import net.sf.mpxj.Priority;
import net.sf.mpxj.TimephasedWork;
import net.sf.mpxj.writer.AbstractProjectWriter;

import plan.schema.*;

/**
 * This class creates a new Plan file from the contents of 
 * a ProjectFile instance.
 */
public final class PlanWriter extends AbstractProjectWriter
{
    /**
    * {@inheritDoc}
    */
    public void write(ProjectFile projectFile, OutputStream stream) throws IOException
    {
        try
        {
            m_projectFile = projectFile;

            if (CONTEXT == null)
            {
                throw CONTEXT_EXCEPTION;
            }

            Marshaller marshaller = CONTEXT.createMarshaller();
            marshaller.setProperty(Marshaller.JAXB_FORMATTED_OUTPUT, Boolean.TRUE);
            if (m_encoding != null)
            {
                marshaller.setProperty(Marshaller.JAXB_ENCODING, m_encoding);
            }

            m_factory = new ObjectFactory();
            m_plan = m_factory.createPlan();
            writePlan();

            marshaller.marshal(m_plan, stream);
        }

        catch (JAXBException ex)
        {
            throw new IOException(ex.toString());
        }

        finally
        {
            m_projectFile = null;
            m_factory = null;
            m_planProject = null;
            m_plan = null;
        }
    }

    /**
    * This method writes a Plan xml file.
    */
    private void writePlan()
    {
        m_plan.setEditor("planConverter");
        m_plan.setVersion("0.7.0");
        m_plan.setMime("application/x-vnd.kde.plan");

        writeProject();
        writeProjectSettings();
        writeCalendars();
        writeResources();
        writeResourceGroupRelations();
        writeTasks();
        writeRelations();
        writeRequests();
        writeProjectSchedules();
        System.out.println("writePlan: finished");

    }
    /**
    * This method writes project data to a Plan file.
    */
    private void writeProject()
    {
        ProjectProperties projectProperties = m_projectFile.getProjectProperties();

        m_planProject = m_factory.createProject();
        m_plan.setProject(m_planProject);
        m_planProject.setId(getProjectId());
        m_planProject.setName(projectProperties.getName());
        m_planProject.setLeader(projectProperties.getManager());
        m_planProject.setScheduling(getScheduleFromString(projectProperties.getScheduleFrom()));
        m_planProject.setStartTime(getDateTimeString(projectProperties.getStartDate()));
        m_planProject.setEndTime(getDateTimeString(projectProperties.getFinishDate()));
      // m_planProject.setCompany(projectProperties.getCompany());
   }

    /**
    * This method writes project settings to a Plan file.
    */
    private void writeProjectSettings()
    {
        ProjectSettings projectSettings = m_factory.createProjectSettings();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(projectSettings);

        writeStandardWorktime(projectSettings);
        writeLocale(projectSettings);
    }

    /**
    * This method writes estimate conversion data to a Plan file.
    */
    private void writeStandardWorktime(ProjectSettings projectSettings)
    {
        ProjectProperties projectProperties = m_projectFile.getProjectProperties();

        StandardWorktime planStandardWorktime = m_factory.createStandardWorktime();
        projectSettings.getTaskModulesOrSharedResourcesOrWbsDefinitionOrLocaleOrWorkpackageinfoOrStandardWorktime().add(planStandardWorktime);

        planStandardWorktime.setDay(getMinutesString(projectProperties.getMinutesPerDay().longValue()));
        planStandardWorktime.setWeek(getMinutesString(projectProperties.getMinutesPerWeek().longValue()));
        planStandardWorktime.setMonth(getMinutesString(projectProperties.getMinutesPerDay().longValue() * projectProperties.getDaysPerMonth()));
        planStandardWorktime.setYear(getMinutesString(projectProperties.getMinutesPerYear()));
    }

    /**
    * This method writes estimate conversion data to a Plan file.
    */
    private void writeLocale(ProjectSettings projectSettings)
    {
        ProjectProperties projectProperties = m_projectFile.getProjectProperties();

        Locale planLocale = m_factory.createLocale();
        projectSettings.getTaskModulesOrSharedResourcesOrWbsDefinitionOrLocaleOrWorkpackageinfoOrStandardWorktime().add(planLocale);

        planLocale.setCurrencySymbol(projectProperties.getCurrencySymbol());
        planLocale.setCurrencyDigits(getIntegerString(projectProperties.getCurrencyDigits()));
    }

    /**
    * This method writes calendar data to a Plan file.
    */
    private void writeCalendars()
    {
        System.out.println("writeCalendars:");
        Calendars planCalendars = m_factory.createCalendars();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(planCalendars);
        ProjectCalendarContainer mpxjCalendars = m_projectFile.getCalendars();
        for (int i = 0; i < mpxjCalendars.size(); ++i)
        {
            ProjectCalendar mpxjCalendar = mpxjCalendars.get(i);
            plan.schema.Calendar planCalendar = m_factory.createCalendar();
            writeCalendar(mpxjCalendar, planCalendar);
            if (i == 0)
            {
                planCalendar.setDefault("1");
            }
            planCalendars.getCalendar().add(planCalendar);
        }
    }

    /**
    * This method writes data for a single calendar to a Plan file.
    *
    * @param mpxjCalendar MPXJ calendar instance
    * @param planCalendar Plan calendar instance
    */
    private void writeCalendar(ProjectCalendar mpxjCalendar, plan.schema.Calendar planCalendar)
    {
        try
        {
        System.out.println("writeCalendar: " + mpxjCalendar.getName() + ", id " + mpxjCalendar.getUniqueID());

        planCalendar.setId(getIntegerString(mpxjCalendar.getUniqueID()));
        planCalendar.setName(getString(mpxjCalendar.getName()));
        if (mpxjCalendar.getParent() != null)
        {
            planCalendar.setParent(getIntegerString(mpxjCalendar.getParent().getUniqueID()));
        }
        //
        // Set working and non working days for weekdays
        //
        Weekday weekday = m_factory.createWeekday();
        writeWeekday(mpxjCalendar, Day.MONDAY, weekday);
        planCalendar.getCalendarOrWeekdayOrDay().add(weekday);
        weekday = m_factory.createWeekday();
        writeWeekday(mpxjCalendar, Day.TUESDAY, weekday);
        planCalendar.getCalendarOrWeekdayOrDay().add(weekday);
        weekday = m_factory.createWeekday();
        writeWeekday(mpxjCalendar, Day.WEDNESDAY, weekday);
        planCalendar.getCalendarOrWeekdayOrDay().add(weekday);
        weekday = m_factory.createWeekday();
        writeWeekday(mpxjCalendar, Day.THURSDAY, weekday);
        planCalendar.getCalendarOrWeekdayOrDay().add(weekday);
        weekday = m_factory.createWeekday();
        writeWeekday(mpxjCalendar, Day.FRIDAY, weekday);
        planCalendar.getCalendarOrWeekdayOrDay().add(weekday);
        weekday = m_factory.createWeekday();
        writeWeekday(mpxjCalendar, Day.SATURDAY, weekday);
        planCalendar.getCalendarOrWeekdayOrDay().add(weekday);
        weekday = m_factory.createWeekday();
        writeWeekday(mpxjCalendar, Day.SUNDAY, weekday);
        planCalendar.getCalendarOrWeekdayOrDay().add(weekday);

        //System.out.println("writeCalendar: exceptions:");
        //
        // Set working and non working days for specific dates
        //
        for (ProjectCalendarException mpxjCalendarException : mpxjCalendar.getCalendarExceptions())
        {
            Date rangeStartDay = mpxjCalendarException.getFromDate();
            Date rangeEndDay = mpxjCalendarException.getToDate();
            while (rangeStartDay.getTime() == rangeEndDay.getTime())
            {
                System.out.println("Exception Day: " + mpxjCalendarException);
                //
                // Exception covers a single day
                //
                plan.schema.Day planDay = m_factory.createDay();
                planCalendar.getCalendarOrWeekdayOrDay().add(planDay);
                planDay.setDate(getDateString(mpxjCalendarException.getFromDate()));
                planDay.setState(mpxjCalendarException.getWorking() ? "2" : "1");
            }
        }

        //System.out.println("writeCalendar: derived:");
        //
        // Process any derived calendars
        //
        for (ProjectCalendar mpxjDerivedCalendar : mpxjCalendar.getDerivedCalendars())
        {
            if (mpxjDerivedCalendar != null)
            {
                //System.out.println("writeCalendar: derived: " + mpxjDerivedCalendar);
                plan.schema.Calendar planDerivedCalendar = m_factory.createCalendar();
                planCalendar.getCalendarOrWeekdayOrDay().add(planDerivedCalendar);
                writeCalendar(mpxjDerivedCalendar, planDerivedCalendar);
            }
        }
        System.out.println("writeCalendar: end " + mpxjCalendar.getUniqueID());
        }
        catch (Exception ex)
        {
            System.out.println("Calendar Exception");
            ex.printStackTrace(System.out);
        }
    }

    /**
    * This method writes data for a single weekday to a Plan file.
    *
    * @param mpxjCalendar MPXJ calendar instance
    * @param day a MPJX Day instance
    * @param planWeekday Plan planWeekday instance
    */
    private void writeWeekday(net.sf.mpxj.ProjectCalendar mpxjCalendar, Day day, plan.schema.Weekday planWeekday)
    {
        //System.out.println("writeWeekday: " + mpxjCalendar.getName() + " : " + day + " : " + mpxjCalendar.getWorkWeeks());
        int[] dayno = new int[] {-1, 6, 0, 1, 2, 3, 4, 5};
        planWeekday.setDayNumber(getIntegerString(dayno[day.getValue()]));
        planWeekday.setState(getWorkingDayString(mpxjCalendar, day));
        ProjectCalendarHours hours = mpxjCalendar.getHours(day);
        for (DateRange range : hours) {
            //System.out.println("writeWeekday: " + day + " range " + range);
            TimeInterval planTimeInterval = m_factory.createTimeInterval();
            planWeekday.getTimeInterval().add(planTimeInterval);
            planTimeInterval.setStart(getTimeString(getTime(range.getStart())));
            planTimeInterval.setLength(getIntegerString(range.getEnd().getTime() - range.getStart().getTime()));
            //System.out.println("writeWeekday: " + day + " : " + planTimeInterval.getStart() + " : " + planTimeInterval.getLength());
        }
    }

    /**
    * This method writes data for a all resources to a Plan file.
    */
    private void writeResources()
    {
        // prepare for possible resource groups
        m_resourcegroups = m_factory.createResourceGroups();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(m_resourcegroups);

        Resources planResources = m_factory.createResources();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(planResources);

        ResourceContainer mpxjResources = m_projectFile.getResources();
        for (int i = 0; i < mpxjResources.size(); ++i)
        {
            writeResource(mpxjResources.get(i), planResources);
        }
    }

    /**
    * This method writes data for a single resource to a Plan file.
    *
    * @param mpxjResource MPXJ Resource instance
    */
    private void writeResource(Resource mpxjResource, Resources planResources)
    {
        plan.schema.Resource planResource = m_factory.createResource();
        planResources.getResource().add(planResource);

//         ProjectCalendar resourceCalendar = mpxjResource.getResourceCalendar();
//         if (resourceCalendar != null)
//         {
//             planResource.setCalendarId(getIntegerString(resourceCalendar.getUniqueID()));
//         }

        planResource.setEmail(mpxjResource.getEmailAddress());
        planResource.setId(getResourceId(mpxjResource));
        planResource.setName(getString(mpxjResource.getName()));
        planResource.setInitials(mpxjResource.getInitials());
        planResource.setType(mpxjResource.getType() == ResourceType.MATERIAL ? "Material" : "Work");
        planResource.setUnits("100");

        // TODO convert rate with different unit than hour
        Rate rate = mpxjResource.getStandardRate();
        if (rate != null) {
            planResource.setNormalRate(Double.toString(rate.getAmount()));
        }
        rate = mpxjResource.getOvertimeRate();
        if (rate != null) {
            planResource.setOvertimeRate(Double.toString(rate.getAmount()));
        }
        if (mpxjResource.getGroup() != null) {
            if (!m_groups.containsKey(mpxjResource.getGroup())) {
                m_groups.put(mpxjResource.getGroup(), planResource.getId());
                ResourceGroup resourcegroup = m_factory.createResourceGroup();
                m_resourcegroups.getResourceGroup().add(resourcegroup);
                resourcegroup.setId(mpxjResource.getGroup());
                resourcegroup.setName(mpxjResource.getGroup());
            }
            System.out.println("writeResource: " + mpxjResource + " group: " + mpxjResource.getGroup());
        }
    }

    /**
    * This method writes data for a all resourcesgroup-resource-relations to a Plan file.
    * Note: Must be called after writeResources()
    */
    private void writeResourceGroupRelations()
    {
        ResourceGroupRelations planRelations = m_factory.createResourceGroupRelations();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(planRelations);

        for (Map.Entry<String,String> entry : m_groups.entrySet()) {
            ResourceGroupRelation planRelation = m_factory.createResourceGroupRelation();
            planRelations.getResourceGroupRelation().add(planRelation);
            planRelation.setGroupId(entry.getKey());
            planRelation.setResourceId(entry.getValue());
            System.out.println("writeResourceGroupRelations: " + entry);
        }
    }

   /**
    * This method writes task data to a Plan file.
    *
    * @throws JAXBException on xml creation errors
    */
    private void writeTasks()
    {
        Tasks planTasks = m_factory.createTasks();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(planTasks);

        for (Task mpxjTask : m_projectFile.getChildTasks())
        {
            plan.schema.Task planTask = m_factory.createTask();
            planTasks.getTask().add(planTask);
            writeTask(mpxjTask, planTask);
        }
    }

    /**
        * This method writes data for a single task to a Plan file.
        *
        * @param mpxjTask MPXJ Task instance
        * @param taskList list of child tasks for current parent
        */
    private void writeTask(Task mpxjTask, plan.schema.Task planTask)
    {
        if (mpxjTask.getName() == null && ! mpxjTask.hasChildTasks()) {
            System.out.println("writeTask: Task is probably a bogus task for internal use, add anyways: " + mpxjTask);
        }
        planTask.setId(getTaskId(mpxjTask));
        planTask.setWbs(mpxjTask.getWBS());
        planTask.setName(getString(mpxjTask.getName()));
        setScheduling(mpxjTask, planTask);
        Priority prio = mpxjTask.getPriority();
        System.out.println("writeTask: " + mpxjTask + " prio: " + prio);
        if (prio != null) {
            planTask.setPriority(getIntegerString(prio.getValue()));
        }

        writeEstimate(mpxjTask, planTask);
        writeTaskSchedules(mpxjTask, planTask);

        String note = mpxjTask.getNotes();
        if (note != null)
        {
            planTask.setDescription(note);
        }
        writeProgress(mpxjTask, planTask);

        //
        // Write child tasks
        //
        for (Task task : mpxjTask.getChildTasks())
        {
            plan.schema.Task childTask = m_factory.createTask();
            planTask.getTaskOrEstimateOrDocumentsOrTaskSchedulesOrProgress().add(childTask);
            writeTask(task, childTask);
        }
   }

    /**
    * This method writes task resource requests to a Plan file.
    *
    */
    private void writeRequests()
    {
        ResourceRequests planRequests = m_factory.createResourceRequests();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(planRequests);

        for (Task mpxjTask : m_projectFile.getTasks()) {
            if (mpxjTask.getMilestone() || mpxjTask.getSummary())
            {
                continue;
            }
            writeRequests(mpxjTask, planRequests);
        }
    }

    /**
    * This method writes task resource requests to a Plan file.
    *
    */
    private void writeRequests(Task mpxjTask, plan.schema.ResourceRequests planRequests)
    {
        //System.out.println("writeRequests: " + mpxjTask);

        int requestId = 1;
        for (ResourceAssignment mpxjAssignment : mpxjTask.getResourceAssignments())
        {
            //System.out.println("writeRequests: " + mpxjAssignment + " : " + mpxjAssignment.getResource());
            if (mpxjAssignment.getResource() == null)
            {
                continue;
            }
            ResourceRequest planResourceRequest = m_factory.createResourceRequest();
            planRequests.getResourceRequest().add(planResourceRequest);

            planResourceRequest.setRequestId(getIntegerString(requestId++));
            planResourceRequest.setTaskId(getTaskId(mpxjTask));
            planResourceRequest.setResourceId(getResourceId(mpxjAssignment.getResource()));
            planResourceRequest.setUnits(getIntegerString(mpxjAssignment.getUnits()));
        }
    }

    /**
    * This method writes task estimate data to a Plan file.
    *
    */
    private void writeEstimate(Task mpxjTask, plan.schema.Task planTask)
    {
        if (mpxjTask.getSummary()) {
            return;
        }
        Estimate planEstimate = m_factory.createEstimate();
        planTask.getTaskOrEstimateOrDocumentsOrTaskSchedulesOrProgress().add(planEstimate);

        planEstimate.setType(getEstimateType(mpxjTask.getType()));

        if (mpxjTask.getMilestone())
        {
            planEstimate.setExpected("0");
            planEstimate.setOptimistic("0");
            planEstimate.setPessimistic("0");
        }
        else
        {
            Duration duration = mpxjTask.getDuration();
            if (duration == null) {
                System.out.println("writeEstimate: " + mpxjTask + " null duration");
                return;
            }
            if (mpxjTask.getType() == TaskType.FIXED_DURATION)
            {
                // if duration is in working time a calendar must be added
                switch (duration.getUnits())
                {
                    case MINUTES :
                    case HOURS :
                    case DAYS :
                    case WEEKS :
                    case MONTHS :
                    case YEARS :
                    {
                        // add a calendar, use default calendar for now
                        planEstimate.setCalendarId(getIntegerString(m_projectFile.getDefaultCalendar().getUniqueID()));
                        break;
                    }
                    default :
                    {
                        break;
                    }
                }
            }
            else // effort
            {
                // if duration is in calendar time, it must be
                // converted from ELAPSED_* (working time)
                switch (duration.getUnits())
                {
                    case ELAPSED_MINUTES :
                    {
                        duration = duration.convertUnits(TimeUnit.MINUTES, m_projectFile.getProjectProperties());
                        break;
                    }
                    case ELAPSED_HOURS :
                    {
                        duration = duration.convertUnits(TimeUnit.HOURS, m_projectFile.getProjectProperties());
                        break;
                    }
                    case ELAPSED_DAYS :
                    {
                        duration = duration.convertUnits(TimeUnit.DAYS, m_projectFile.getProjectProperties());
                        break;
                    }
                    case ELAPSED_WEEKS :
                    {
                        duration = duration.convertUnits(TimeUnit.WEEKS, m_projectFile.getProjectProperties());
                        break;
                    }
                    case ELAPSED_MONTHS :
                    {
                        duration = duration.convertUnits(TimeUnit.MONTHS, m_projectFile.getProjectProperties());
                        break;
                    }
                    case ELAPSED_YEARS :
                    {
                        duration = duration.convertUnits(TimeUnit.YEARS, m_projectFile.getProjectProperties());
                        break;
                    }
                    default :
                    {
                        break;
                    }
                }
            }
            System.out.println("writeEstimate: 2 " + mpxjTask);
            planEstimate.setUnit(getUnitString(duration));
            String est = getEstimateString(duration);
            planEstimate.setExpected(est);
            planEstimate.setOptimistic(est);
            planEstimate.setPessimistic(est);
        }
    }

    /**
    * This method writes task progress data to a Plan file.
    *
    */
    private void writeProgress(Task mpxjTask, plan.schema.Task planTask)
    {
        System.out.println("writeProgress: " + mpxjTask);
        if (mpxjTask.getActualStart() == null)
        {
            Number completion = mpxjTask.getPercentageComplete();
            if (completion == null || completion.intValue() == 0)
            {
                return;
            }
            Progress planProgress = m_factory.createProgress();
            planTask.getTaskOrEstimateOrDocumentsOrTaskSchedulesOrProgress().add(planProgress);

            planProgress.setStarted("1");
            Date date = mpxjTask.getActualStart() == null ? mpxjTask.getStart() : mpxjTask.getActualStart();
            planProgress.setStartTime(getDateTimeString(date));
            if (mpxjTask.getActualFinish() != null || completion.intValue() == 100)
            {
                date = mpxjTask.getActualFinish() == null ? mpxjTask.getFinish() : mpxjTask.getActualFinish();
                planProgress.setFinished("1");
                planProgress.setFinishTime(getDateTimeString(date));
            }
            CompletionEntry planCompletionEntry = m_factory.createCompletionEntry();
            planProgress.getCompletionEntryOrUsedEffort().add(planCompletionEntry);

            planCompletionEntry.setDate(getDateTimeString(date));
            planCompletionEntry.setPercentFinished(getIntegerString(completion));

            //System.out.println("writeProgress: " + mpxjTask.getName() + " start: " + planProgress.getStartTime() + " date: " + date + " completion: " + planCompletionEntry.getPercentFinished() + '%');
        } else {
            //System.out.println("writeProgress: " + mpxjTask.getActualStart() + " - " + mpxjTask.getActualFinish());
            Progress planProgress = m_factory.createProgress();
            planTask.getTaskOrEstimateOrDocumentsOrTaskSchedulesOrProgress().add(planProgress);

            planProgress.setStarted("1");
            planProgress.setStartTime(getDateTimeString(mpxjTask.getActualStart()));
            if (mpxjTask.getActualFinish() != null)
            {
                planProgress.setFinished("1");
                planProgress.setFinishTime(getDateTimeString(mpxjTask.getActualFinish()));
            }
            writeCompletion(mpxjTask, planProgress);
        }
        //writeUsedEffort(mpxjTask, planProgress);
    }
    /**
    * This method writes task progress data to a Plan file.
    *
    */
    private void writeCompletion(Task mpxjTask, Progress planProgress)
    {
        planProgress.setEntrymode("EnterEffortPerTask");

        List<ResourceAssignment> assignments = mpxjTask.getResourceAssignments();
        //System.out.println("writeCompletion: " + assignments);
        if (assignments.isEmpty()) {
            CompletionEntry planCompletionEntry = m_factory.createCompletionEntry();
            planProgress.getCompletionEntryOrUsedEffort().add(planCompletionEntry);

            Date date = mpxjTask.getActualStart();
            if (mpxjTask.getActualFinish() != null) {
                date = mpxjTask.getActualFinish();
            }
            planCompletionEntry.setDate(getDateTimeString(date));
            planCompletionEntry.setPercentFinished(getIntegerString(mpxjTask.getPercentageWorkComplete()));
            Duration work = mpxjTask.getActualWork();//TODO + mpxjTask.getActualOvertimeWork();
            planCompletionEntry.setPerformedEffort(getDurationString(work));
            work = mpxjTask.getRemainingWork();//TODO + mpxjTask.getRemainingOvertimeWork();
            planCompletionEntry.setRemainingEffort(getDurationString(work));
            return;
        }

        // Looks like a resource cannot work on non-working days. Why?
        // Get working days
        List<Calendar> workdays = new ArrayList<>();
        List<CompletionEntry> entries = new ArrayList<>();
        List<Duration> efforts = new ArrayList<>();
        for (ResourceAssignment mpxjAssignment : assignments) {
            ProjectCalendar resourceCalendar = mpxjAssignment.getResource().getResourceCalendar();
            for (TimephasedWork tpw : mpxjAssignment.getTimephasedActualWork()) {
                //System.out.println("writeCompletion: " + mpxjAssignment.getResource() + " : " + tpw);
                Calendar date = getCalendarDate(tpw.getStart());
                Calendar end = getCalendarDate(tpw.getFinish());
                Duration amount = tpw.getAmountPerDay();
                for (; !date.after(end); date.add(Calendar.DAY_OF_MONTH, 1)) {
                    if (resourceCalendar.isWorkingDate(date.getTime())) {
                        int idx = -1;
                        for (int i = 0; i < workdays.size(); ++i) {
                            if (workdays.get(i).compareTo(date) == 0) {
                                idx = i;
                                break;
                            }
                        }
                        if (idx >= 0) {
                            //System.out.println("writeCompletion: date exists " + idx + " date " + date.getTime() + " list " + workdays.get(idx).getTime());
                            efforts.set(idx, Duration.add(efforts.get(idx), tpw.getAmountPerDay(), m_projectFile.getProjectProperties()));
                            //System.out.println("writeCompletion: " + date.getTime() + " exists, amount added " + efforts.get(idx));
                        } else {
                            //System.out.println("writeCompletion: date not exists " + idx + " date " + date.getTime());
                            workdays.add((Calendar)date.clone());
                            entries.add(m_factory.createCompletionEntry());
                            efforts.add(tpw.getAmountPerDay());
                            //System.out.println("writeCompletion: idx " + workdays.indexOf(date) + " " + date.getTime() + " new, amount set " + efforts.get(workdays.indexOf(date)));
                        }
                    }
                }
            }
        }
        for (int i = 0; i < workdays.size(); ++i) {
            Calendar date = workdays.get(i);
            Duration effort = efforts.get(i);
            CompletionEntry planCompletionEntry = entries.get(i);

            planProgress.getCompletionEntryOrUsedEffort().add(planCompletionEntry);

            planCompletionEntry.setDate(getDateTimeString(date.getTime()));
            planCompletionEntry.setPerformedEffort(getDurationString(effort));
            //System.out.println("writeCompletion: " + date.getTime() +" : " + planCompletionEntry.getDate() + " " + planCompletionEntry.getPerformedEffort() + " " + planCompletionEntry.getPerformedEffort());
        }
        Number percentageComplete = mpxjTask.getPercentageWorkComplete();
        if (percentageComplete.intValue() == 0) {
            percentageComplete = mpxjTask.getPercentageComplete();
        }
        if (percentageComplete.intValue() > 0) {
            // Add percent finished to last entry
            if (entries.isEmpty()) {
                entries.add(m_factory.createCompletionEntry());
                planProgress.getCompletionEntryOrUsedEffort().add(entries.get(0));
                //System.out.println("writeCompletion: No entries, added date=???");
            }
            entries.get(entries.size()-1).setPercentFinished(getIntegerString(percentageComplete));
            //System.out.println("writeCompletion: percent finished " + percentageComplete);
        }
    }
    /**
    * This method writes task used effort data to a Plan file.
    *
    */
    private void writeUsedEffort(Task mpxjTask, Progress planProgress)
    {
        planProgress.setEntrymode("EnterEffortPerResource");

        List<ResourceAssignment> resourceAssignments = mpxjTask.getResourceAssignments();
        if (resourceAssignments.isEmpty()) {
            return;
        }
        UsedEffort planUsedEffort = m_factory.createUsedEffort();
        planProgress.getCompletionEntryOrUsedEffort().add(planUsedEffort);

        for (ResourceAssignment mpxjAssignment : resourceAssignments) {
            Resource mpxjResource = mpxjAssignment.getResource();
            plan.schema.Resource planResource = m_factory.createResource();
            planUsedEffort.getResource().add(planResource);
            planResource.setId(getResourceId(mpxjResource));

            ProjectCalendar resourceCalendar = mpxjResource.getResourceCalendar();
            for (TimephasedWork tpw : mpxjAssignment.getTimephasedActualWork()) {
                //System.out.println("writeUsedEffort: " + mpxjAssignment.getResource() + " : " + tpw);
                Calendar date = getCalendarDate(tpw.getStart());
                Calendar end = getCalendarDate(tpw.getFinish());
                Duration amount = tpw.getAmountPerDay();
                for (; !date.after(end); date.add(Calendar.DAY_OF_MONTH, 1)) {
                    // Looks like a resource cannot work on non-working days?
                    if (resourceCalendar.isWorkingDate(date.getTime())) {
                        ActualEffort planActualEffort = m_factory.createActualEffort();
                        planResource.getActualEffort().add(planActualEffort);

                        planActualEffort.setDate(getDateTimeString(date.getTime()));
                        planActualEffort.setNormalEffort(getDurationString(amount));
                        //System.out.println("writeUsedEffort: " + date.getTime() +" : " + planCompletionEntry.getDate() + " " + planCompletionEntry.getPerformedEffort());
                    } else {
                        System.out.println("writeUsedEffort: " + mpxjAssignment.getResource() + "  Not working on " + date.getTime());
                    }
                }
            }
        }
    }
//     /**
//     * This method writes resource actual effort data to a Plan file.
//     *
//     */
//     private void writeActualEffort(ResourceAssignment mpxjAssignment, plan.schema.Resource planResource)
//     {
//         if (mpxjAssignment.getActualStart() == null)
//         {
//             return;
//         }
//         ActualEffort planActualEffort = m_factory.createActualEffort();
//         planResource.getActualEffort().add(planActualEffort);
//
//         List<TimephasedResourceAssignment> list = mpxjAssignment.getTimephasedComplete();
//         if (list.isEmpty())
//         {
//             Date date = mpxjAssignment.getActualStart();
//             if (mpxjAssignment.getActualFinish() != null)
//             {
//                 date = mpxjAssignment.getActualFinish();
//             }
//             planActualEffort.setDate(getDateTimeString(date));
//             planActualEffort.setOvertimeEffort(getDurationString(mpxjAssignment.getOvertimeWork()));
//             planActualEffort.setNormalEffort(getDurationString(mpxjAssignment.getActualWork()));
//         }
//         else
//         {
//             for (TimephasedResourceAssignment mpxjTpa : list)
//             {
//                 writeActualEffort(mpxjTpa, planActualEffort);
//             }
//         }
//     }
//     /**
//     * This method writes resource actual effort data to a Plan file.
//     *
//     */
//     private void writeActualEffort(TimephasedResourceAssignment mpxjTpa, ActualEffort planActualEffort)
//     {
//         Date currentDate = DateUtility.getDayStartDate(mpxjTpa.getStart());
//         long end = DateUtility.getDayEndDate(mpxjTpa.getFinish()).getTime();
//         while (currentDate.getTime() < end)
//         {
//             planActualEffort.setDate(getDateString(currentDate));
//             planActualEffort.setNormalEffort(getDurationString(mpxjTpa.getWorkPerDay()));
//             currentDate = addDays(currentDate, 1);
//         }
//     }

    /**
    * This method writes task scheduling data to a Plan file.
    *
    */
    private void writeTaskSchedules(Task mpxjTask, plan.schema.Task planTask)
    {
        TaskSchedules planSchedules = m_factory.createTaskSchedules();
        planTask.getTaskOrEstimateOrDocumentsOrTaskSchedulesOrProgress().add(planSchedules);
        writeTaskSchedule(mpxjTask, planSchedules);

    }

    /**
    * This method writes task scheduling data to a Plan file.
    *
    */
    private void writeTaskSchedule(Task mpxjTask, TaskSchedules planSchedules)
    {
        TaskSchedule planSchedule = m_factory.createTaskSchedule();
        planSchedules.getTaskSchedule().add(planSchedule);

        planSchedule.setId("1");
        planSchedule.setName("Plan");
        planSchedule.setType("Expected");
        planSchedule.setStart(getDateTimeString(mpxjTask.getStart()));
        planSchedule.setEnd(getDateTimeString(mpxjTask.getFinish()));
        planSchedule.setDuration(getDurationString(mpxjTask.getStart(), mpxjTask.getFinish()));

        planSchedule.setEarlystart(getDateTimeString(mpxjTask.getEarlyStart()));
        planSchedule.setEarlyfinish(getDateTimeString(mpxjTask.getEarlyFinish()));
        planSchedule.setLatestart(getDateTimeString(mpxjTask.getLateStart()));
        planSchedule.setLatefinish(getDateTimeString(mpxjTask.getLateFinish()));

        planSchedule.setPositiveFloat(getDurationString(mpxjTask.getTotalSlack()));
//         planSchedule.setNegativeFloat();
        planSchedule.setFreeFloat(getDurationString(mpxjTask.getFreeSlack()));
//         planSchedule.setStartFloat(getDurationString(mpxjTask.getStartSlack()));
//         planSchedule.setFinishFloat(getDurationString(mpxjTask.getFinishSlack()));

        planSchedule.setNotScheduled("0");
        planSchedule.setSchedulingConflict("0");
        planSchedule.setResourceError("0");
        planSchedule.setResourceOverbooked("0");
        planSchedule.setResourceNotAvailable("0");
        planSchedule.setInCriticalPath("0");
    }

    /**
    * This method writes task relations to a Plan file.
    */
    private void writeRelations()
    {
        //System.out.println("writeRelations:");
        Relations planRelations = m_factory.createRelations();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(planRelations);

        for(Task mpxjTask : m_projectFile.getTasks())
        {
            List<net.sf.mpxj.Relation> predecessors = mpxjTask.getPredecessors();
            if (predecessors != null)
            {
                for (net.sf.mpxj.Relation rel : predecessors)
                {
                    //System.out.println("writeRelation: " + rel);
                    Task child = rel.getSourceTask();
                    if (child.getName() == null || child.getUniqueID() == 0) {
                        System.out.println("writeRelations: Bogus child task? " + child);
                    }
                    Task parent = rel.getTargetTask();
                    if (parent.getName() == null || parent.getUniqueID() == 0) {
                        System.out.println("writeRelations: Bogus parent task? " + parent);
                    }
                    plan.schema.Relation planRelation = m_factory.createRelation();
                    planRelation.setParentId(getTaskId(parent));
                    planRelation.setChildId(getTaskId(child));
                    planRelation.setLag(getDurationString(rel.getLag()));
                    planRelation.setType(RELATIONSHIP_TYPES.get(rel.getType()));
                    planRelations.getRelation().add(planRelation);
                }
            }
        }
        //System.out.println("writeRelations: end");
    }

    /**
    * This method writes scheduling data to a Plan file.
    *
    */
    private void writeProjectSchedules()
    {
        ProjectSchedules planProjectSchedules = m_factory.createProjectSchedules();
        m_planProject.getProjectSettingsOrAccountsOrCalendarsOrResourceGroupsOrResourcesOrResourceGroupRelationsOrTasksOrRelationsOrProjectSchedulesOrResourceTeamsOrExternalAppointmentsOrResourceRequestsOrRequiredResourceRequestsOrAlternativeRequests().add(planProjectSchedules);

        writeScheduleManagement(planProjectSchedules);

    }

    /**
    * This method writes schedule managnebt data to a Plan file.
    *
    */
    private void writeScheduleManagement(ProjectSchedules planProjectSchedules)
    {
        ScheduleManagement planScheduleManagement = m_factory.createScheduleManagement();
        planProjectSchedules.getScheduleManagement().add(planScheduleManagement);

        planScheduleManagement.setId("1");
        planScheduleManagement.setName("Plan");

        writeProjectSchedule(planScheduleManagement);
    }

    /**
    * This method writes scheduling data to a Plan file.
    *
    */
    private void writeProjectSchedule(ScheduleManagement planScheduleManagement)
    {

        ProjectSchedule planProjectSchedule = m_factory.createProjectSchedule();
        planScheduleManagement.getScheduleManagementOrProjectSchedule().add(planProjectSchedule);

        planProjectSchedule.setId("1");
        planProjectSchedule.setName("Plan");
        planProjectSchedule.setType("Expected");
        planProjectSchedule.setNotScheduled("0");
        planProjectSchedule.setSchedulingError("0");
        planProjectSchedule.setSchedulingConflict("0");

        ProjectProperties projectProperties = m_projectFile.getProjectProperties();
        planProjectSchedule.setStart(getDateTimeString(projectProperties.getStartDate()));
        planProjectSchedule.setEnd(getDateTimeString(projectProperties.getFinishDate()));
        planProjectSchedule.setDuration(getDurationString(projectProperties.getDuration()));

        writeAppointments(planProjectSchedule);
    }

    /**
    * This method writes appointment data to a Plan file.
    *
    */
    private void writeAppointments(ProjectSchedule planProjectSchedule)
    {
        Date start = null;
        Date finish = null;
        Map<Resource,List<Task>> map = new HashMap<Resource,List<Task>>();
        ResourceAssignmentContainer assignments = m_projectFile.getResourceAssignments();
        for (int i = 0; i < assignments.size(); ++i)
        {
            ResourceAssignment mpxjAssignment = assignments.get(i);
            Task task = mpxjAssignment.getTask();
            Resource resource = mpxjAssignment.getResource();
            if (task != null && resource != null) {
                Appointment planAppointment = m_factory.createAppointment();
                planProjectSchedule.getCriticalpathListOrAppointment().add(planAppointment);

                planAppointment.setTaskId(getTaskId(task));
                planAppointment.setResourceId(getResourceId(resource));
                writeIntervals(planAppointment, mpxjAssignment);
            }
            if (start == null || task.getStart().before(start))
            {
                start = task.getStart();
            }
            if (finish == null || finish.before(task.getFinish()))
            {
                finish = task.getFinish();
            }
        }
        if (start == null)
        {
            start = m_projectFile.getStartDate();
        }
        if (finish == null)
        {
            finish = m_projectFile.getFinishDate();
        }
        planProjectSchedule.setStart(getDateTimeString(start));
        planProjectSchedule.setEnd(getDateTimeString(finish));
        planProjectSchedule.setDuration(getDurationString(start, finish));
    }

    /**
    * This method writes appointment intervals to a Plan file.
    *
    */
    private void writeIntervals(Appointment planAppointment, ResourceAssignment mpxjAssignment)
    {
        System.out.println("writeIntervals: Assignment: "+mpxjAssignment);
        if (mpxjAssignment.getStart() == null) {
            System.out.println("writeIntervals: Assignment has no start time: " + mpxjAssignment);
            return;
        }
        if (mpxjAssignment.getFinish() == null) {
            // TODO handle duration instead
            System.out.println("writeIntervals: Assignment has no finish time " + mpxjAssignment);
            return;
        }
        Calendar startDateTime = Calendar.getInstance();
        startDateTime.setTime(mpxjAssignment.getStart());
        Calendar endDateTime = Calendar.getInstance();
        endDateTime.setTime(mpxjAssignment.getFinish());

        ProjectCalendar calendar = mpxjAssignment.getCalendar();
        Calendar currentDate = Calendar.getInstance();
        currentDate.clear();
        currentDate.set(startDateTime.get(Calendar.YEAR), startDateTime.get(Calendar.MONTH), startDateTime.get(Calendar.DAY_OF_MONTH));

        while (currentDate.before(endDateTime))
        {
            //System.out.println("writeIntervals: Date: " + currentDate.getTime());
            Day day = Day.getInstance(currentDate.get(Calendar.DAY_OF_WEEK));
            ProjectCalendarDateRanges ranges = calendar.getHours(day);
            Boolean incDate = true;
            //System.out.println("writeIntervals: ranges: " + ranges);
            for (DateRange range : ranges)
            {
                Calendar rangeStart = Calendar.getInstance();
                rangeStart.setTime(getTime(range.getStart()));
                rangeStart.set(currentDate.get(Calendar.YEAR), currentDate.get(Calendar.MONTH), currentDate.get(Calendar.DAY_OF_MONTH));
                Calendar rangeEnd = Calendar.getInstance();
                rangeEnd.setTime(getTime(range.getEnd()));
                rangeEnd.set(currentDate.get(Calendar.YEAR), currentDate.get(Calendar.MONTH), currentDate.get(Calendar.DAY_OF_MONTH));
                if (rangeEnd.before(rangeStart)) {
                    rangeEnd.add(Calendar.DAY_OF_MONTH, 1);
                    if (!isMidnight(rangeEnd)) {
                        System.out.println("writeIntervals: Invalid. Range spans midnight: " + rangeStart.getTime() + " - " + rangeEnd.getTime());
                        continue;
                    }
                }
                writeInterval(planAppointment, rangeStart, rangeEnd, mpxjAssignment.getUnits());
            }
            currentDate.add(Calendar.DAY_OF_MONTH, 1);
        }
        //System.out.println("writeIntervals: end");
    }

    /**
    * @range is a time range without date info
    * @return end date
    */
    private void writeInterval(Appointment planAppointment, Calendar start, Calendar end, Number units)
    {
        AppointmentInterval interval = m_factory.createAppointmentInterval();
        planAppointment.getAppointmentInterval().add(interval);

        interval.setStart(getDateTimeString(start.getTime()));
        interval.setEnd(getDateTimeString(end.getTime()));
        interval.setLoad(getIntegerString(units));
        //System.out.println("writeInterval: " + interval.getStart() + " - " + interval.getEnd() + " : " + interval.getLoad());
    }

    private Boolean isMidnight(Calendar dt)
    {
        return dt.get(Calendar.HOUR) == 0 || dt.get(Calendar.MINUTE) == 0 || dt.get(Calendar.SECOND) == 0;
    }

    private Date getTime(Date dt)
    {
        Date epoch = new Date();
        epoch.setTime(0);
        Calendar c = Calendar.getInstance();
        c.setTime(epoch);
        c.set(Calendar.HOUR, dt.getHours());
        c.set(Calendar.MINUTE, dt.getMinutes());
        c.set(Calendar.SECOND, dt.getSeconds());
        return c.getTime();
    }

    private Calendar getCalendarDate(Date dt)
    {
        Calendar c = Calendar.getInstance();
        c.setTime(dt);
        c.set(Calendar.HOUR, 0);
        c.set(Calendar.MINUTE, 0);
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);
        return c;
    }
//     /**
//     * Convert a Plan date-time value into a Java date.
//     *
//     * 20070222T080000Z
//     *
//     * @param value Plan date-time
//     * @return Java Date instance
//     */
//     private String getDateTime(Date value)
//     {
//         StringBuffer result = new StringBuffer(16);
//
//         if (value != null)
//         {
//             Calendar cal = Calendar.getInstance();
//             cal.setTime(value);
//
//             result.append(m_fourDigitFormat.format(cal.get(Calendar.YEAR)));
//             result.append(m_twoDigitFormat.format(cal.get(Calendar.MONTH) + 1));
//             result.append(m_twoDigitFormat.format(cal.get(Calendar.DAY_OF_MONTH)));
//             result.append("T");
//             result.append(m_twoDigitFormat.format(cal.get(Calendar.HOUR_OF_DAY)));
//             result.append(m_twoDigitFormat.format(cal.get(Calendar.MINUTE)));
//             result.append(m_twoDigitFormat.format(cal.get(Calendar.SECOND)));
//             result.append("Z");
//         }
//
//         return (result.toString());
//     }

   /**
    * Convert an Integer value into a String.
    *
    * @param value Integer value
    * @return String value
    */
   private String getIntegerString(Number value)
   {
      return (value == null ? null : Integer.toString(value.intValue()));
   }

   /**
    * Convert an int value into a String.
    *
    * @param value int value
    * @return String value
    */
   private String getIntegerString(int value)
   {
      return (Integer.toString(value));
   }

//    /**
//     * Used to determine if a particular day of the week is normally
//     * a working day.
//     *
//     * @param mpxjCalendar ProjectCalendar instance
//     * @param day Day instance
//     * @return boolean flag
//     */
//    private boolean isWorkingDay(ProjectCalendar mpxjCalendar, Day day)
//    {
//       boolean result = false;
//
//       switch (mpxjCalendar.getWorkingDay(day))
//       {
//          case WORKING :
//          {
//             result = true;
//             break;
//          }
//
//          case NON_WORKING :
//          {
//             result = false;
//             break;
//          }
//
//          case DEFAULT :
//          {
//             result = isWorkingDay(mpxjCalendar.getBaseCalendar(), day);
//             break;
//          }
//       }
//
//       return (result);
//    }

   /**
    * Returns a flag represented as a String, indicating if
    * the supplied day is a working day.
    *
    * @param mpxjCalendar MPXJ ProjectCalendar instance
    * @param day Day instance
    * @return boolean flag as a string
    */
   private String getWorkingDayString(ProjectCalendar mpxjCalendar, Day day)
   {
      String result = null;

      switch (mpxjCalendar.getWorkingDay(day))
      {
         case WORKING :
         {
            result = "2";
            break;
         }

         case NON_WORKING :
         {
            result = "1";
            break;
         }

         case DEFAULT :
         {
            result = "0";
            break;
         }
      }

      return (result);
   }

    /**
    * Convert a Java date into a Plan time.
    *
    * 0800
    *
    * @param value Java Date instance
    * @return Plan time value
    */
    private String getTimeString(Date value)
    {
        Calendar cal = Calendar.getInstance();
        cal.setTime(value);
        int hours = cal.get(Calendar.HOUR_OF_DAY);
        int minutes = cal.get(Calendar.MINUTE);
        int seconds = cal.get(Calendar.SECOND);

        StringBuffer sb = new StringBuffer(8);
        sb.append(m_twoDigitFormat.format(hours));
        sb.append(':');
        sb.append(m_twoDigitFormat.format(minutes));
        sb.append(':');
        sb.append(m_twoDigitFormat.format(seconds));

        return (sb.toString());
    }

    /**
    * Convert a Java date into a Plan date.
    *
    * 20070222
    *
    * @param value Java Date instance
    * @return Plan date
    */
    private String getDateString(Date value)
    {
        String format = "yyyy-MM-dd";
        SimpleDateFormat df = new SimpleDateFormat(format);
        return df.format(value);
    }

    /**
    * Convert a Java date into a Plan date-time string.
    *
    * 20070222T080000Z
    *
    * @param value Java date
    * @return Plan date-time string
    */
    private String getDateTimeString(Date value)
    {
        if (value == null) {
            //System.out.println("getDateTimeString: null date");
            return new String();
        }
        String format = "yyyy-MM-dd'T'HH:mm:ssZ";
        SimpleDateFormat df = new SimpleDateFormat(format);
        return df.format(value);
    }

    private String getMinutesString(long value)
    {
        long min = value % 60;
        long hours = value / 60;
        return Long.toString(hours) + 'h' + Long.toString(min) + 'm';
    }

/**
    * Converts the duration in milliseconds into a string
    *
    * Plan represents durations as a number of milliseconds
    * formatted as a bb:cc:dd.e where:
    * a = days, bb = hours, cc = minutes, dd = seconds, e = milliseconds
    */
    private String getDurationString(long value)
    {
        String result;
        long ms = value % 1000;
        value /= 1000;
        long sec = value % 60;
        value /= 60;
        long min = value % 60;
        value /= 60;
        long hour = value % 24;
        value /= 24;
        long day = value;
        result = Long.toString(day) + ' ' + Long.toString(hour) + ':' + Long.toString(min) + ':' + Long.toString(sec) + '.' + Long.toString(ms);
        return result;
    }

    /**
    * Converts the duration between two dates into a string
    */
    private String getDurationString(Date start, Date end)
    {
        if (start == null || end == null) {
            return getDurationString(0);
        }
        return getDurationString(end.getTime() - start.getTime());
    }

    /**
    * Converts an MPXJ Duration instance into the string representation
    * of a Plan duration.
    */
    private String getDurationString(Duration value)
    {
        double seconds = 0;

        if (value != null)
        {

            switch (value.getUnits())
            {
            case MINUTES :
            case ELAPSED_MINUTES :
            {
                seconds = value.getDuration() * 60;
                break;
            }

            case HOURS :
            case ELAPSED_HOURS :
            {
                seconds = value.getDuration() * (60 * 60);
                break;
            }

            case DAYS :
            {
                double minutesPerDay = m_projectFile.getProjectProperties().getMinutesPerDay().doubleValue();
                seconds = value.getDuration() * (minutesPerDay * 60);
                break;
            }

            case ELAPSED_DAYS :
            {
                seconds = value.getDuration() * (24 * 60 * 60);
                break;
            }

            case WEEKS :
            {
                double minutesPerWeek = m_projectFile.getProjectProperties().getMinutesPerWeek().doubleValue();
                seconds = value.getDuration() * (minutesPerWeek * 60);
                break;
            }

            case ELAPSED_WEEKS :
            {
                seconds = value.getDuration() * (7 * 24 * 60 * 60);
                break;
            }

            case MONTHS :
            {
                double minutesPerDay = m_projectFile.getProjectProperties().getMinutesPerDay().doubleValue();
                double daysPerMonth = m_projectFile.getProjectProperties().getDaysPerMonth().doubleValue();
                seconds = value.getDuration() * (daysPerMonth * minutesPerDay * 60);
                break;
            }

            case ELAPSED_MONTHS :
            {
                seconds = value.getDuration() * (30 * 24 * 60 * 60);
                break;
            }

            case YEARS :
            {
                double minutesPerDay = m_projectFile.getProjectProperties().getMinutesPerDay().doubleValue();
                double daysPerMonth = m_projectFile.getProjectProperties().getDaysPerMonth().doubleValue();
                seconds = value.getDuration() * (12 * daysPerMonth * minutesPerDay * 60);
                break;
            }

            case ELAPSED_YEARS :
            {
                seconds = value.getDuration() * (365 * 24 * 60 * 60);
                break;
            }

            default :
            {
                break;
            }
            }
        }
        return getDurationString((long)(seconds*1000));
    }

    /**
    * Converts an MPXJ Duration instance into the string representation
    * of a Plan estimate.
    * If type is FIXED_WORK (or FIXED_UNITS) estimate must be in working time
    * If type is FIXED_DURATION estimate must be in calendar time
    */
    private String getEstimateString(Duration value)
    {
        double v =  value.getDuration();
        return "" + v;
    }
    /**
    * Converts an MPXJ Duration instance into the string representation
    * of a Plan unit.
    */
    private String getUnitString(Duration value)
    {
        String unit = "d";
        if (value != null)
        {
            switch (value.getUnits())
            {
                case MINUTES :
                case ELAPSED_MINUTES :
                {
                    unit = "m";
                    break;
                }
                case HOURS :
                case ELAPSED_HOURS :
                {
                    unit = "h";
                    break;
                }
                case DAYS :
                case ELAPSED_DAYS :
                {
                    unit = "d";
                    break;
                }
                case WEEKS :
                case ELAPSED_WEEKS :
                {
                    unit = "w";
                    break;
                }
                case MONTHS :
                case ELAPSED_MONTHS :
                {
                    unit = "M";
                    break;
                }
                case YEARS :
                case ELAPSED_YEARS :
                {
                    unit = "Y";
                    break;
                }
                default :
                {
                    break;
                }
            }
        }
        return unit;
    }

    /**
    * Convert a string representation of the task type
    * into a TaskType instance.
    *
    * @param value string value
    * @return TaskType value
    */
    private String getEstimateType(TaskType value)
    {
        String result = "Effort";
        if (value != null && value == TaskType.FIXED_DURATION)
        {
            result = "Duration";
        }
        return (result);
    }

    /**
    * Writes a string value, ensuring that null is mapped to an empty string.
    *
    * @param value string value
    * @return string value
    */
    private String getString(String value)
    {
        return (value == null ? "" : value);
    }

    /**
    * Gets the project id, using GUID if available else unique id
    */
    private String getProjectId()
    {
        String id;
        ProjectProperties properties = m_projectFile.getProjectProperties();
        if (properties.getGUID() != null) {
            id = properties.getGUID().toString();
        } else {
            id = getIntegerString(properties.getUniqueID());
        }
        if (id == null) {
            // Fallback to title
            id = properties.getProjectTitle();
        }
        return id;
    }

    /**
    * Gets the resource id, using GUID if available else unique id
    */
    private String getResourceId(Resource resource)
    {
        return resource.getGUID() == null
                    ? getIntegerString(resource.getUniqueID())
                    : resource.getGUID().toString();
    }

    /**
    * Gets the task id, using GUID if available else unique id
    */
    private String getTaskId(Task task)
    {
        return task.getGUID() == null
                    ? getIntegerString(task.getUniqueID())
                    : task.getGUID().toString();
    }

    /**
    * Gets the scheduling constraint
    */
    private String getScheduleFromString(ScheduleFrom value)
    {
        String result = "MustStartOn";
        if (value != null && value == ScheduleFrom.FINISH) {
            result = "MustFinishOn";
        }
        return result;
    }

    /**
    * Gets the scheduling constraint
    */
    private void setScheduling(Task mpxjTask, plan.schema.Task planTask)
    {
        ConstraintType mpjxConstraintType = mpxjTask.getConstraintType();
        if (mpjxConstraintType == null) {
            planTask.setScheduling("ASAP");
        }
        else
        {
            switch (mpjxConstraintType)
            {
                case AS_SOON_AS_POSSIBLE :
                    planTask.setScheduling("ASAP");
                    break;
                case AS_LATE_AS_POSSIBLE :
                    planTask.setScheduling("ALAP");
                    break;
                case MUST_START_ON :
                    planTask.setScheduling("MustStartOn");
                    planTask.setConstraintStarttime(getDateTimeString(mpxjTask.getConstraintDate()));
                    break;
                case MUST_FINISH_ON :
                    planTask.setScheduling("MustFinishOn");
                    planTask.setConstraintEndtime(getDateTimeString(mpxjTask.getConstraintDate()));
                    break;
                case START_NO_EARLIER_THAN :
                    planTask.setScheduling("StartNotEarlier");
                    planTask.setConstraintStarttime(getDateTimeString(mpxjTask.getConstraintDate()));
                    break;
                case START_NO_LATER_THAN :
                    planTask.setScheduling("ASAP"); // Note: not supported
                    planTask.setConstraintStarttime(getDateTimeString(mpxjTask.getConstraintDate()));
                    break;
                case FINISH_NO_EARLIER_THAN :
                    planTask.setScheduling("ALAP"); // Note: not supported
                    planTask.setConstraintEndtime(getDateTimeString(mpxjTask.getConstraintDate()));
                    break;
                case FINISH_NO_LATER_THAN :
                    planTask.setScheduling("FinishNotLater");
                    planTask.setConstraintEndtime(getDateTimeString(mpxjTask.getConstraintDate()));
                    break;
                default:
                    break;
            }
        }
    }

    /**
    * Given a date represented by a Date instance, set the time
    * component of the date based on the hours and minutes of the
    * time supplied by the Date instance.
    *
    * @param date Date instance representing the date
    * @param time Date instance representing the time of day
    * @return new Date instance with the required time set
    */
    private Date setTime(Date date, Date time)
    {
        Date result;
        if (date == null) {
            return date;
        }
        if (time == null)
        {
            result = date;
        }
        else
        {
            Calendar cal = Calendar.getInstance();
            cal.setTime(time);
            cal.set(Calendar.DAY_OF_YEAR, 1);
            cal.set(Calendar.YEAR, 1970);
            cal.set(Calendar.MILLISECOND, 0);
            Date canonicalTime = cal.getTime();
            //System.out.println("setTime: " + canonicalTime);

            String ds = getDateString(date);
            SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd");
            ParsePosition pos = new ParsePosition(0);
            result = df.parse(ds, pos);
            long offset = canonicalTime.getTime();
            //System.out.println("setTime: " + ds + result);
            result = new Date(result.getTime() + offset);
        }
        return result;
    }

//     /**
//     * Add days to date
//     */
//     public Date addDays(Date date, int days)
//     {
//         Calendar cal = Calendar.getInstance();
//         cal.setTime(date);
//         cal.add(Calendar.DATE, days);
//         return cal.getTime();
//     }
//
//     /**
//     * Set the encoding used to write the file. By default UTF-8 is used.
//     *
//     * @param encoding encoding name
//     */
//     public void setEncoding(String encoding)
//     {
//         m_encoding = encoding;
//     }
//
//     /**
//     * Retrieve the encoding used to write the file. If this value is null,
//     * UTF-8 is used.
//     *
//     * @return encoding name
//     */
//     public String getEncoding()
//     {
//         return m_encoding;
//     }
//
    private String m_encoding;
    private ProjectFile m_projectFile;
    private ObjectFactory m_factory;
    private Plan m_plan;
    private Project m_planProject;
    private ResourceGroups m_resourcegroups;
    private Map<String, String> m_groups = new HashMap<String, String>(); // <group id, resource id>

    private NumberFormat m_twoDigitFormat = new DecimalFormat("00");
    private NumberFormat m_fourDigitFormat = new DecimalFormat("0000");

    private static Map<RelationType, String> RELATIONSHIP_TYPES = new HashMap<RelationType, String>();
    static
    {
        RELATIONSHIP_TYPES.put(RelationType.FINISH_FINISH, "Finish-Finish");
        RELATIONSHIP_TYPES.put(RelationType.FINISH_START, "Finish-Start");
        RELATIONSHIP_TYPES.put(RelationType.START_FINISH, "Finish-Start"); //Note: START_FINISH not supported
        RELATIONSHIP_TYPES.put(RelationType.START_START, "Start-Start");
    }

    /**
    * Cached context to minimise construction cost.
    */
    private static JAXBContext CONTEXT;

    /**
    * Note any error occurring during context construction.
    */
    private static JAXBException CONTEXT_EXCEPTION;

    static
    {
        try
        {
            //
            // JAXB RI property to speed up construction
            //
            System.setProperty("com.sun.xml.bind.v2.runtime.JAXBContextImpl.fastBoot", "true");

            //
            // Construct the context
            //
            CONTEXT = JAXBContext.newInstance("plan.schema", PlanWriter.class.getClassLoader());
        }

        catch (JAXBException ex)
        {
            CONTEXT_EXCEPTION = ex;
            CONTEXT = null;
        }
    }
}
