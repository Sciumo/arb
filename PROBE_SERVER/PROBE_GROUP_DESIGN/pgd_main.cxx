#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <set>

#include <arbdb.h>
#include <arbdbt.h>

#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#include "../global_defs.h"
#include "../common.h"
#include "../mapping.h"

#define MINTEMPERATURE 30.0
#define MAXTEMPERATURE 100.0
#define MINGC          50.0
#define MAXGC          100.0
#define MAXBOND        4.0
#define MINPOSITION    0
#define MAXPOSITION    10000
#define MISHIT         0
#define MINTARGETS     100
#define DTEDGE         0.5
#define DT             0.5
#define SPLIT          0.5
#define CLIPRESULT     40       //??? length of output

typedef string SpeciesName;
typedef string Probes;

static GBDATA *gb_main = 0;            //ARB-database
static GBDATA *pd_main = 0;            //probe-design-database

typedef enum { TF_NONE, TF_CREATE, TF_EXTEND } Treefile_Action;

struct InputParameter {
    string          db_name;
    string          pt_server_name;
    string          pd_db_name;
    int             pb_length;
    Treefile_Action gen_treefile;
    string          treefile;
    string          versionumber;
};

static InputParameter para;

static float bondval[16]={0.0,0.0,0.5,1.1,
                   0.0,0.0,1.5,0.0,
                   0.5,1.5,0.4,0.9,
                   1.1,0.0,0.9,0.0};

struct gl_struct {
    aisc_com  *link;
    T_PT_LOCS  locs;
    T_PT_MAIN  com;
};


static bool  server_initialized  = false;
static char *current_server_name = 0;

static GB_HASH *path2subtree        = 0;
static long     no_of_subtrees      = 0;
static int      max_subtree_pathlen = 0;
static char    *pathbuffer          = 0;

static GB_ERROR init_path2subtree_hash(GBDATA *pd_father, long expected_no_of_subtrees) {
    GB_ERROR error = 0;
    if (!path2subtree) {
        path2subtree   = GBS_create_hash(expected_no_of_subtrees, 1);
        no_of_subtrees = 0;

        for (GBDATA *pd_subtree = GB_find(pd_father,"subtree",0,down_level);
             pd_subtree && !error;
             pd_subtree = GB_find(pd_subtree,"subtree",0,this_level|search_next))
        {
            GBDATA *pd_path = GB_find(pd_subtree, "path", 0, down_level);
            if (!pd_path) {
                error = "Missing 'path' entry";
            }
            else {
                GBS_write_hash(path2subtree, GB_read_char_pntr(pd_path), (long)pd_subtree);
                no_of_subtrees++;

                // determine max. pathlen
                int pathlen = GB_read_count(pd_path);
                if (pathlen>max_subtree_pathlen) {
                    max_subtree_pathlen = pathlen;
                }
            }
        }

        if (!error) {
            if (no_of_subtrees == 0) {
                error = "No subtrees found";
            }
            else if (no_of_subtrees != expected_no_of_subtrees) {
                error = GBS_global_string("Wrong number of subtrees (found=%li, expected=%li)", no_of_subtrees, expected_no_of_subtrees);
            }
        }

        if (!error) {
            pathbuffer = (char*)GB_calloc(max_subtree_pathlen+1, sizeof(char));
        }
    }

    return error;
}

static char *probe_pt_look_for_server(GBDATA *gb_main,const char *servername,GB_ERROR& error){
    int serverid = -1;

    for (int i=0;i<1000;++i){
        char *aServer=GBS_ptserver_id_to_choice(i);
        if (aServer){
            if(strcmp(aServer,servername)==0){
                serverid=i;
                printf("Found pt-server: %s\n",aServer);
                free(aServer);
                break;
            }
            free(aServer);
        }
    }
    if (serverid==-1){
        error = GB_export_error("'%s' is not a known pt-server.",servername);
        return 0;
    }

    char choice[256];
    sprintf(choice,"ARB_PT_SERVER%li",(long)serverid);
    GB_ERROR starterr = arb_look_and_start_server(AISC_MAGIC_NUMBER, choice, gb_main);
    if (starterr) {
        error = GB_export_error("Cannot start pt-server '%s' (Reason: %s)", servername, starterr);
        return 0;
    }
    return GBS_read_arb_tcp(choice);
}

