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

import java.util.Calendar;
import java.util.Calendar;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;

import net.sf.mpxj.MPXJException;
import net.sf.mpxj.ProjectFile;
import net.sf.mpxj.reader.ProjectReader;
import net.sf.mpxj.reader.UniversalProjectReader;

import plan.PlanWriter;

public class Convert {

    public static void main(String[] args) {
        System.out.println("Convert");
        try
        {
            System.out.println("Convert args: " + args.length);
            if (args.length < 1 || args.length > 2)
            {
                System.out.println("Usage: Convert <input file name> [<plan file name>]");
            }
            else
            {
                System.out.println("Reading input file started: " + args[0]);
                long start = System.currentTimeMillis();

                Convert convert = new Convert();

                ProjectFile projectFile = convert.readFile(args[0]);
                long elapsed = System.currentTimeMillis() - start;
                System.out.println("Reading input file completed in " + elapsed + "ms.");

                if (args.length == 2)
                {
                    System.out.println("Writing Plan output file started: " +  args[1]);
                    PlanWriter w = new PlanWriter();
                    w.write(projectFile, args[1]);
                    System.out.println("Writing Plan output file completed.");
                }
            }
        }
        catch (Exception ex)
        {
            System.out.println("Exception");
            ex.printStackTrace(System.out);
        }
    }

    /**
    * Use the universal project reader to open the file.
    * Throw an exception if we can't determine the file type.
    *
    * @param inputFile file name
    * @return ProjectFile instance
    */
    public ProjectFile readFile(String inputFile) throws MPXJException
    {
        ProjectReader reader = new UniversalProjectReader();
        ProjectFile projectFile = reader.read(inputFile);
        if (projectFile == null)
        {
            throw new IllegalArgumentException("Unsupported file type");
        }
        return projectFile;
    }

}
