// =============================================================== //
//                                                                 //
//   File      : ali_pt.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ali_pt.hxx"

int ALI_PT::init_communication() {
    // Initialize the communication with the pt server
    const char *user = GB_getenvUSER();
    if (aisc_create(link, PT_MAIN, com,
                    MAIN_LOCS, PT_LOCS, locs,
                    LOCS_USER, user,
                    NULL))
    {
        return 1;
    }
    return 0;
}

char *ALI_PT::get_family_member(char *specifiedfamily, unsigned long number)
{
    char *ptr = specifiedfamily;
    char *end;
    char *buffer, *dest;

    while (number > 0 && *ptr != '\0' && *ptr != ';') {
        while (*ptr != '\0' && *ptr != ';' && *ptr != ',')
            ptr++;
        if (*ptr == ',')
            ptr++;
        number--;
    }

    if (*ptr != '\0' && *ptr != ';') {
        end = ptr;
        while (*end != '\0' && *end != ',' && *end != ';')
            end++;

        buffer = dest = (char *) CALLOC((unsigned int)
                                        (end - ptr) + 1, sizeof(char));
        if (buffer == 0)
            ali_fatal_error("Out of memory");

        while (ptr != end)
            *dest++ = *ptr++;
        *dest = '\0';

        return buffer;
    }
    return 0;
}

char *ALI_PT::get_extension_member(char *specifiedfamily,    unsigned long number)
{
    char *ptr = specifiedfamily;
    char *end;
    char *buffer, *dest;

    while (*ptr != '\0' && *ptr != ';')
        ptr++;
    if (*ptr == ';')
        ptr++;

    while (number > 0 && *ptr != '\0') {
        while (*ptr != '\0' && *ptr != ',')
            ptr++;
        if (*ptr == ',')
            ptr++;
        number--;
    }

    if (*ptr != '\0') {
        end = ptr;
        while (*end != '\0' && *end != ',')
            end++;

        buffer = dest = (char *) CALLOC((unsigned int) (end - ptr) + 1, sizeof(char));
        if (buffer == 0)
            ali_fatal_error("Out of memory");

        while (ptr != end)
            *dest++ = *ptr++;
        *dest = '\0';

        return buffer;
    }

    return 0;
}


int ALI_PT::open(char *servername)
{
    if (arb_look_and_start_server(AISC_MAGIC_NUMBER, servername)) {
        ali_message ("Cannot contact Probe bank server");
        return -1;
    }

    const char *socketid = GBS_read_arb_tcp(servername);

    if (!socketid) {
        ali_message(GB_await_error());
        return -1;
    }

    GB_ERROR openerr = NULL;
    link             = aisc_open(socketid, com, AISC_MAGIC_NUMBER, &openerr);

    if (openerr) {
        ali_message (openerr);
        return -1;
    }

    if (!link) {
        ali_message ("Cannot contact Probe bank server ");
        return -1;
    }

    if (init_communication()) {
        ali_message ("Cannot contact Probe bank server (2)");
        return -1;
    }

    return 0;
}

void ALI_PT::close()
{
    if (link) aisc_close(link, com);
    link = 0;
}

ALI_PT::ALI_PT(ALI_PT_CONTEXT *context)
{
    link = 0;

    fam_list_max = context->fam_list_max;
    ext_list_max = context->ext_list_max;
    percent_min = context->percent_min;
    matches_min = context->matches_min;

    family_list = 0;
    extension_list = 0;

    if (context->use_specified_family != 0) {
        mode = SpecifiedMode;
        specified_family = ARB_strdup(context->use_specified_family);
    }
    else {
        mode = ServerMode;
        specified_family = 0;

        ali_message("Connecting to PT server");
        if (open(context->servername) != 0) {
            ali_fatal_error("Can't connect to PT server");
        }
        ali_message("Connection established");
    }
}

ALI_PT::~ALI_PT()
{
    close();

    if (family_list != 0 && !family_list->is_empty()) {
        delete family_list->first();
        while (family_list->is_next())
            delete family_list->next();
        delete family_list;
    }
    if (extension_list != 0 && !extension_list->is_empty()) {
        delete extension_list->first();
        while (extension_list->is_next())
            delete extension_list->next();
        delete extension_list;
    }
}