static GB_ERROR PG_init_pt_server(GBDATA *gb_main, const char *servername) {
    GB_ERROR error = 0;
    if (server_initialized) {
        error = "pt-server initialized twice";
    }
    else {
        printf("Search a free running pt-server..\n");
        current_server_name            = (char *)probe_pt_look_for_server(gb_main,servername,error);
        if (!error) server_initialized = true;
    }
    return error;
}

static void PG_exit_pt_server(void) {
    free(current_server_name);
    current_server_name = 0;
    server_initialized  = false;
}

static int init_local_com_struct(struct gl_struct& pd_gl)
{
    const char *user;
    if (!(user = getenv("USER"))) user = "unknown user";

    /* @@@ use finger, date and whoami */
    if( aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &pd_gl.locs,
                    LOCS_USER, user,
                    NULL)){
        return 1;
    }
    return 0;
}

class PT_server_connection {
    //private:
    //    T_PT_PDC  pdc;
    //    GB_ERROR error;
    //    struct gl_struct my_pd_gl;

public:
    T_PT_PDC  pdc;
    GB_ERROR error;
    struct gl_struct my_pd_gl;
    PT_server_connection(){
        error = 0;
        memset(&my_pd_gl, 0, sizeof(my_pd_gl));

    my_pd_gl.link=(aisc_com *)aisc_open(current_server_name,&my_pd_gl.com,AISC_MAGIC_NUMBER);

    if(!my_pd_gl.link) error = "can't contact pt_server (unknown reason\n";
    else if(init_local_com_struct(my_pd_gl)) error = "can't contact pt_server (connection refused)\n";
    else{
        aisc_create(my_pd_gl.link,PT_LOCS, my_pd_gl.locs,
                LOCS_PROBE_DESIGN_CONFIG, PT_PDC, &pdc,
                0);
    }
    }
    virtual ~PT_server_connection() {
        if (my_pd_gl.link) aisc_close(my_pd_gl.link);
        delete error;
    }

    struct gl_struct& get_pd_gl() {return my_pd_gl;}
    GB_ERROR get_error() const {return error;}
    T_PT_PDC& get_pdc() {return pdc;}
};

static PT_server_connection *my_server;

//  -----------------------------
//      void helpArguments()
//  -----------------------------
static void helpArguments(){
    fprintf(stderr,
            "\n"
            "Usage: arb_probe_group_design <db_name> <pt_server> <db_out> <probe_length> [(-c|-x) <treefile> <versionumber>]\n"
            "\n"
            "db_name        name of ARB-database to build groups for\n"
            "pt_server      name of pt_server\n"
            "db_out         name of probe-design-database\n"
            "probe_lengh    length of the probe (15-20)\n"
            "treefile       treefile for probe_server [-c=create, -x=extend]\n"
            "versionumber   should be current date in seconds till epoch\n"
            "\n"
            );
}

//  --------------------------------------------------------
//      GB_ERROR scanArguments(int argc,char *argv[])
//  --------------------------------------------------------
static GB_ERROR scanArguments(int argc,char *argv[]){
    GB_ERROR error = 0;

    if (argc == 5 || argc == 8) {
        para.db_name        = argv[1];
        para.pt_server_name = string("localhost: ")+argv[2];
        para.pd_db_name     = string(argv[3])+argv[4];
        para.pb_length      = atoi(argv[4]);
        if (argc == 8) {
            if      (strcmp(argv[5], "-c") == 0) para.gen_treefile = TF_CREATE;
            else if (strcmp(argv[5], "-x") == 0) para.gen_treefile = TF_EXTEND;
            else error = GBS_global_string("'-c' or '-x' expected, '%s' found", argv[5]);

            para.treefile     = string(argv[6]);
            para.versionumber = string(argv[7]);
        }
        else {
            para.gen_treefile = TF_NONE;
        }
    }
    else {
        helpArguments();
        error = "Wrong number of arguments";
    }

    return error;
}

