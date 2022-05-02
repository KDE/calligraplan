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
import java.util.ArrayList;
import java.util.Arrays;

import net.sf.mpxj.MPXJException;
import net.sf.mpxj.ProjectFile;
import net.sf.mpxj.reader.ProjectReader;
import net.sf.mpxj.reader.UniversalProjectReader;
import net.sf.mpxj.mpp.MPPReader;

import plan.PlanWriter;

public class Convert {

    public static void main(String[] args) {
        System.out.println("Convert");
        try
        {
            System.out.println("Convert args: " + args.length);
            Convert convert = new Convert();
            if (args.length < 1 || args.length > 6)
            {
                convert.usage();
            }
            else
            {

                List<String> options = convert.extractOptions(args);
                System.out.println("Convert args: options = " + options);
                if (!options.isEmpty() && (options.size() != 2 || args.length < (options.size() * 2 + 1)))
                {
                    convert.usage();
                }
                else
                {
                    Integer idxIn = options.size() * 2;
                    String type = new String();
                    String password = new String();
                    if (!options.isEmpty()) {
                        type = options.get(0);
                        password = options.get(1);
                    }
                    System.out.println("Reading input file started: " + args[idxIn]);
                    long start = System.currentTimeMillis();
                    ProjectFile projectFile = convert.readFile(args[idxIn], type, password);
                    long elapsed = System.currentTimeMillis() - start;
                    System.out.println("Reading input file completed in " + elapsed + "ms.");

                    if (args.length == idxIn + 2)
                    {
                        System.out.println("Writing Plan output file started: " +  args[idxIn + 1]);
                        PlanWriter w = new PlanWriter();
                        w.write(projectFile, args[idxIn + 1]);
                        System.out.println("Writing Plan output file completed.");
                    }
                }
            }
        }
        catch (Throwable ex)
        {
            System.out.println("Convert Exception");
            ex.printStackTrace(System.out);
        }
    }

    public void usage()
    {
        // Note: No option or both options must be present.
        // options: --type <file type> --password <password>
        // arg 0: <input file>
        // arg 1: [<plan file name>]
        System.out.println("Usage: Convert [--type <file type> --password <password>] <input file name> [<plan file name>]");
    }

    public List<String> extractOptions(String[] args)
    {
        List<String> options = new ArrayList<String>();
        if (args.length < 4) {
            return options;
        }
        if (!args[0].startsWith("--type") || !args[2].startsWith("--password")) {
            System.out.println("extractOptions: failed: args[0] = " + args[0] + ", args[2] = " + args[2]);
            return options;
        }
        options.add(args[1]);
        options.add(args[3]);
        return options;
    }

    /**
    * Use the universal project reader to open the file.
    * Throw an exception if we can't determine the file type.
    *
    * @param inputFile file name
    * @return ProjectFile instance
    */
    static public ProjectFile readFile(String inputFile, String type, String password) throws MPXJException
    {
        ProjectFile projectFile;
        if (type.contentEquals("mpp"))
        {
            MPPReader mppreader = new MPPReader();
            mppreader.setReadPassword(password);
            projectFile = mppreader.read(inputFile);
        }
        else
        {
            System.out.println("readFile: read any project type");
            ProjectReader reader = new UniversalProjectReader();
            projectFile = reader.read(inputFile);
        }
        if (projectFile == null)
        {
            throw new IllegalArgumentException("Unsupported file type");
        }
        return projectFile;
    }

}
