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


import java.io.File;
import java.util.Calendar;
import java.util.Calendar;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.io.PrintStream;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;

import net.sf.mpxj.MPXJException;
import net.sf.mpxj.ProjectFile;
import net.sf.mpxj.reader.ProjectReader;
import net.sf.mpxj.reader.UniversalProjectReader;
import net.sf.mpxj.Task;
import net.sf.mpxj.Resource;
import net.sf.mpxj.ProjectCalendar;
import net.sf.mpxj.ResourceAssignment;
import net.sf.mpxj.TimephasedWork;
import net.sf.mpxj.SubProject;

import plan.PlanWriter;

public class TestConvert {

    public static void main(String[] args)
    {
        System.out.println("TestConvert");
        try
        {
            if (args.length < 2)
            {
                System.out.println("Usage: TestConvert <test data directory name> <test result directory name> [<log file name>]");
            }
            else
            {
                if (args.length == 3)
                {
                    System.setOut(new PrintStream(new BufferedOutputStream(new FileOutputStream(args[2]))));
                }
                File dir = new File(args[0]);
                test(dir, args[1]);
            }
        }

        catch (Exception ex)
        {
            System.out.println("Exception");
            ex.printStackTrace(System.out);
        }
    }

    private static void test(File datadir, String resultdir) throws Exception
    {
        String[] paths = datadir.list();

        System.out.println("Directory: " + datadir.getCanonicalPath());
        for (String fname : paths)
        {
            try
            {
                System.out.println();
                File f = new File(datadir, fname);
                if (f.isDirectory()) {
                    test(f, resultdir);
                    continue;
                }
                if (fname.endsWith(".mpd"))
                {
                    System.out.println(".mpd file not supported: " + fname);
                    continue;
                }
                String infile = datadir.getCanonicalPath() + '/' + fname;
                System.out.println("Testing input file: " + infile);
                int i = fname.lastIndexOf​('.');
                if (i < 0)
                {
                    System.out.println("File must have an extension: " + fname);
                    continue;
                }
                String filename = fname.substring(0, i);
                String outfile = resultdir + '/' + filename + ".plan";
                System.out.println("File name: " + filename);

                ProjectFile projectFile = TestConvert.readFile(infile);

                printStatistics(projectFile);

                System.out.println("Writing Plan output file: " +  outfile);
                PlanWriter w = new PlanWriter();
                w.write(projectFile, outfile);
                System.out.println("Test completed.");
            }
            catch (Exception ex)
            {
                System.out.println("FAIL: TestConvert");
                ex.printStackTrace(System.out);
            }
        }
    }

    /**
    * Use the universal project reader to open the file.
    * Throw an exception if we can't determine the file type.
    *
    * @param inputFile file name
    * @return ProjectFile instance
    */
    public static ProjectFile readFile(String inputFile) throws MPXJException
    {
        ProjectReader reader = new UniversalProjectReader();
        ProjectFile projectFile = reader.read(inputFile);
        if (projectFile == null)
        {
            throw new IllegalArgumentException("Unsupported file type");
        }
        return projectFile;
    }

    private static void printStatistics(ProjectFile file)
    {
        System.out.println();
        System.out.println("Project statistics:");
        System.out.println("    Title:          " + file.getProjectProperties().getProjectTitle());
        System.out.println("    Language:       " + file.getProjectProperties().getLanguage());
        System.out.println("    Calendars:      " + file.getCalendars().size());
        System.out.println("    Tasks:          " + file.getTasks().size());
        boolean taskcalendar = false;
        boolean relations = false;
        for (Task t : file.getTasks()) {
            if (!taskcalendar && t.getCalendar() != null) {
                taskcalendar = true;
            }
            if (!relations && (!t.getPredecessors().isEmpty() || !t.getSuccessors().isEmpty())) {
                relations = true;
            }
        }
        if (taskcalendar) {
            System.out.println("        Has Task calendar");
        }
        if (relations) {
            System.out.println("        Has Task relations");
        }

        System.out.println("    Groups:         " + file.getGroups().size());
        System.out.println("    Resources:      " + file.getResources().size());
        System.out.println("    Assignments:    " + file.getResourceAssignments().size());

        int planned = 0;
        int complete = 0;
        int overtime = 0;
        for (Resource r : file.getResources()) {
            for (ResourceAssignment assignment : r.getTaskAssignments()) {
                if (planned == 0) {
                    List<TimephasedWork> tpw = assignment.getTimephasedWork();
                    planned = tpw != null ? tpw.size() : 0;
                    if (planned > 0) {
                        System.out.println("        Has Planned time phased assignments");
                    }
                }
                if (overtime == 0) {
                    List<TimephasedWork> tpw = assignment.getTimephasedOvertimeWork();
                    overtime = tpw != null ? tpw.size() : 0;
                    if (overtime > 0) {
                        System.out.println("        Has Overtime time phased assignments");
                    }
                }
                if (complete == 0) {
                    List<TimephasedWork> tpw = assignment.getTimephasedActualWork();
                    complete = tpw != null ? tpw.size() : 0;
                    if (complete > 0) {
                        System.out.println("        Has Complete time phased assignments");
                    }
                }
                if (planned > 0 && complete > 0 && overtime == 0) {
                    break;
                }
            }
            if (planned > 0 && complete > 0 && overtime > 0) {
                break;
            }
        }
        for (ProjectFile pf : file.getBaselines()) {
            if (pf != null) {
                System.out.println("    Baseline " + pf);
                TestConvert.printStatistics(pf);
            }
        }
        System.out.println("    Sub-projects:   " + file.getSubProjects().size());
        for (SubProject p : file.getSubProjects()) {
            System.out.println("    Sub-project:   " + p);
        }
        System.out.println();
   }

}