static GB_ERROR pgd_add_species(int len, set<SpeciesName> *species) {
    // uses the global buffer 'pathbuffer'

    GBDATA *pd_subtree = (GBDATA*)GBS_read_hash(path2subtree, pathbuffer);

    if (pd_subtree) {
        pathbuffer[len]   = 'L';
        pathbuffer[len+1] = 0;

        if (pgd_add_species(len+1, species)) {
            // error occurred -> we are a leaf
            pathbuffer[len] = 0;

            GBDATA *gb_member = GB_find(pd_subtree, "member", 0, down_level);
            if (!gb_member) {
                return GBS_global_string("'member' expected in '%s'", pathbuffer);
            }

            const string& name = PM_ID2name(GB_read_int(gb_member));
            species->insert(name.c_str());
            return 0; // ok
        }

        pathbuffer[len]   = 'R';
        pathbuffer[len+1] = 0;

        return pgd_add_species(len+1, species);
    }

    return GBS_global_string("illegal path '%s'", pathbuffer);
}

static GB_ERROR pgd_init_species(GBDATA *pd_probe_group, set<SpeciesName> *species) {
    GBDATA *pd_subtreepath = GB_find(pd_probe_group, "subtreepath", 0, down_level);

    if (pd_subtreepath) {       // the probe group belongs to a subtree
        const char *path = GB_read_char_pntr(pd_subtreepath);
        int         len  = GB_read_count(pd_subtreepath);

        memcpy(pathbuffer, path, len+1);
        return pgd_add_species(len, species);
    }

    // probe group does not belong to subtree

    return "subtree-independant probe groups not implemented yet!";
}

static char *pgd_get_the_names(set<SpeciesName> *species,bytestring &bs,bytestring &checksum){
    GBDATA *gb_species;
    GBDATA *gb_name;
    GBDATA *gb_data;

    void *names = GBS_stropen(1024);
    void *checksums = GBS_stropen(1024);

    GB_begin_transaction(gb_main);
    char *use=GBT_get_default_alignment(gb_main);

    gb_species=GB_search(gb_main,"species_data",GB_FIND);
    if(!gb_species) return 0;

    for(set<SpeciesName>::const_iterator i=(*species).begin();i!=(*species).end();++i){
        gb_name=GB_find(gb_species,"name",(*i).c_str(),down_2_level);
        if(gb_name){
            gb_data=GB_find(gb_name,use,0,this_level);
            if(gb_data) gb_data=GB_find(gb_data,"data",0,down_level);
            if(!gb_data) return (char*)GB_export_error("Species '%s' has no sequence '%s'",GB_read_char_pntr(gb_name),use);
            GBS_intcat(checksums, GBS_checksum(GB_read_char_pntr(gb_data), 1, ".-"));
            GBS_strcat(names, GB_read_char_pntr(gb_name));
            GBS_chrcat(checksums, '#');
            GBS_chrcat(names, '#');
        }
    }

    bs.data = GBS_strclose(names, 0);
    bs.size = strlen(bs.data)+1;

    checksum.data = GBS_strclose(checksums, 0);
    checksum.size = strlen(checksum.data)+1;

    GB_commit_transaction(gb_main);
    return 0;
}

static int pgd_probe_design_send_data(){
    if (aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_DTEDGE,DTEDGE*100.0,
                 PDC_DT,DT*100.0,
                 PDC_SPLIT,SPLIT,
                 PDC_CLIPRESULT,CLIPRESULT,
                 0)) return 1;
    for (int i=0;i<16;i++) {
        if (aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                     PT_INDEX,i,
                     PDC_BONDVAL,bondval[i],
                     0)) return 1;
    }
    return 0;
}

