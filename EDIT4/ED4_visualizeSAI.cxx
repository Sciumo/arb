// =============================================================== //
//                                                                 //
//   File      : ED4_visualizeSAI.cxx                              //
//   Purpose   : Visualize sequence associated information (SAI)   //
//               in the Editor                                     //
//                                                                 //
//   Coded by Yadhu Kumar                                          //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <ed4_extern.hxx>
#include "ed4_class.hxx"

#include <aw_awars.hxx>
#include <awt_canvas.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_select.hxx>

#include <arbdbt.h>
#include <ad_cb.h>
#include <arb_strbuf.h>

#include <iostream>

// --------------------------------------------------------------------------------

#define AWAR_SAI_CLR_TAB              "saicolors/"
#define AWAR_SAI_VISUALIZED           AWAR_SAI_CLR_TAB "current"             // current visualized SAI
#define AWAR_SAI_CLR_DEF              AWAR_SAI_CLR_TAB "clr_trans_tab/"      // container for definitions
#define AWAR_SAI_ENABLE               AWAR_SAI_CLR_TAB "enable"              // global enable of visualization
#define AWAR_SAI_ALL_SPECIES          AWAR_SAI_CLR_TAB "all_species"         // 1 = all / 0 = marked
#define AWAR_SAI_AUTO_SELECT          AWAR_SAI_CLR_TAB "auto_select"         // 1 = auto select / 0 = manual select
#define AWAR_SAI_CLR_TRANS_TABLE      AWAR_SAI_CLR_TAB "clr_trans_table"     // current translation table
#define AWAR_SAI_CLR_TRANS_TAB_NAMES  AWAR_SAI_CLR_TAB "clr_trans_tab_names" // ;-separated list of existing translation tables
#define AWAR_SAI_CLR_TRANS_TAB_REL    AWAR_SAI_CLR_TAB "sai_relation/"       // container to store trans tables for each SAI
#define AWAR_SAI_CLR_DEFAULTS_CREATED AWAR_SAI_CLR_TAB "defaults_created"    // whether defaults have been created (create only once)

#define AWAR_SAI_CLR_TRANS_TAB_NEW_NAME "tmp/sai/clr_trans_tab_new_name" // textfield to enter translation table name
#define AWAR_SAI_CLR                    "tmp/sai/color_0" // the definition of the current translation table (number runs from 0 to 9)
#define AWAR_SAI_CLR_COUNT              10

// --------------------------------------------------------------------------------

extern GBDATA *GLOBAL_gb_main;

static bool clrDefinitionsChanged       = false;
static bool inCallback                  = false; // used to avoid multiple refreshes
static bool in_colorDefChanged_callback = false; // used to avoid colorDef correction

// --------------------------------------------------------------------------------

#define BUFSIZE 100
static const char *getAwarName(int awarNo) {
    static char buf[BUFSIZE];

    strcpy(buf, AWAR_SAI_CLR);
    (strchr(buf, 0)-1)[0] = '0'+awarNo;

    return buf;
}

static const char *getClrDefAwar(const char *awarName) {
    static char buf[BUFSIZE];

    e4_assert(awarName);
    e4_assert(awarName[0]); // empty awar is bad

    IF_ASSERTION_USED(int size =) sprintf(buf, AWAR_SAI_CLR_DEF "%s", awarName);
    e4_assert(size<BUFSIZE);
    return buf;
}
#undef BUFSIZE

// ---------------------------------------------------------

static void setVisualizeSAI_cb(AW_root *awr) {
    ED4_ROOT->visualizeSAI = awr->awar(AWAR_SAI_ENABLE)->read_int();
    ED4_ROOT->request_refresh_for_sequence_terminals();
}

static void setVisualizeSAI_options_cb(AW_root *awr) {
    ED4_ROOT->visualizeSAI_allSpecies = awr->awar(AWAR_SAI_ALL_SPECIES)->read_int();
    ED4_ROOT->request_refresh_for_sequence_terminals();
}

