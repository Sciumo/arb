#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "AW_rename.hxx"

#include <names_client.h>
#include <servercntrl.h>
#include <client.h>

//  -----------------------------------
//      class NameServerConnection
//  -----------------------------------

class NameServerConnection {
private:
    aisc_com   *link;
    T_AN_LOCAL  locs;
    T_AN_MAIN   com;
    int         persistant;     // if true -> connection will not be closed
    time_t      linktime;       // time, when link has been established

    //  ----------------------------------
    //      int init_local_com_names()
    //  ----------------------------------
    int init_local_com_names()
    {
        if (!link) return 1;    /*** create and init local com structure ***/
        if (aisc_create(link, AN_MAIN, com,
                        MAIN_LOCAL, AN_LOCAL, &locs,
                        LOCAL_WHOAMI, "i bin der arb_tree",
                        NULL)){
            return 1;
        }
        return 0;
    }

    NameServerConnection(const NameServerConnection& other);
    NameServerConnection& operator=(const NameServerConnection& /*other*/);

    GB_ERROR reconnect(GBDATA *gb_main) { // reconnect ignoring consistency
        int old_persistant = persistant;

        printf("Reconnecting name server\n");

        persistant = 0; // otherwise disconnect() won't disconnect
        disconnect();
        persistant = old_persistant; // restore previous persistancy

        return connect(gb_main);
    }

public:

    NameServerConnection() {
        link       = 0;
        locs       = 0;
        com        = 0;
        persistant = 0;
    }
    virtual ~NameServerConnection() {
        gb_assert(persistant == 0); // forgot to remove persistancy ?
        disconnect();
    }

    GB_ERROR connect(GBDATA *gb_main) {
        GB_ERROR err = 0;
        if (!link) {
            const char *name_server = "ARB_NAME_SERVER";

            if (arb_look_and_start_server(AISC_MAGIC_NUMBER,name_server,gb_main)) {
                err = "Sorry I can't start the NAME SERVER";
            }
            else {
                char *servername = (char *)GBS_read_arb_tcp(name_server);
                if (!servername) {
                    GB_CORE;
                    exit (-1);
                }

                link     = (aisc_com *)aisc_open(servername, &com,AISC_MAGIC_NUMBER);
                linktime = time(0);
                
                if (init_local_com_names()) err = "Sorry I can't start the NAME SERVER";
                free(servername);
            }
        }
        else {
            long linkage = int(time(0)-linktime);

#if defined(DEBUG) && 0
            // print information about name-server link age
            static long lastage = -1;
            if (linkage != lastage) {
                printf("Age of NameServerConnection: %li\n", linkage);
                lastage = linkage;
            }
#endif // DEBUG
            
            if (linkage > (5*60)) { // perform a reconnect after 15 minutes
                // Reason : The pipe to the name server breaks after some time
                err = reconnect(gb_main);
            }
        }
        return err;
    }

    void disconnect() {
        if (persistant == 0) {
            if (link) {
                aisc_close(link);
            }
            link = 0;
        }
    }
    
    void persistancy(bool persist) {
        if (persist) {
            ++persistant;
        }
        else {
            --persistant;
            if (persistant <= 0) {
                persistant = 0;
                disconnect();
            }
        }
    }


    aisc_com *getLink() { return link; }
    T_AN_LOCAL getLocs() { return locs; }
};

static NameServerConnection name_server;

PersistantNameServerConnection::PersistantNameServerConnection() {
    name_server.persistancy(true);
}
PersistantNameServerConnection::~PersistantNameServerConnection() {
    name_server.persistancy(false);
}