static GB_ERROR probe_design_event(set<SpeciesName> *species,set<Probes> *probe_list)
{
    T_PT_TPROBE  tprobe;        //long
    bytestring   bs;
    bytestring   check;
    char        *match_info;
    const char  *error = 0;

    // validate sequence of the species
    error = pgd_get_the_names(species,bs,check);

    if (!error) {
        aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_NAMES,&bs,
                 PDC_CHECKSUMS,&check,
                 0);

        // validate PT server (Get unknown names)
        bytestring unknown_names;
        if (aisc_get(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                     PDC_UNKNOWN_NAMES,&unknown_names, 0))
        {
            error = "Connection to PT_SERVER lost (2)";
        }

        if (!error) {
            char *unames = unknown_names.data;
            if (unknown_names.size>1) {
                error = GB_export_error("Your PT server is not up to date or wrongly chosen\n"
                                        "The following names are new to it:\n"
                                        "%s" , unames);
                delete unknown_names.data;
            }
        }
    }

    if (!error) {
        // calling the design
        aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_GO,0, 0);

        //result
        // printf("Read the results from the server");
        {
            char *locs_error = 0;
            if (aisc_get(my_server->get_pd_gl().link,PT_LOCS,my_server->get_pd_gl().locs,
                         LOCS_ERROR,&locs_error, 0))
            {
                error = "Connection to PT_SERVER lost (3)";
            }
            else {
                if (locs_error && locs_error[0]) {
                    error = GB_export_error(locs_error);
                }
                free(locs_error);
            }
        }
    }

    if (!error) {
        aisc_get(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_TPROBE,&tprobe, 0);
    }

    if (!error) {
        while (tprobe) {
            long tprobe_next;
            if(aisc_get(my_server->get_pd_gl().link,PT_TPROBE,tprobe,
                        TPROBE_NEXT,&tprobe_next,
                        TPROBE_INFO,&match_info, 0))
                break;

            tprobe = tprobe_next;

            char *probe,*space;
            probe = strpbrk(match_info,"acgtuACGTU");

            if (probe) {
                space = strchr(probe,' ');
            }

            if (probe && space) {
                *space = 0;
                probe  = strdup(probe);
                *space = ' ';
            }
            else{
                probe = strdup("");
            }
            (*probe_list).insert(probe);
            free(probe);
            free(match_info);
        }
    }

    return error;
}

static GB_ERROR PGD_export_tree(GBT_TREE *node, FILE *out) {
    if (node->is_leaf) {
        if (!node->name) return "leaf w/o name";
        fprintf(out, "'%s'", node->name);
        return 0;
    }

    fprintf(out, "(");
    GB_ERROR error = PGD_export_tree(node->leftson, out);
    fprintf(out, ":%.5f,\n", node->leftlen);
    if (!error) {
        error = PGD_export_tree(node->rightson, out);
        fprintf(out, ":%.5f)", node->rightlen);
        if (node->name) { // groupname
            fprintf(out, "'%s'", node->name);
        }
    }
    return error;
}

static GB_ERROR PGD_decodeBranchNames(GBT_TREE *node) {
    GB_ERROR error = 0;
    if (node->is_leaf) {
        if (!node->name) return "node w/o name";
        char *newName = decodeTreeNode(node->name, error);
        if (error) return error;

        char *afterNum = strchr(newName, ',');
        if (!afterNum) {
            error = GBS_global_string("komma expected in node name '%s'", newName);
        }
        else {
            const string& shortname = PM_ID2name(atoi(newName));
            if (shortname.empty()) {
                error = GBS_global_string("illegal id in '%s'", newName);
            }
            else {
                char *fullname = afterNum+1;
                char *acc      = strchr(fullname, ',');
                if (!acc) {
                    error = GBS_global_string("2nd komma expected in '%s'", newName);
                }
                else {
                    *acc++ = 0;

                    char *newName2 = GBS_global_string_copy("n=%s,f=%s,a=%s", shortname.c_str(), fullname, acc);

                    free(newName);
                    newName = newName2;
                }
            }
        }

        if (error) {
            free(newName);
            return error;
        }

        free(node->name);
        node->name = newName;
        return 0;
    }

    // not leaf:
    if (node->remark_branch) { // if remark_branch then it is a group name
        char *groupName = decodeTreeNode(node->remark_branch, error);
        if (error) return error;

        char *newName = GBS_global_string_copy("g=%s", groupName);
        free(groupName);

        free(node->remark_branch);
        node->remark_branch = 0;

        free(node->name);
        node->name = newName;
    }

    error             = PGD_decodeBranchNames(node->leftson);
    if (!error) error = PGD_decodeBranchNames(node->rightson);
    return error;
}
static GB_ERROR PGD_encodeBranchNames(GBT_TREE *node) {
    if (node->is_leaf) {
        if (!node->name) return "node w/o name";
        char *newName = encodeTreeNode(node->name);
        free(node->name);
        node->name    = newName;
        return 0;
    }

    GB_ERROR error    = PGD_encodeBranchNames(node->leftson);
    if (!error) error = PGD_encodeBranchNames(node->rightson);
    return error;
}