static bool colorTransTable_exists(AW_root *awr, const char *name) {
    char       *tableNames = awr->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES)->read_string();
    const char *searchFrom = tableNames;
    int         len        = strlen(name);

    while (searchFrom) {
        const char *found = strstr(searchFrom, name);

        if (found) {
            if ((found == tableNames || found[-1] == '\n') && // found start of entry
                (found[len] == '\n' || found[len] == 0)) // avoid partial entry
            {
                break; // exists!
            }
            else {              // search next match
                searchFrom = found+1;
            }
        }
        else {
            searchFrom = 0;
        }
    }

    free(tableNames);
    return searchFrom != 0;
}

static void colorDefChanged_callback(AW_root *awr, int awarNo) {
    clrDefinitionsChanged = true;

    if (!in_colorDefChanged_callback) { // this callback is special, because it may change all other color defs
        LocallyModify<bool> flag(in_colorDefChanged_callback, true);

        {
            LocallyModify<bool> in_cb(inCallback, true);

            char *clrTabName = awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();
            if (clrTabName[0]) {
                unsigned char charUsed[256]; memset(charUsed, 255, 256);

                {
                    for (int i=0; i<10;  i++) {
                        char *awarString_next = awr->awar_string(getAwarName(i))->read_string();
                        for (int c=0; awarString_next[c]; ++c) {
                            charUsed[(unsigned char)awarString_next[c]] = i;
                        }
                        free(awarString_next);
                    }

                    char *awarString = awr->awar_string(getAwarName(awarNo))->read_string();
                    for (int c=0; awarString[c]; ++c) {
                        charUsed[(unsigned char)awarString[c]] = awarNo;
                    }
                    free(awarString);
                }

                typedef unsigned char mystr[256];
                mystr s[10];
                for (int i=0; i<10; i++)  s[i][0]=0; // initializing the strings

                for (int i=0; i<256; i++) {
                    int table = charUsed[i];
                    if (table != 255) {
                        char *eos = strchr((char *)s[table], 0); // get pointer to end of string
                        eos[0] = char(i);
                        eos[1] = 0;
                    }
                }

                {
                    GBS_strstruct clrDefStr(500);
                    for (int i=0; i<10; i++) {
                        awr->awar_string(getAwarName(i))->write_string((char *)s[i]);

                        char *escaped = GBS_escape_string((char*)s[i], ";", '&');
                        clrDefStr.cat(escaped);
                        free(escaped);

                        clrDefStr.put(';');
                    }

                    AW_awar *awar_def = awr->awar_string(getClrDefAwar(clrTabName), "", AW_ROOT_DEFAULT);
                    awar_def->write_string(clrDefStr.get_data());
                }
            }
            else {
                if (!in_cb.old_value()) { // only warn when user really changed the setting
                    aw_message("Please select a VALID Color Translation Table to EDIT.");
                }
            }
            free(clrTabName);
        }

        if (!inCallback) ED4_ROOT->request_refresh_for_sequence_terminals();
    }
}