GB_ERROR AWTC_generate_one_name(GBDATA *gb_main, const char *full_name, const char *acc, char*& new_name, bool openstatus, bool showstatus) {
    // create a unique short name for 'full_name'
    // the result is written into 'new_name' (as malloc-copy)
    // if fails: GB_ERROR!=0 && new_name==0
    // acc may be 0

    new_name = 0;
    if (!acc) acc = "";

    if (openstatus) {
        aw_openstatus(GBS_global_string("Short name for '%s'", full_name));
        showstatus = true;
    }

    if (showstatus) {
        aw_status("Connecting to name server");
        aw_status((double)0);
    }
    
    GB_ERROR err = name_server.connect(gb_main);
    if (err) return err;

    if (showstatus) aw_status("Generating name");
    static char *shrt = 0;
    if (strlen(full_name)) {
        if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                      LOCAL_FULL_NAME,  full_name,
                      LOCAL_ACCESSION,  acc,
                      LOCAL_ADVICE,     "",
                      0)){
            err = "Connection Problems with the NAME_SERVER";
        }
        if (aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                     LOCAL_GET_SHORT,   &shrt,
                     0)){
            err = "Connection Problems with the NAME_SERVER";
        }
    }

    if (err) {
        free(shrt);
    }
    else {
        if (shrt) {
            new_name = shrt;
            shrt = 0;
        }
        else {
            err = GB_export_error("Generation of short name for '%s' failed", full_name);
        }
    }

    if (openstatus) aw_closestatus();
    name_server.disconnect();

    return err;
}

GB_ERROR AWTC_recreate_name(GBDATA *gb_species, bool update_status) {
    GBDATA   *gb_main = GB_get_root(gb_species);

    if (update_status) {
        aw_status("Connecting to name server");
        aw_status((double)0);
    }

    GB_ERROR error = name_server.connect(gb_main);
    if (!error) {
        if (update_status) aw_status("Generating name");

        GBDATA *gb_name      = GB_find(gb_species, "name", 0, down_level);
        GBDATA *gb_full_name = GB_find(gb_species, "full_name", 0, down_level);
        GBDATA *gb_acc       = GB_find(gb_species, "acc", 0, down_level);

        char *name      = gb_name ? GB_read_string(gb_name) : strdup("");
        char *full_name = gb_full_name ? GB_read_string(gb_full_name) : strdup("");
        char *acc       = gb_acc ? GB_read_string(gb_acc) : strdup("");
        int   deleted   = 0;
        char *shrt      = 0;

        if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                      LOCAL_FULL_NAME,  full_name,
                      LOCAL_ACCESSION,  acc,
                      LOCAL_ADVICE,     "",
                      0) != 0 ||
            aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                     LOCAL_DEL_SHORT,   &deleted,
                     0)  != 0 ||
            aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                     LOCAL_GET_SHORT,   &shrt,
                     0)  !=0)
        {
            error = "Connection Problems with the NAME_SERVER";
        }
        name_server.disconnect();

        if (!error) {
            GBT_begin_rename_session(gb_main, 0);
            error = GBT_rename_species(name, shrt);
            if (error) {
                if (GBT_find_species(gb_main, shrt)) { // it was a rename error
                    int done = 0;
                    error    = 0;
                    for (int count = 2; !done && !error && count<10; count++) {
                        const char *other_short = GBS_global_string("%s.%i", shrt, count);
                        if (!GBT_find_species(gb_main, other_short)) {
                            error            = GBT_rename_species(name, other_short);
                            if (!error) done = 1;
                        }
                    }

                    if (!done && !error) {
                        error = "Failed to regenerate name. Please use 'Generate new names'";
                    }
                }
            }

            if (error) GBT_abort_rename_session();
            else {
                if (update_status) GBT_commit_rename_session(aw_status, aw_status);
                else GBT_commit_rename_session(0, 0);
            }
        }

        free(shrt);
        free(acc);
        free(full_name);
        free(name);
    }

    return error;
}

