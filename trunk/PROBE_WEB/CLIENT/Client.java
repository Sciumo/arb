import java.util.*;
import java.awt.*;
import java.awt.event.*;


/**
 * central driver and control class for ARB probe service client
 */

class Client
{

private ProbesGUI    display;
private TreeDisplay   tree;
private TreeNode      root;
private String        treeString;
private String        baseurl;
private HttpSubsystem webAccess;

private HashMap knownOptions()
     {
         HashMap hm = new HashMap();
         hm.put("server", "=URL        Specify server URL manually (note: needs leading /)");
         hm.put("tree",   "=reload       Force tree reload");
         return hm;
     }

    // private TreeNode subtree;

    // private Communicationsubsystem


public static String readTree(HttpSubsystem webAccess, boolean forceReload)
    {
        String     localTreeVersion  = null;
        String     serverTreeVersion = webAccess.getNeededTreeVersion();
        String     error             = null;
        String     localTreeFile     = new String("probetree.gz");
        TreeReader tr                = null;

        if (forceReload) {
            localTreeVersion = "reload forced";
        }
        else {
            tr    = new TreeReader(localTreeFile);
            error = tr.getError();

            if (error != null) {
                System.out.println("Can't access the tree ("+error+")");
                localTreeVersion = "missing or corrupt";
            }
            else {
                localTreeVersion = tr.getVersionString();
            }
        }

        if (!serverTreeVersion.equals(localTreeVersion))
        {
            System.out.println("You have tree version '" + localTreeVersion+"'");
            System.out.println("Downloading updated tree '"+serverTreeVersion+"' ..");

            error = webAccess.downloadZippedTree(localTreeFile);
            if (error == null) {
                tr    = new TreeReader(localTreeFile);
                error = tr.getError();
            }
            if (error != null) {
                Toolkit.AbortWithError("Server problem: Can't get a valid tree ("+error+")");
            }
            else {
                if (!tr.getVersionString().equals(serverTreeVersion)) {
                    System.out.println("Warning: downloaded tree has unexpected version '"+tr.getVersionString()+"'");
                }
            }
        }

        return tr.getTreeString();
    }

public static void main(String[] args)

    {
        // create central client object(s)
        // create communication structure
        // initialization of communication
        // control of user interaction

        // System.out.println(args.length);

        Client cl         = new Client();
        Toolkit.clientName = "arb_probe_library";

        System.out.println(Toolkit.clientName+" v"+Toolkit.clientVersion+" -- (C) 2003 Lothar Richter & Ralf Westram");
        CommandLine cmdline = new CommandLine(args, cl.knownOptions());

        if (cmdline.getOption("server")) {
            cl.baseurl = cmdline.getOptionValue("server");
        }
        else {
            // cl.baseurl = new String("http://probeserver.mikro.biologie.tu-muenchen.de/"); // final server URL (not working yet)
            cl.baseurl = new String("http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/"); // URL for debugging
        }

        cl.webAccess = new HttpSubsystem(cl.baseurl);

        // ask server for version info
        cl.webAccess.retrieveVersionInformation(); // terminates on failure
        if (!Toolkit.clientVersion.equals(cl.webAccess.getNeededClientVersion())) {
            Toolkit.AbortWithError("Your client is out-of-date! Please download the new version from http://arb-home.de/");
        }

        // load and parse the most recent tree
        {
            boolean reload_tree = cmdline.getOption("tree") && cmdline.getOptionValue("tree").equals("reload");
            cl.treeString       = readTree(cl.webAccess, reload_tree); // terminates on failure
            cl.root             = (new TreeParser(cl.treeString)).getRootNode();
        }

        if (cl.root == null)
            {
                System.out.println("in Client(): no valid node given to display");
                System.exit(1);
            }

        cl.root.setPath("");
        cl.display = new ProbesGUI(cl.root, 10, Toolkit.clientName+" Version "+Toolkit.clientVersion);
        //        ProbesGUIActionListener al = new ProbesGUIActionListener(cl.display);
        //        cl.display.setMenuBar(new ProbeMenu(al));
        //
        // obtain reference to Treedisplay first !
        cl.display.getTreeDisplay().setBoss(cl);
        cl.display.setLocation(200, 200);
        cl.display.setVisible(true);


    }

public Client()
    {
        //        display = new ProbesGUI();
    }

public ProbesGUI getDisplay()
    {
        return display;
    }

public String getNodeInformation(String nodePath)
    {
        return webAccess.retrieveNodeInformation(nodePath);
    }
}