static void colorDefTabNameChanged_callback(AW_root *awr) {
    char *clrTabName     = awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();
    {
        LocallyModify<bool> flag(inCallback, true);
        {
            LocallyModify<bool> flag2(in_colorDefChanged_callback, true); // avoid correction

            // clear current translation table definition
            for (int i=0; i<10; i++) {
                AW_awar *transDef_awar = awr->awar_string(getAwarName(i), "", AW_ROOT_DEFAULT);
                transDef_awar->write_string("");
            }

            if (clrTabName[0]) {
                AW_awar *clrTabDef_awar = awr->awar_string(getClrDefAwar(clrTabName), "", AW_ROOT_DEFAULT);
                char    *clrTabDef      = clrTabDef_awar->read_string();

                if (clrTabDef[0]) {
                    int i        = 0;
                    int tokStart = 0;

                    for (int si = 0; clrTabDef[si]; ++si) {
                        if (clrTabDef[si] == ';') {
                            e4_assert(i >= 0 && i<10);
                            AW_awar *awar = awr->awar(getAwarName(i));

                            if (tokStart == si) { // empty definition
                                awar->write_string("");
                            }
                            else {
                                int toklen = si-tokStart;

                                e4_assert(toklen > 0);
                                e4_assert(clrTabDef[tokStart+toklen] == ';');
                                clrTabDef[tokStart+toklen] = 0;

                                char *unescaped = GBS_unescape_string(clrTabDef+tokStart, ";", '&');
                                awar->write_string(unescaped);
                                free(unescaped);

                                clrTabDef[tokStart+toklen] = ';';
                            }
                            ++i;
                            tokStart = si+1;
                        }
                    }
                    e4_assert(i == 10);
                }
                free(clrTabDef);
            }
        }
        colorDefChanged_callback(awr, 0); // correct first def manually
        {
            // store the selected table as default for this SAI:
            char *saiName = awr->awar(AWAR_SAI_VISUALIZED)->read_string();
            if (saiName[0]) {
                char buf[100];
                sprintf(buf, AWAR_SAI_CLR_TRANS_TAB_REL "%s", saiName);
                awr->awar_string(buf, "", AW_ROOT_DEFAULT); // create an AWAR for the selected SAI and
                awr->awar(buf)->write_string(clrTabName); // write the existing clr trans table names to the same
            }
            free(saiName);
        }
    }
    free(clrTabName);

    if (!inCallback && clrDefinitionsChanged) ED4_ROOT->request_refresh_for_sequence_terminals();
}

static void refresh_display_cb(GB_CB_TYPE cb_type) {
    if ((cb_type & GB_CB_CHANGED) &&
        ED4_ROOT->aw_root->awar(AWAR_SAI_ENABLE)->read_int())
    {
        clrDefinitionsChanged = 1;
        ED4_ROOT->request_refresh_for_sequence_terminals();
    }
}

static void saiChanged_callback(AW_root *awr) {
    char *saiName = 0;
    {
        LocallyModify<bool> flag(inCallback, true);
        {
            static GBDATA *gb_last_SAI = 0;

            if (gb_last_SAI) {
                GB_transaction ta(GLOBAL_gb_main);
                GB_remove_callback(gb_last_SAI, GB_CB_CHANGED, makeDatabaseCallback(refresh_display_cb));
                gb_last_SAI = 0;
            }

            saiName = awr->awar(AWAR_SAI_VISUALIZED)->read_string();
            char *transTabName = 0;

            if (saiName[0]) {
                char  buf[100];
                sprintf(buf, AWAR_SAI_CLR_TRANS_TAB_REL "%s", saiName);
                awr->awar_string(buf, "", AW_ROOT_DEFAULT);
                transTabName = awr->awar(buf)->read_string();
            }

            {
                GB_transaction ta(GLOBAL_gb_main);
                gb_last_SAI = GBT_find_SAI(GLOBAL_gb_main, saiName);
                if (gb_last_SAI) {
                    GB_add_callback(gb_last_SAI, GB_CB_CHANGED, makeDatabaseCallback(refresh_display_cb));
                }
            }
            awr->awar(AWAR_SAI_CLR_TRANS_TABLE)->write_string(transTabName ? transTabName : "");
            free(transTabName);

            clrDefinitionsChanged = true; // SAI changed -> update needed
        }
    }
    
    if (!inCallback && clrDefinitionsChanged) {
        // SAI changed notify Global SAI Awar AWAR_SAI_GLOBAL
        awr->awar(AWAR_SAI_GLOBAL)->write_string(saiName);
        ED4_ROOT->request_refresh_for_sequence_terminals();
    }
    free(saiName);
}