static GBT_TREE *PGD_find_subtree_by_path(GBT_TREE *node, const char *path) {
    const char *restPath = path;

    while (node && !node->is_leaf) {
        char go = restPath[0];

        if (go == 'L') {
            node = node->leftson;
            restPath++;
        }
        else if (go == 'R') {
            node = node->rightson;
            restPath++;
        }
        else if (go) {
            GB_export_error("Illegal character '%c' in path (%s)", go, path);
            node = 0;
        }
        else { // end of path reached
             break;
        }
    }


    // leaf reached
    if (node && restPath[0]) {
        GB_export_error("Leaf reached. Cannot follow rest (%s) of path (%s)", restPath, path);
        node = 0;
    }

    return node;
}

int main(int argc,char *argv[]) {
    printf("arb_probe_group_design v1.0 -- (C) 2001-2003 by Tina Lai & Ralf Westram\n");

    //Check and init Parameters:
    GB_ERROR error = scanArguments(argc, argv);

    //Open arb database:
    if (!error) {
        const char *db_name = para.db_name.c_str();
        printf("Opening ARB-database '%s'...",db_name);

        // @@@ FIXME:  I think this programm should be able to run w/o the main database
        // it only uses the db to open the pt-server connection and does
        // some unneeded species checking..

        gb_main = GBT_open(db_name,"rwt",0);
        if (!gb_main) {
            error            = GB_get_error();
            if(!error) error = GB_export_error("Can't open database '%s'",db_name);
        }
    }

    //Open probe design database:
    if(!error){
        string      pd_nameS = para.pd_db_name+".arb";
        const char *pd_name  = pd_nameS.c_str();
        printf("Opening probe-group-design-database '%s'...\n" ,pd_name);

        pd_main = GB_open(pd_name,"rwcN");
        if (!pd_main) {
            error            = GB_get_error();
            if(!error) error = GB_export_error("Can't open database '%s'",pd_name);
        }
        else {
            error = checkDatabaseType(pd_main, pd_name, "probe_group_subtree_db", "complete");
        }
    }

    if (!error) {
        error = PM_initSpeciesMaps(pd_main);
    }

    if (!error) {
        GB_begin_transaction(pd_main);
        GBDATA *pd_subtree_cont    = GB_find(pd_main,"subtrees",0,down_level);
        GBDATA *pd_probegroup_cont = GB_find(pd_main,"probe_groups",0,down_level);
        if (!pd_subtree_cont) {
            error = "Can't find container 'subtrees'";
        }
        else if (!pd_probegroup_cont) {
            error = "Can't find container 'probe_groups'";
        }
        else {
            if (!GB_find(pd_subtree_cont,"subtree",0,down_level)) {
                error = "Can't find container 'subtree'";
            }
            else {
                {
                    GBDATA *pd_subtree_counter = GB_find(pd_main, "subtree_counter", 0, down_level);
                    if (!pd_subtree_counter) {
                        error = "Can't find 'subtree_counter'";
                    }
                    else {
                        long subtrees = GB_read_int(pd_subtree_counter);
                        error = init_path2subtree_hash(pd_subtree_cont, subtrees); // initialize path cache
                    }
                }

                bool pt_server_initialized = false;
                if (!error) {
                    // initialize pt-server connection
                    PG_init_pt_server(gb_main,para.pt_server_name.c_str());
                    my_server = new PT_server_connection();
                    error     = my_server->get_error();

                    if (!error) pt_server_initialized = true;
                }

                bool aisc_created = false;
                if (!error) {
                    //initialize parameters
                    aisc_create(my_server->get_pd_gl().link,PT_LOCS,my_server->get_pd_gl().locs,
                                LOCS_PROBE_DESIGN_CONFIG,PT_PDC,&my_server->pdc,
                                PDC_PROBELENGTH,para.pb_length,
                                PDC_MINTEMP,MINTEMPERATURE,
                                PDC_MAXTEMP,MAXTEMPERATURE,
                                PDC_MINGC,MINGC/100.0,
                                PDC_MAXGC,MAXGC/100.0,
                                PDC_MAXBOND,MAXBOND,
                                0);
                    aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                             PDC_MINPOS,MINPOSITION,
                             PDC_MAXPOS,MAXPOSITION,
                             PDC_MISHIT,MISHIT,
                             PDC_MINTARGETS,MINTARGETS/100.0,
                             0);

                    if (pgd_probe_design_send_data()) {
                        error = "Connection to PT_SERVER lost (1)";
                    }
                    else {
                        aisc_created = true;
                    }
                }

                if (!error) {
                    long probe_group_counter;
                    {
                        GBDATA *pd_probe_group_counter = GB_find(pd_main, "probe_group_counter", 0, down_level);
                        if (!pd_probe_group_counter) {
                            error = "Can't find 'probe_group_counter'";
                        }
                        else {
                            probe_group_counter = GB_read_int(pd_probe_group_counter);
                        }
                    }

                    if (!error) {
                        printf("Designing probes for %li used probe groups:\n", probe_group_counter);
                    }

                    // design probes
                    long     count         = 0;

                    for (GBDATA *pd_probe_group = GB_find(pd_probegroup_cont,"probe_group",0,down_level);
                         pd_probe_group && !error;
                         pd_probe_group = GB_find(pd_probe_group,"probe_group",0,this_level|search_next))
                    {
                        ++count;
                        if (count%60) fputc('.', stdout);
                        else printf(". %i%%\n", int((double(count)/probe_group_counter)*100+0.5));

                        {
                            // erase existing data
                            GBDATA *pbgrp = GB_find(pd_probe_group,"probe_group_design",0,down_level);
                            if (pbgrp) GB_delete(pbgrp);
                        }

                        // get species
                        std::set<SpeciesName> species; //set of species for probe design
                        std::set<Probes>      probe; //set of probes from the design

                        error = pgd_init_species(pd_probe_group, &species);

                        if (!error && species.empty()) error = "No species detected for probe group.";
                        if (!error) error = probe_design_event(&species, &probe); // call probe_design

                        if (!error && !probe.empty()) {
                            // store the result

                            GBDATA *pd_design_only = 0;
                            GBDATA *pd_both        = 0;
                            GBDATA *pd_match_only  = GB_find(pd_probe_group, "probe_matches", 0, down_level);

                            if (!pd_match_only) error = "probe group w/o 'probe_matches' container";

                            for (set<Probes>::const_iterator i=probe.begin(); i != probe.end() && !error; ++i) {
                                const char *probe_string = i->c_str();
                                GBDATA     *pd_did_match = GB_find(pd_match_only, 0, probe_string, down_level);

                                if (pd_did_match) { // probe already matched in container 'probe_matches'
                                    // remove there and put into 'probe_group_common'
                                    if (!pd_both) {
                                        pd_both             = GB_search(pd_probe_group, "probe_group_common", GB_CREATE_CONTAINER);
                                        if (!pd_both) error = GB_get_error();
                                    }
                                    if (!error) {
                                        GBDATA *pd_probe = GB_create(pd_both, "probe", GB_STRING);
                                        error            = GB_write_string(pd_probe, probe_string);

                                        if (!error) error = GB_delete(pd_did_match);
                                    }
                                }
                                else { // probe was not found yet -> put into 'probe_group_design'
                                    if (!pd_design_only) {
                                        pd_design_only             = GB_search(pd_probe_group, "probe_group_design", GB_CREATE_CONTAINER);
                                        if (!pd_design_only) error = GB_get_error();
                                    }
                                    if (!error) {
                                        GBDATA *pd_probe = GB_create(pd_design_only, "probe", GB_STRING);
                                        error            = GB_write_string(pd_probe, probe_string);
                                    }
                                }
                            }

                            if (!error) {
                                // remove empty 'probe_matches' container
                                GBDATA *pd_has_match = GB_find(pd_match_only, 0, 0, down_level);
                                if (!pd_has_match) error = GB_delete(pd_match_only);
                            }
                        }
                    }

                    if (error) fputc('\n', stdout);
                    else printf(" %i%%\n", int((double(count)/probe_group_counter)*100+0.5));

                    // probes for all groups have been designed and checked against matched probes.
                    // now update tree

                    if (!error) {
                        if (para.gen_treefile != TF_NONE) { // create/extend tree ?
                            // load the tree
                            GBT_TREE *tree    = 0;
                            GBDATA   *pd_tree = GB_find(pd_main, "tree", 0, down_level);
                            if (!pd_tree) {
                                error = "'tree' missing";
                            }
                            else {
                                tree = GBT_read_plain_tree(pd_main, pd_tree, sizeof(*tree));
                                PGD_decodeBranchNames(tree);
                            }

                            for (GBDATA *pd_subtree = GB_find(pd_subtree_cont,"subtree",0,down_level);
                                 pd_subtree && !error;
                                 pd_subtree = GB_find(pd_subtree,"subtree",0,this_level|search_next))
                            {
                                GBDATA *pd_exact = GB_find(pd_subtree, "exact", 0, down_level);
                                if (pd_exact) { // has exact probes
                                    GBDATA *pd_path = GB_find(pd_subtree, "path", 0, down_level);
                                    if (!pd_path) {
                                        error = "no 'path' in 'subtree'";
                                    }
                                    else {
                                        const char *path = GB_read_char_pntr(pd_path);
                                        GBT_TREE   *node = PGD_find_subtree_by_path(tree, path);
                                        if (!node) {
                                            error = GBS_global_string("cannot find node '%s'", path);
                                        }
                                        else {
                                            long exact_matches = GB_read_int(pd_exact);

                                            if (node->name) {
                                                char *newName      = GBS_global_string_copy("%s,em=%li", node->name, exact_matches);
                                                free(node->name);
                                                node->name = newName;
                                            }
                                            else {
                                                node->name = GBS_global_string_copy("em=%li", exact_matches);
                                            }
                                        }
                                    }
                                }
                            }

                            if (!error) {
                                // save the tree
                                PGD_encodeBranchNames(tree);

                                const char *treefilename = para.treefile.c_str();
                                FILE       *out          = fopen(treefilename, "wt");

                                fprintf(out, "[version=ARB_PS_TREE_%s\n]\n", para.versionumber.c_str());
                                error = PGD_export_tree(tree, out);
                                fputc('\n', out);
                                fclose(out);

                                if (error) unlink(treefilename);
                            }
                        }
                    }


                    /*
                    if (error) fputc('\n', stdout);
                    else {
                        printf(" %i%%\n", int((double(count)/subtrees)*100+0.5));
                        printf("\nMarking common probes..\n");

                        // check whether probes common in probe_group and probe_group_design
                        count           = 0;
                        for (GBDATA *pd_probe_group = GB_find(pd_probegroup_cont,"probe_group",0,down_level);
                             pd_probe_group && !error;
                             pd_probe_group = GB_find(pd_probe_group,"probe_group",0,this_level|search_next))
                        {
                            ++count;
                            if (count%60) fputc('.', stdout);
                            else printf(". %i%%\n", int((double(count)/probe_group_counter)*100+0.5));

                            GBDATA *pd_probe_matches      = GB_find(pd_probe_group, "probe_matches", 0, down_level);
                            GBDATA *pd_probe_group_design = GB_find(pd_probe_group, "probe_group_design", 0, down_level);
                            GBDATA *pd_probe_group_common = 0;

                            if (pd_probe_matches && pd_probe_group_design) {
                                int probes_found = 0;

                                for (GBDATA *pd_probe1 = GB_find(pd_probe_matches, "probe", 0, down_level);
                                     pd_probe1; )
                                {
                                    GBDATA     *pd_next_probe1 = GB_find(pd_probe1, "probe", 0, this_level|search_next);
                                    const char *probe_string   = GB_read_char_pntr(pd_probe1);
                                    GBDATA     *pd_probe2      = GB_find(pd_probe_group_design, 0, probe_string, down_level);

                                    if (pd_probe2) { // 'probe_group_design' also contains the probe
                                        // -> put the common probe into 'probe_group_common'
                                        if (!pd_probe_group_common) {
                                            pd_probe_group_common = GB_search(pd_probe_group, "probe_group_common", GB_CREATE_CONTAINER);
                                            if (!pd_probe_group_common) error = GB_get_error();
                                        }

                                        if (!error) {
                                            GBDATA *pd_common1     = GB_create(pd_probe_group_common, "probe", GB_STRING);
                                            if (!pd_common1) error = GB_get_error();
                                            else    error          = GB_write_string(pd_common1, probe_string);

                                            probes_found++;

                                            // and delete probe from 'probe_group' and 'probe_group_design'
                                            if (!error) error = GB_delete(pd_probe2);
                                            if (!error) error = GB_delete(pd_probe1);
                                        }
                                    }

                                    pd_probe1 = pd_next_probe1;
                                }

                                // add number of found probes to nodename
                                if (probes_found && !error) {
                                    GBDATA     *pd_path      = GB_find(pd_probe_group, "path", 0, down_level);
                                    const char *path         = GB_read_char_pntr(pd_path);
                                    GBT_TREE   *current_node = PGD_find_subtree_by_path(tree, path);

                                    if (!current_node) {
                                        error = GBS_global_string("Can't find current node (%s)", path);
                                    }
                                    else {
                                        if (current_node->name) {
                                            char *newName      = GBS_global_string_copy("%s,em=%i", current_node->name, probes_found);
                                            free(current_node->name);
                                            current_node->name = newName;
                                        }
                                        else {
                                            current_node->name = GBS_global_string_copy("em=%i", probes_found);
                                        }
                                    }
                                }
                            }
                            else {
#if defined(DEBUG)
                                GBDATA     *pd_path = GB_find(pd_probe_group, "path", 0, down_level);
                                const char *path    = GB_read_char_pntr(pd_path);
                                const char *error_message;

                                if (pd_probe_matches) {
                                    error_message = "Container 'probe_group_design' not found";
                                }
                                else {
                                    if (pd_probe_group_design) error_message = "Container 'probe_matches' not found";
                                    else error_message                       = "Containers 'probe_matches' and 'probe_group_design' not found";
                                }

                                fprintf(stderr, "\n%s (in subtree '%s')\n", error_message, path);
#endif // DEBUG
                            }
                        }

                        if (error) fputc('\n', stdout);
                        else printf(" %i%%\n", int((double(count)/subtrees)*100+0.5));

                    }
                    */
                }

                // clean up pt-server connection
                if (aisc_created) {
                    aisc_close(my_server->get_pd_gl().link);
                    my_server->get_pd_gl().link = 0;
                }
                if (pt_server_initialized) PG_exit_pt_server();
            }
        }

        // adjust database type
        if (!error) error = setDatabaseState(pd_main, "probe_group_design_db", "complete");

        if (error) {
            GB_abort_transaction(pd_main);
        }
        else {
            GB_commit_transaction(pd_main);
            string      savenameS = para.pd_db_name+"_design.arb";
            const char *savename  = savenameS.c_str();
            printf("Saving %s ..\n", savename);
            error                 = GB_save(pd_main, savename, SAVE_MODE);
        }
    }

    if (error) {
        fprintf(stderr, "Error in probe_group_design: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