GB_ERROR AWTC_pars_names(GBDATA *gb_main, int update_status)
{
    GB_ERROR err = name_server.connect(gb_main);
    if (!err) {
        err = GBT_begin_rename_session(gb_main,1);
        if (!err) {
            char     *ali_name = GBT_get_default_alignment(gb_main);
            GB_HASH  *hash     = GBS_create_hash(GBT_get_species_hash_size(gb_main), 1);
            GB_ERROR  warning  = 0;
            long      spcount  = 0;
            long      count    = 0;

            if (update_status) {
                aw_status("Renaming");
                spcount = GBT_count_species(gb_main);
            }

            for (GBDATA *gb_species = GBT_first_species(gb_main);
                 gb_species && !err;
                 gb_species = GBT_next_species(gb_species))
            {
                if (update_status) aw_status(count++/(double)spcount);

                GBDATA *gb_full_name = GB_find(gb_species,"full_name",0,down_level);
                GBDATA *gb_name      = GB_find(gb_species,"name",0,down_level);
                GBDATA *gb_acc       = GBT_gen_accession_number(gb_species,ali_name);

                char *acc       = gb_acc ? GB_read_string(gb_acc) : strdup("");
                char *full_name = gb_full_name ? GB_read_string(gb_full_name) : strdup("");
                char *name      = GB_read_string(gb_name);

                char *shrt = 0;

                if (strlen(acc) + strlen(full_name) ) {
                    if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                                  LOCAL_FULL_NAME,  full_name,
                                  LOCAL_ACCESSION,  acc,
                                  LOCAL_ADVICE,     name,
                                  0)){
                        err = "Connection Problems with the NAME_SERVER";
                    }
                    if (aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                                 LOCAL_GET_SHORT,   &shrt,
                                 0)){
                        err = "Connection Problems with the NAME_SERVER";
                    }
                }
                else {
                    shrt = strdup(name);
                }
                if (!err) {
                    if (GBS_read_hash(hash,shrt)) {
                        char *newshrt;
                        int i;
                        newshrt = (char *)GB_calloc(sizeof(char),strlen(shrt)+20);
                        for (i= 1 ; ; i++) {
                            sprintf(newshrt,"%s.%i",shrt,i);
                            if (!GBS_read_hash(hash,newshrt))break;
                        }
                        warning = "There are duplicated entries!!.\n"
                            "The duplicated entries contain a '.' character in field 'name'!\n"
                            "Please resolve this problem (see HELP in 'Generate new names' window)";
                        free(shrt);
                        shrt = newshrt;
                    }
                    GBS_incr_hash(hash,shrt);

                    err = GBT_rename_species(name,shrt);
                }

                free(name);
                free(acc);
                free(full_name);
                free(shrt);
            }

            if (err) {
                GBT_abort_rename_session();
            }
            else {
                // aw_status("Renaming species in trees");
                // aw_status((double)0);
                GBT_commit_rename_session(aw_status, aw_status);
            }

            GBS_free_hash(hash);
            free(ali_name);

            if (!err) err = warning;
        }
        name_server.disconnect();
    }

    return err;
}


void awt_rename_cb(AW_window *aww,GBDATA *gb_main)
{
    AWUSE(aww);
    //  int use_advice = (int)aww->get_root()->awar(AWT_RENAME_USE_ADVICE)->read_int();
    //  int save_data  = (int)aww->get_root()->awar(AWT_RENAME_SAVE_DATA)->read_int();
    aw_openstatus("Generating new names");
    aw_status("Contacting name server");
    GB_ERROR error     = AWTC_pars_names(gb_main,1);
    aw_closestatus();
    if (error) aw_message(error);

    aww->get_root()->awar(AWAR_TREE_REFRESH)->touch();
}


AW_window *AWTC_create_rename_window(AW_root *root, AW_CL gb_main)
{
    AWUSE(root);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "AUTORENAME_SPECIES", "AUTORENAME SPECIES");

    aws->load_xfig("awtc/autoren.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"rename.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP","H");

    aws->at("go");
    aws->highlight();
    aws->callback((AW_CB1)awt_rename_cb,gb_main);
    aws->create_button("GO", "GO","G");

    //  aws->at("advice");
    //  aws->create_option_menu(AWT_RENAME_USE_ADVICE,0,0);
    //  aws->insert_option("Create totally new names","n",0);
    //  aws->insert_default_option("Use Advice","U",1);
    //  aws->update_option_menu();

    //  aws->at("save");
    //  aws->create_option_menu(AWT_RENAME_SAVE_DATA,0,0);
    //  aws->insert_option("dont update","f",0);
    //  aws->insert_default_option("update $ARBHOME/lib/nas/names.dat","s",1);
    //  aws->update_option_menu();

    return (AW_window *)aws;
}