static void update_ClrTransTabNamesList_cb(AW_root *awr, AW_selection_list *colorTransList) {
    char *clrTransTabNames = awr->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES)->read_string();

    colorTransList->clear();

    for (char *tok = strtok(clrTransTabNames, "\n"); tok; tok = strtok(0, "\n")) {
        colorTransList->insert(tok, tok);
    }
    colorTransList->insert_default("????", "");
    colorTransList->update();

    free(clrTransTabNames);
}

static void autoselect_cb(AW_root *aw_root) {
    char *curr_sai = aw_root->awar(AWAR_SAI_NAME)->read_string();
#if defined(DEBUG)
    printf("curr_sai='%s'\n", curr_sai);
#endif // DEBUG
    aw_root->awar(AWAR_SAI_VISUALIZED)->write_string(curr_sai);
    free(curr_sai);
}

static void set_autoselect_cb(AW_root *aw_root) {
    static bool callback_active = false;

    if (aw_root->awar(AWAR_SAI_AUTO_SELECT)->read_int()) { // auto select is activated
        aw_root->awar(AWAR_SAI_NAME)->add_callback(autoselect_cb);
        callback_active = true;
    }
    else {
        if (callback_active) { // only remove if added
            aw_root->awar(AWAR_SAI_NAME)->remove_callback(autoselect_cb);
            callback_active = false;
        }
    }
}

static void addOrUpdateTransTable(AW_root *aw_root, const char *newClrTransTabName, const char *defaultDefinition, bool autoselect) {
    AW_awar *table_def_awar = aw_root->awar_string(getClrDefAwar(newClrTransTabName), defaultDefinition,  AW_ROOT_DEFAULT);
    table_def_awar->write_string(defaultDefinition);

    if (!colorTransTable_exists(aw_root, newClrTransTabName)) {
        AW_awar    *names_awar = aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES);
        const char *old_names  = names_awar->read_char_pntr();
        names_awar->write_string(old_names[0]
                                 ? GBS_global_string("%s\n%s", old_names, newClrTransTabName)
                                 : newClrTransTabName); // add new name
    }

    if (autoselect) {
        aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->write_string(newClrTransTabName); // select new
    }
}

static void addDefaultTransTable(AW_root *aw_root, const char *newClrTransTabName, const char *defaultDefinition) {
    addOrUpdateTransTable(aw_root, newClrTransTabName, defaultDefinition, false);
}

void ED4_createVisualizeSAI_Awars(AW_root *aw_root, AW_default aw_def) {  // --- Creating and initializing AWARS -----
    aw_root->awar_int(AWAR_SAI_ENABLE,      0, aw_def);
    aw_root->awar_int(AWAR_SAI_ALL_SPECIES, 0, aw_def);
    aw_root->awar_int(AWAR_SAI_AUTO_SELECT, 0, aw_def);

    aw_root->awar_string(AWAR_SAI_VISUALIZED,             "", aw_def);
    aw_root->awar_string(AWAR_SAI_CLR_TRANS_TABLE,        "", aw_def);
    aw_root->awar_string(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME, "", aw_def);
    aw_root->awar_string(AWAR_SAI_CLR_TRANS_TAB_NAMES,    "", aw_def);

    for (int i=0; i<10; i++) { // initializing 10 color definition string AWARS
       AW_awar *def_awar = aw_root->awar_string(getAwarName(i), "", aw_def);
       def_awar->add_callback(makeRootCallback(colorDefChanged_callback, i));
    }
    aw_root->awar(AWAR_SAI_ENABLE)         ->add_callback(setVisualizeSAI_cb);
    aw_root->awar(AWAR_SAI_ALL_SPECIES)    ->add_callback(setVisualizeSAI_options_cb);
    aw_root->awar(AWAR_SAI_AUTO_SELECT)    ->add_callback(set_autoselect_cb);
    aw_root->awar(AWAR_SAI_VISUALIZED)     ->add_callback(saiChanged_callback);
    aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->add_callback(colorDefTabNameChanged_callback);

    ED4_ROOT->visualizeSAI            = aw_root->awar(AWAR_SAI_ENABLE)->read_int();
    ED4_ROOT->visualizeSAI_allSpecies = aw_root->awar(AWAR_SAI_ALL_SPECIES)->read_int();

    // create some defaults:
    aw_root->awar_int(AWAR_SAI_CLR_DEFAULTS_CREATED, 1,  aw_def); // @@@ Feb 2012 - remove me in some years

    addDefaultTransTable(aw_root, "numeric",   "0;1;2;3;4;5;6;7;8;9;");
    addDefaultTransTable(aw_root, "binary",    ".0;;+1;;;;;;;;");
    addDefaultTransTable(aw_root, "consensus", "=ACGTU;;acgtu;.;;;;;;;");
    addDefaultTransTable(aw_root, "helix",     ";;<>;;;;;[];;;");
    addDefaultTransTable(aw_root, "xstring",   ";x;;;;;;;;;");
    addDefaultTransTable(aw_root, "gaps",      ";-.;;;;;;;;;");

    LocallyModify<bool> flag(inCallback, true); // avoid refresh
    saiChanged_callback(aw_root);
    colorDefTabNameChanged_callback(aw_root); // init awars for selected table
    set_autoselect_cb(aw_root);
}