int ALI_PT::find_family(ALI_SEQUENCE *sequence, int find_type)
{
    unsigned long number;
    int matches, max_matches;
    char *seq_name;
    T_PT_FAMILYLIST f_list;
    ali_pt_member *pt_member;
    char *species;

    bytestring bs;
    bs.data = sequence->string();
    bs.size = strlen(bs.data)+1;

    family_list = new ALI_TLIST<ali_pt_member *>;
    extension_list = new ALI_TLIST<ali_pt_member *>;
    if (family_list == 0 || extension_list == 0)
        ali_fatal_error("Out of memory");

    if (mode == ServerMode) {
        /* Start find_family() at the PT_SERVER
         *
         * Here we have to make a loop, until the match count of the
         * first member is big enought
         */

        T_PT_FAMILYFINDER ffinder;
        if (aisc_create(link, PT_LOCS, locs,
                        LOCS_FFINDER, PT_FAMILYFINDER, ffinder, 
                        FAMILYFINDER_FIND_TYPE,   (long)find_type,
                        FAMILYFINDER_FIND_FAMILY, &bs,
                        NULL))
        {
            ali_message ("Communication Error (2)");
            return -1;
        }

        // Read family list
        aisc_get(link, PT_FAMILYFINDER, ffinder,
                 FAMILYFINDER_FAMILY_LIST, f_list.as_result_param(),
                 NULL);
        if (!f_list.exists())
            ali_error("Family not found in PT Server");

        aisc_get(link, PT_FAMILYLIST, f_list,
                 FAMILYLIST_NAME,    &seq_name,
                 FAMILYLIST_MATCHES, &matches,
                 FAMILYLIST_NEXT,    f_list.as_result_param(),
                 NULL);

        while (strcmp(seq_name, sequence->name()) == 0) {
            free(seq_name);
            if (!f_list.exists())
                ali_error("Family too small in PT Server");
            aisc_get(link, PT_FAMILYLIST, f_list,
                     FAMILYLIST_NAME,    &seq_name,
                     FAMILYLIST_MATCHES, &matches,
                     FAMILYLIST_NEXT,    f_list.as_result_param(),
                     NULL);
        }
        // found the first element

        // make the family list
        max_matches = matches;
        number = 0;
        do {
            pt_member = new ali_pt_member(seq_name, matches);
            family_list->append_end(pt_member);
            number++;
            do {
                if (!f_list.exists())
                    ali_error("Family too small in PT Server");
                aisc_get(link, PT_FAMILYLIST, f_list,
                         FAMILYLIST_NAME,    &seq_name,
                         FAMILYLIST_MATCHES, &matches,
                         FAMILYLIST_NEXT,    f_list.as_result_param(),
                         NULL);
                if (strcmp(seq_name, sequence->name()) == 0)
                    free(seq_name);
            } while (strcmp(seq_name, sequence->name()) == 0);
        } while (number < fam_list_max &&
                 (float) matches / (float) max_matches > percent_min);

        // make the extension list
        number = 0;
        while (number < ext_list_max) {
            pt_member = new ali_pt_member(seq_name, matches);
            extension_list->append_end(pt_member);
            number++;
            do {
                if (!f_list.exists())
                    ali_error("Family too small in PT Server");
                aisc_get(link, PT_FAMILYLIST, f_list,
                         FAMILYLIST_NAME,    &seq_name,
                         FAMILYLIST_MATCHES, &matches,
                         FAMILYLIST_NEXT,    f_list.as_result_param(),
                         NULL);
                if (strcmp(seq_name, sequence->name()) == 0)
                    free(seq_name);
            } while (strcmp(seq_name, sequence->name()) == 0);
        }
    }
    else {
        number = 0;
        while ((species = get_family_member(specified_family, number)) != 0) {
            pt_member = new ali_pt_member(species, matches_min);
            if (pt_member == 0)
                ali_fatal_error("Out of memory");
            family_list->append_end(pt_member);
            number++;
        }

        if (number == 0)
            ali_fatal_error("Specified family too small");

        number = 0;
        while ((species = get_extension_member(specified_family, number)) != 0) {
            pt_member = new ali_pt_member(species,
                                          (int) (matches_min * percent_min) - 1);
            if (pt_member == 0)
                ali_fatal_error("Out of memory");
            extension_list->append_end(pt_member);
            number++;
        }
    }

    free(bs.data);
    return 0;
}


ALI_TLIST<ali_pt_member *> *ALI_PT::get_family_list()
{
    ALI_TLIST<ali_pt_member *> *ret;

    ret = family_list;
    family_list = 0;

    return ret;
}


ALI_TLIST<ali_pt_member *> *ALI_PT::get_extension_list()
{
    ALI_TLIST<ali_pt_member *> *ret;

    ret = extension_list;
    extension_list = 0;

    return ret;
}