void AWTC_create_rename_variables(AW_root *root,AW_default db1){
    root->awar_int( AWT_RENAME_USE_ADVICE, 0  ,     db1);
    root->awar_int( AWT_RENAME_SAVE_DATA, 1  ,  db1);
}

UniqueNameDetector::UniqueNameDetector(GBDATA *gb_item_data, int additionalEntries) {
    hash = GBS_create_hash(2*(GB_number_of_subentries(gb_item_data)+additionalEntries), 1);

    for (GBDATA *gb_item = GB_find(gb_item_data, 0, 0, down_level);
         gb_item;
         gb_item = GB_find(gb_item, 0, 0, this_level|search_next))
    {
        GBDATA *gb_name = GB_find(gb_item, "name", 0, down_level);
        if (gb_name) { // item has name -> insert to hash
            GBS_write_hash(hash, GB_read_char_pntr(gb_name), 1);
        }
    }
}

UniqueNameDetector::~UniqueNameDetector() { GBS_free_hash(hash); }

// inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data) {
    // return GBT_find_species_rel_species_data(gb_species_data, short_name)==0;
// }

static char *makeUniqueShortName(const char *prefix, UniqueNameDetector& existing) {
    // generates a non-existing short-name (name starts with prefix)
    // returns 0 if failed

    char *result     = 0;
    int   prefix_len = strlen(prefix);

    gb_assert(prefix_len<8); // prefix has to be shorter than 8 chars!
    if (prefix_len<8) {
        const int max_nums[8] = { 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10 };
        static int next_try[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

        int  max_num = max_nums[prefix_len];
        char short_name[9];
        strcpy(short_name, prefix);

        char *dig_pos = short_name+prefix_len;
        int   num     = next_try[prefix_len];
        int   stop    = num ? num-1 : max_num;

        while (num != stop) {
            sprintf(dig_pos, "%i", num);
            ++num;
            if (!existing.name_known(short_name))  {
                result = strdup(short_name);
                break;
            }
            if (num == max_num && stop != max_num) num = 0;
        }
        if (num == max_num) num = 0; 
        next_try[prefix_len] = num;
    }
    return result;
}

char *AWTC_makeUniqueShortName(const char *prefix, UniqueNameDetector& existingNames) {
    // generates a unique species name from prefix
    // (prefix will be fillup with zero digits and then shortened down to first char)
    // returns 0 if fails

    int  len = strlen(prefix);
    char p[9];
    strncpy(p, prefix, 8);

    if (len>8) len = 8;
    else {
        if (len == 0) p[len++] = 'x'; // don't use digit as first character
        while (len<8) p[len++] = '0';
    }

    p[len] = 0;

    char *result = 0;

    for (int l = len-1; l>0 && !result; --l) {
        p[l]   = 0;
        result = makeUniqueShortName(p, existingNames);
    }

    gb_assert(!result || strlen(result) <= 8);

    return result;
}

char *AWTC_generate_random_name(UniqueNameDetector& existingNames) {
    char *new_species_name = 0;
    char  short_name[9];
    int   count            = 10000;

    short_name[8] = 0;
    while (count--) {
        short_name[0] = 'a'+GB_random(26); // first character has to be alpha

        for (int x=1; x<8; ++x) {
            int r = GB_random(36); // rest may be alphanumeric
            short_name[x] = r<10 ? ('0'+r) : ('a'+r-10);
        }

        if (!existingNames.name_known(short_name))  {
            new_species_name = strdup(short_name);
            break;
        }
    }

    if (!new_species_name) {
        aw_message("Failed to generate a random name - retrying (this might hang forever)");
        return AWTC_generate_random_name(existingNames);
    }

    return new_species_name;
}