enum CreationMode {
    ED4_VIS_CREATE, // creates a new (empty) color translation table
    ED4_VIS_COPY,   // copies the selected color translation table
};

static void createCopyClrTransTable(AW_window *aws, CreationMode mode) {
    AW_root *aw_root = aws->get_root();

    char *newClrTransTabName = 0;
    char *clrTabSourceName   = 0;

    switch (mode) {
        case ED4_VIS_CREATE:
            newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME)->read_char_pntr());

            if (strcmp(newClrTransTabName, "__") == 0) { // user entered nothing
                aw_message("Please enter a translation table name");
            }
            else if (colorTransTable_exists(aw_root, newClrTransTabName)) {
                aw_message(GBS_global_string("Color translation table '%s' already exists.", newClrTransTabName));
            }
            else {
                addOrUpdateTransTable(aw_root, newClrTransTabName, "", true);
            }
            break;

        case ED4_VIS_COPY:
            newClrTransTabName = GBS_string_2_key(aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME)->read_char_pntr());
            clrTabSourceName   = aw_root->awar(AWAR_SAI_CLR_TRANS_TABLE)->read_string();

            if (!clrTabSourceName[0]) {
                aw_message("Please select a valid Color Translation Table to COPY!");
            }
            else if (colorTransTable_exists(aw_root, newClrTransTabName)) {
                aw_message(GBS_global_string("Color Translation Table \"%s\" EXISTS! Please enter a different name.", newClrTransTabName));
            }
            else {
                char *old_def = aw_root->awar(getClrDefAwar(clrTabSourceName))->read_string();
                addOrUpdateTransTable(aw_root, newClrTransTabName, old_def, true);
                free(old_def);
            }
            break;
    }

    free(clrTabSourceName);
    free(newClrTransTabName);
}

static void deleteColorTranslationTable(AW_window *aws) {
    AW_root *aw_root = aws->get_root();
    const char *clrTabName = aw_root->awar_string(AWAR_SAI_CLR_TRANS_TABLE)->read_char_pntr();

    if (clrTabName[0]) {
        AW_awar       *awar_tabNames    = aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES);
        char          *clrTransTabNames = awar_tabNames->read_string();
        GBS_strstruct newTransTabName(strlen(clrTransTabNames));

        for (const char *tok = strtok(clrTransTabNames, "\n"); tok; tok = strtok(0, "\n")) {
            if (strcmp(clrTabName, tok) != 0) { // merge all not to delete
                newTransTabName.cat(tok);
                newTransTabName.put('\n');
            }
        }

        aw_root->awar_string(getClrDefAwar(clrTabName))->write_string("");
        awar_tabNames->write_string(newTransTabName.get_data()); // updates selection list

        free(clrTransTabNames);
    }
    else {
        aw_message("Selected Color Translation Table is not VALID and cannot be DELETED!");
    }
}

static AW_selection_list *buildClrTransTabNamesList(AW_window *aws) {
    AW_root           *awr            = aws->get_root();
    AW_selection_list *colorTransList = aws->create_selection_list(AWAR_SAI_CLR_TRANS_TABLE, true);

    update_ClrTransTabNamesList_cb(awr, colorTransList);

    return colorTransList;
}

const char *ED4_getSaiColorString(AW_root *awr, int start, int end) {
    static int   seqBufferSize = 0;
    static char *saiColors     = 0;
    static int   lastStart     = -1;
    static int   lastEnd       = -1;
    static bool  lastVisualize = false;

    e4_assert(start<=end);

    if (lastStart==start && lastEnd==end  && !clrDefinitionsChanged && lastVisualize) {
        return saiColors-start;    // if start and end positions are same as the previous positions and no settings
    }                              // were changed return the last calculated saiColors string

    lastStart = start; lastEnd = end; clrDefinitionsChanged = false; // storing start and end positions

    int seqSize = end-start+1;

    if (seqSize>seqBufferSize) {
        free(saiColors);
        seqBufferSize = seqSize;
        ARB_calloc(saiColors, seqBufferSize);
    }
    else memset(saiColors, 0, sizeof(char)*seqSize);

    char *saiSelected = awr->awar(AWAR_SAI_VISUALIZED)->read_string();

    GB_push_transaction(GLOBAL_gb_main);
    char   *alignment_name = GBT_get_default_alignment(GLOBAL_gb_main);
    GBDATA *gb_extended    = GBT_find_SAI(GLOBAL_gb_main, saiSelected);
    bool    visualize      = false; // set to true if all goes fine

    if (gb_extended) {
        GBDATA *gb_ali = GB_entry(gb_extended, alignment_name);
        if (gb_ali) {
            const char *saiData      = 0;
            bool        free_saiData = false;

            {
                GBDATA *saiSequence = GB_entry(gb_ali, "data"); // search "data" entry (normal SAI)
                if (saiSequence) {
                    saiData = GB_read_char_pntr(saiSequence); // not allocated
                }
                else {
                    saiSequence = GB_entry(gb_ali, "bits"); // search "bits" entry (binary SAI)
                    if (saiSequence) {
                        saiData      = GB_read_as_string(saiSequence); // allocated
                        free_saiData = true; // free saiData below
                    }
                }
            }

            if (saiData) {
                char trans_table[256];
                {
                    // build the translation table:
                    memset(trans_table, 0, 256);
                    for (int i = 0; i<AWAR_SAI_CLR_COUNT; ++i) {
                        char *def      = awr->awar(getAwarName(i))->read_string();
                        int   clrRange = ED4_G_CBACK_0 + i;

                        for (char *d = def; *d; ++d) {
                            trans_table[(unsigned char)*d] = clrRange;
                        }
                        free(def);
                    }
                }

                // translate SAI to colors
                for (int i = start; i <= end; ++i) {
                    saiColors[i-start] = trans_table[(unsigned char)saiData[i]];
                }

                visualize = true;
            }

            if (free_saiData) {
                free(const_cast<char*>(saiData)); // in this case saiData is allocated (see above)
            }
        }
    }
    free(alignment_name);
    free(saiSelected);
    GB_pop_transaction(GLOBAL_gb_main);

    lastVisualize = visualize;
    if (visualize) {
        e4_assert(saiColors);
        return saiColors-start;
    }

    return 0; // don't visualize (sth went wrong)
}


// -------------------- Creating Windows and Display dialogs --------------------

static AW_window *create_copyColorTranslationTable_window(AW_root *aw_root) { // creates copy color translation table window
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "COPY_CLR_TR_TABLE", "Copy Color Translation Table");
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME, 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(createCopyClrTransTable, ED4_VIS_COPY));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static AW_window *create_createColorTranslationTable_window(AW_root *aw_root) { // creates create color translation table window
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "CREATE_CLR_TR_TABLE", "Create Color Translation Table");
    aws->load_xfig("ad_al_si.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nfor the Color Translation Table");

    aws->at("input");
    aws->create_input_field(AWAR_SAI_CLR_TRANS_TAB_NEW_NAME, 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(createCopyClrTransTable, ED4_VIS_CREATE));
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static void reverseColorTranslationTable(AW_window *aww) {
    AW_root *aw_root = aww->get_root();

    int j = AWAR_SAI_CLR_COUNT-1;
    for (int i = 0; i<AWAR_SAI_CLR_COUNT/2+1; ++i, --j) {
        if (i<j) {
            AW_awar *aw_i = aw_root->awar(getAwarName(i));
            AW_awar *aw_j = aw_root->awar(getAwarName(j));

            char *ci = aw_i->read_string();
            char *cj = aw_j->read_string();

            aw_i->write_string(cj);
            aw_j->write_string(ci);

            free(ci);
            free(cj);
        }
    }
}

static AW_window *create_editColorTranslationTable_window(AW_root *aw_root) { // creates edit color translation table window
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "EDIT_CTT", "Color Translation Table");
        aws->load_xfig("saiColorRange.fig");

        char at_name[] = "rangex";
        char *dig      = strchr(at_name, 0)-1;

        for (int i = 0; i<AWAR_SAI_CLR_COUNT; ++i) {
            dig[0] = '0'+i;
            aws->at(at_name);
            aws->create_input_field(getAwarName(i), 20);
        }

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("reverse");
        aws->callback(reverseColorTranslationTable);
        aws->create_button("REVERSE", "Reverse", "R");

        aws->at("colors");
        aws->callback(makeWindowCallback(ED4_popup_gc_window, ED4_ROOT->gc_manager));
        aws->button_length(0);
        aws->create_button("COLORS", "#colors.xpm");
    }
    return aws;
}

AW_window *ED4_createVisualizeSAI_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "VISUALIZE_SAI", "Visualize SAIs");
        aws->load_xfig("visualizeSAI.fig");

        aws->callback(makeHelpCallback("visualizeSAI.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("enable");
        aws->create_toggle(AWAR_SAI_ENABLE);

        aws->at("sai");
        aws->button_length(30);
        awt_create_SAI_selection_button(GLOBAL_gb_main, aws, AWAR_SAI_VISUALIZED);

        aws->at("auto_select");
        aws->create_toggle(AWAR_SAI_AUTO_SELECT);

        aws->at("clrTrList");
        AW_selection_list *clrTransTableLst = buildClrTransTabNamesList(aws);

        aws->at("edit");
        aws->button_length(10);
        aws->callback(create_editColorTranslationTable_window);
        aws->create_button("EDIT", "EDIT");

        aws->at("create");
        aws->callback(create_createColorTranslationTable_window);
        aws->create_button("CREATE", "CREATE");

        aws->at("copy");
        aws->callback(create_copyColorTranslationTable_window);
        aws->create_button("COPY", "COPY");

        aws->at("delete");
        aws->callback(deleteColorTranslationTable);
        aws->create_button("DELETE", "DELETE");

        aws->at("marked");
        aws->create_toggle_field(AWAR_SAI_ALL_SPECIES, 1);
        aws->insert_toggle("MARKED SPECIES", "M", 0);
        aws->insert_toggle("ALL SPECIES", "A", 1);
        aws->update_toggle_field();

        AW_awar *trans_tabs = aw_root->awar(AWAR_SAI_CLR_TRANS_TAB_NAMES);
        trans_tabs->add_callback(makeRootCallback(update_ClrTransTabNamesList_cb, clrTransTableLst));
        trans_tabs->touch();        // force update
    }
    aws->show();

    return aws;
}

