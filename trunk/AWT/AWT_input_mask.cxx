//  ==================================================================== //
//                                                                       //
//    File      : AWT_input_mask.cxx                                     //
//    Purpose   : General input masks                                    //
//    Time-stamp: <Fri Sep/07/2001 14:45 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2001           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <arbdb.h>
#include <awt.hxx>
#include <awt_www.hxx>

#ifndef __MAP__
#include <map>
#endif
#ifndef __VECTOR__
#include <vector>
#endif

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "awt_input_mask.hxx"

using namespace std;

const char *awt_itemtype_names[AWT_IT_TYPES+1] =
{
 "Unknown",
 "Species", "Organism", "Gene", "Experiment",
 "<overflow>"
};

#define SEC_XBORDER    3        // space left/right of NEW_SECTION line
#define SEC_YBORDER    4        // space above/below
#define SEC_LINE_WIDTH 1        // line width

#define MIN_TEXTFIELD_SIZE 1
#define MAX_TEXTFIELD_SIZE 1000

// #########################################################
// #########################################################
// ###                                                   ###
// ##          Callbacks from database and awars          ##
// ###                                                   ###
// #########################################################
// #########################################################

static bool in_item_changed_callback  = false;
static bool in_field_changed_callback = false;
static bool in_awar_changed_callback  = false;

//  -------------------------------------------------------------------------------------------------
//      static void item_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type)
//  -------------------------------------------------------------------------------------------------
static void item_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type) {
    if (!in_item_changed_callback) { // avoid deadlock
        in_item_changed_callback   = true;
        awt_input_handler *handler = (awt_input_handler*)cl_awt_input_handler;

        if (type&GB_CB_DELETE) { // handled child was deleted
            handler->relink();
        }
        else if ((type&(GB_CB_CHANGED|GB_CB_SON_CREATED)) == (GB_CB_CHANGED|GB_CB_SON_CREATED)) {
            // child was created (not only changed)
            handler->relink();
        }

        in_item_changed_callback = false;
    }
}

//  --------------------------------------------------------------------------------------------------
//      static void field_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type)
//  --------------------------------------------------------------------------------------------------
static void field_changed_cb(GBDATA *gb_item, int *cl_awt_input_handler, GB_CB_TYPE type) {
    if (!in_field_changed_callback) { // avoid deadlock
        in_field_changed_callback  = true;
        awt_input_handler *handler = (awt_input_handler*)cl_awt_input_handler;

        if (type&GB_CB_DELETE) { // field was deleted from db -> relink this item
            handler->relink();
        }
        else if (type&GB_CB_CHANGED) {
            handler->db_changed();  // database entry was changed
        }
        in_field_changed_callback = false;
    }
}
//  ------------------------------------------------------------------------------
//      static void awar_changed_cb(AW_root *awr, AW_CL cl_awt_input_handler)
//  ------------------------------------------------------------------------------
static void awar_changed_cb(AW_root *awr, AW_CL cl_awt_input_handler) {
    if (!in_awar_changed_callback) { // avoid deadlock
        in_awar_changed_callback   = true;
        awt_input_handler *handler = (awt_input_handler*)cl_awt_input_handler;
        awt_assert(handler);
        if (handler) handler->awar_changed();
        in_awar_changed_callback   = false;
    }
}


//  ----------------------------------------------------
//      GB_ERROR awt_input_handler::add_callbacks()
//  ----------------------------------------------------
GB_ERROR awt_input_handler::add_db_callbacks() {
    GB_ERROR error = 0;
    if (gb_item) {
        error = GB_add_callback(gb_item, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), item_changed_cb, (int*)this);
        if (gbd) error = GB_add_callback(gbd, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), field_changed_cb, (int*)this);
    }
    return error;
}
//  ---------------------------------------------------
//      void awt_input_handler::remove_callbacks()
//  ---------------------------------------------------
GB_ERROR awt_input_handler::remove_db_callbacks() {
    GB_ERROR error = 0;
    if (gb_item) {
        error = GB_remove_callback(gb_item, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), item_changed_cb, (int*)this);
        if (gbd) error = GB_remove_callback(gbd, (GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE), field_changed_cb, (int*)this);
    }
    return error;
}

//  ---------------------------------------------------------
//      GB_ERROR awt_input_handler::add_awar_callbacks()
//  ---------------------------------------------------------
GB_ERROR awt_input_handler::add_awar_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->add_callback(awar_changed_cb, AW_CL(this));
    return 0;
}
//  ------------------------------------------------------------
//      GB_ERROR awt_input_handler::remove_awar_callbacks()
//  ------------------------------------------------------------
GB_ERROR awt_input_handler::remove_awar_callbacks() {
    AW_awar *var = awar();
    awt_assert(var);
    if (var) var->remove_callback((AW_RCB)awar_changed_cb, AW_CL(this), AW_CL(0));
    return 0;
}

int awt_input_handler::awar_counter = 0;
//  ---------------------------------------------------------------------------------------------------------------------------------------------
//      awt_input_handler::awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_, const string& label_)
//  ---------------------------------------------------------------------------------------------------------------------------------------------
awt_input_handler::awt_input_handler(awt_input_mask_global *global_, const string& child_path_, GB_TYPES type_, const string& label_)
    : global(global_)
    , gb_item(0)
    , gbd(0)
    , label(label_)
    , child_path(child_path_)
    , id(awar_counter++)
    , db_type(type_)
    , in_destructor(false)
{
}

//  -----------------------------------------------------------------
//      GB_ERROR awt_input_handler::link_to(GBDATA *gb_new_item)
//  -----------------------------------------------------------------
GB_ERROR awt_input_handler::link_to(GBDATA *gb_new_item) {
    GB_ERROR       error = 0;
    GB_transaction dummy(mask_global()->get_gb_main());

    if (gb_item) {
        remove_db_callbacks(); // ignore result (if handled db-entry was deleted, it returns an error)
        error   = remove_awar_callbacks();
        gb_item = 0;
        gbd     = 0;
    }
    else {
        aw_assert(!gbd);
    }

    if (!gb_new_item && !in_destructor) { // crashes if we are in ~awt_input_handler
        db_changed();
    }

    if (!error && gb_new_item) {
        gb_item = gb_new_item;
        gbd = GB_search(gb_item, child_path.c_str(), GB_FIND);

        db_changed();           // synchronize AWAR with DB-entry

        error             = add_db_callbacks();
        if (!error) error = add_awar_callbacks();
    }

    return error;
}

//  ------------------------------------------------
//      void awt_string_handler::awar_changed()
//  ------------------------------------------------
void awt_string_handler::awar_changed() {
    GBDATA         *gbd       = data();
    GBDATA         *gb_main   = mask_global()->get_gb_main();
    GB_transaction  dummy(gb_main);
    bool            relink_me = false;

    if (!gbd) {
        const char *child   = get_child_path().c_str();
        const char *keypath = mask_global()->get_selector()->getKeyPath();

        if (item()) {
            gbd = GB_search(item(), child, GB_FIND);

            if (!gbd) {
                GB_TYPES found_typ = awt_get_type_of_changekey(gb_main, child, keypath);
                if (found_typ != GB_NONE) set_type(found_typ); // fix type if different

                gbd = GB_search(item(), child, type()); // here new items are created
                if (found_typ == GB_NONE) awt_add_new_changekey_to_keypath(gb_main, child, type(), keypath);
                relink_me = true; //  @@@ only if child was created!!
            }
        }
        else {
            awar()->write_string(GBS_global_string("<no %s selected>", awt_itemtype_names[mask_global()->get_itemtype()]));
        }
    }

    if (gbd) {
        char     *content   = awar()->read_string();
        GB_TYPES  found_typ = GB_read_type(gbd);
        if (found_typ != type()) set_type(found_typ); // fix type if different

        GB_write_as_string(gbd, awar2db(content).c_str());
        free(content);
    }
    if (relink_me) relink();
}
//  ----------------------------------------------
//      void awt_string_handler::db_changed()
//  ----------------------------------------------
void awt_string_handler::db_changed() {
    GBDATA *gbd = data();
    if (gbd) { // gbd may be zero, if field does not exist
        GB_transaction  dummy(mask_global()->get_gb_main());
        char           *content = GB_read_as_string(gbd);
        awar()->write_string(db2awar(content).c_str());
        free(content);
    }
    else {
        awar()->write_string(default_value.c_str());
    }
}

// ###############################
// ###############################
// ###                         ###
// ##          Widgets          ##
// ###                         ###
// ###############################
// ###############################

//  -----------------------------------------------------------
//      void awt_input_field::build_widget(AW_window *aws)
//  -----------------------------------------------------------
void awt_input_field::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_input_field(awar_name().c_str(), field_width);
}
//  ---------------------------------------------------------
//      void awt_check_box::build_widget(AW_window *aws)
//  ---------------------------------------------------------
void awt_check_box::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle(awar_name().c_str());
}
//  ------------------------------------------------------------
//      void awt_radio_button::build_widget(AW_window *aws)
//  ------------------------------------------------------------
void awt_radio_button::build_widget(AW_window *aws) {
    const string& lab = get_label();
    if (lab.length()) aws->label(lab.c_str());

    aws->create_toggle_field(awar_name().c_str(), vertical ? 0 : 1);

    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    int                            pos = 0;

    for (; b != buttons.end() && v != values.end(); ++b, ++v, ++pos) {
        void (AW_window::*ins_togg)(AW_label, const char*, const char*);
        ins_togg = (pos == default_position) ? &AW_window::insert_default_toggle : &AW_window::insert_toggle;

        (aws->*ins_togg)(b->c_str(), mask_global()->hotkey(*b), b->c_str());
    }

    aw_assert(b == buttons.end() && v == values.end());

    aws->update_toggle_field();
}

// ########################################################
// ########################################################
// ###                                                  ###
// ##          Special AWAR <-> DB translations          ##
// ###                                                  ###
// ########################################################
// ########################################################

//  ------------------------------------------------------------------
//      string awt_check_box::awar2db(const string& awar_content)
//  ------------------------------------------------------------------
string awt_check_box::awar2db(const string& awar_content) const {
    GB_TYPES typ = type();

    if (awar_content == "yes") {
        if (typ == GB_STRING) return "yes";
        return "1";
    }
    else {
        if (typ == GB_STRING) return "no";
        return "0";
    }
}
//  ----------------------------------------------------------------
//      string awt_check_box::db2awar(const string& db_content)
//  ----------------------------------------------------------------
string awt_check_box::db2awar(const string& db_content) const {
    if (db_content == "yes" || db_content == "1") return "yes";
    if (db_content == "no" || db_content == "0") return "no";
    return atoi(db_content.c_str()) ? "yes" : "no";
}

//  ---------------------------------------------------------------------
//      string awt_radio_button::awar2db(const string& awar_content)
//  ---------------------------------------------------------------------
string awt_radio_button::awar2db(const string& awar_content) const {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*b == awar_content) {
            return *v;
        }
    }

    return string("Unknown awar_content '")+awar_content+"'";
}
//  -------------------------------------------------------------------
//      string awt_radio_button::db2awar(const string& db_content)
//  -------------------------------------------------------------------
string awt_radio_button::db2awar(const string& db_content) const {
    vector<string>::const_iterator b   = buttons.begin();
    vector<string>::const_iterator v   = values.begin();
    for (; b != buttons.end() && v != values.end(); ++b, ++v) {
        if (*v == db_content) {
            return *b;
        }
    }
    return buttons[default_position];
}

//  ----------------------------------------------------------------------------------
//      string awt_numeric_input_field::awar2db(const string& awar_content) const
//  ----------------------------------------------------------------------------------
string awt_numeric_input_field::awar2db(const string& awar_content) const {
    long val = strtol(awar_content.c_str(), 0, 10);

    // correct numeric value :
    if (val<min) val = min;
    if (val>max) val = max;

    return GBS_global_string("%li", val);
}

// ########################################################
// ########################################################
// ###                                                  ###
// ##          Routines to parse user-mask file          ##
// ###                                                  ###
// ########################################################
// ########################################################

//  -------------------------------------------------------------------------
//      static GB_ERROR readLine(FILE *in, string& line, size_t& lineNo)
//  -------------------------------------------------------------------------
static GB_ERROR readLine(FILE *in, string& line, size_t& lineNo) {
	const int  BUFSIZE = 8000;
	char       buffer[BUFSIZE];
	char      *res     = fgets(&buffer[0], BUFSIZE-1, in);
    GB_ERROR   error   = 0;

    if (int err = ferror(in)) {
        error = strerror(err);
    }
    else if (res == 0) {
        error = "Unexpected end of file (@MASK_BEGIN or @MASK_END missing?)";
    }
    else {
        aw_assert(res == buffer);
        res += strlen(buffer);
        if (res>buffer) {
            size_t last = res-buffer;
            if (buffer[last-1] == '\n') {
                buffer[last-1] = 0;
            }
        }
        line = buffer;
        lineNo++; // increment lineNo

        size_t last = line.find_last_not_of(" \t");
        if (last != string::npos && line[last] == '\\') {
            string next;
            error = readLine(in, next, lineNo);
            line  = line.substr(0, last)+' '+next;
        }
    }

    if (error) line = "";

    return error;
}

//  -----------------------------------------------------------------------
//      inline size_t next_non_white(const string& line, size_t start)
//  -----------------------------------------------------------------------
inline size_t next_non_white(const string& line, size_t start) {
    if (start == string::npos) return string::npos;
    return line.find_first_not_of(" \t", start);
}

static bool was_last_parameter = false;

//  --------------------------------------------------------------------------------------------
//      inline size_t eat_para_separator(const string& line, size_t start, GB_ERROR& error)
//  --------------------------------------------------------------------------------------------
inline size_t eat_para_separator(const string& line, size_t start, GB_ERROR& error) {
    size_t para_sep = next_non_white(line, start);

    if (para_sep == string::npos) {
        error = "',' or ')' expected after parameter";
    }
    else {
        switch (line[para_sep]) {
            case ')' :
                was_last_parameter = true;
                break;

            case ',' :
                break;

            default :
                error = "',' or ')' expected after parameter";
                break;
        }
        if (!error) para_sep++;
    }

    return para_sep;
}

//  ---------------------------------------------------------------------------------
//      static void check_last_parameter(GB_ERROR& error, const string& command)
//  ---------------------------------------------------------------------------------
static void check_last_parameter(GB_ERROR& error, const string& command) {
    if (!error && !was_last_parameter) {
        error = GBS_global_string("Superfluous arguments to '%s'", command.c_str());
    }
}

//  ---------------------------------------------------------------------------------------------------------------------
//      static void check_no_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& command)
//  ---------------------------------------------------------------------------------------------------------------------
static void check_no_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& command) {
    size_t start = next_non_white(line, scan_pos);
    scan_pos     = start;

    if (start == string::npos) {
        error = "')' expected";
    }
    else if (line[start] != ')') {
        was_last_parameter = false;
        check_last_parameter(error, command);
    }
    else { // ok
        scan_pos++;
    }
}

//  --------------------------------------------------
//      inline GB_ERROR decode_escapes(string& s)
//  --------------------------------------------------
inline GB_ERROR decode_escapes(string& s) {
    string::iterator f = s.begin();
    string::iterator t = s.begin();

    for (; f != s.end(); ++f, ++t) {
        if (*f == '\\') {
            ++f;
            if (f == s.end()) return GBS_global_string("Trailing \\ in '%s'", s.c_str());
        }
        *t = *f;
    }

    s.erase(t, f);

    return 0;
}

//  ---------------------------------------------------------------------------------------------------
//      static string scan_string_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------------
static string scan_string_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, bool allow_escaped = false) {
    string result;
    size_t open_quote = next_non_white(line, scan_pos);
    scan_pos          = open_quote;

    if (open_quote == string::npos || line[open_quote] != '\"') {
        error = "string parameter expected";
    }
    else {
        size_t close_quote = string::npos;

        if (allow_escaped) {
            size_t start = open_quote+1;

            while (1) {
                close_quote = line.find_first_of("\\\"", start);

                if (close_quote == string::npos) break; // error
                if (line[close_quote] == '\"') break; // found closing quote

                if (line[close_quote] == '\\') { // escape next char
                    close_quote++;
                }
                start = close_quote+1;
            }
        }
        else {
            close_quote = line.find('\"', open_quote+1);
        }

        if (close_quote == string::npos) {
            error = "string parameter missing closing '\"'";
        }
        else {
            result = line.substr(open_quote+1, close_quote-open_quote-1);
            if (allow_escaped) {
                assert(!error);
                error = decode_escapes(result);
            }
            if (!error) scan_pos = eat_para_separator(line, close_quote+1, error);
        }
    }

    return result;
}

//  ------------------------------------------------------------
//      string list_keywords(const char **allowed_keywords)
//  ------------------------------------------------------------
string list_keywords(const char **allowed_keywords) {
    string result;
    for (int i = 0; allowed_keywords[i]; ++i) {
        if (i) {
            if (allowed_keywords[i+1]) result += ", ";
            else result                       += " or ";
        }
        result += allowed_keywords[i];
    }
    return result;
}

//  -------------------------------------------------------------------
//      inline int isKeyword(const char *current, const char *keyword)
//  -------------------------------------------------------------------
inline int isKeyword(const char *current, const char *keyword) {
    int pos = 0;
    for (; keyword[pos]; ++pos) {
        if (!current[pos] || tolower(current[pos]) != tolower(keyword[pos])) {
            return 0;
        }
    }
    return pos;
}
//  --------------------------------------------------------------------------------------------------------------------------------
//      static int scan_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const char **allowed_keywords)
//  --------------------------------------------------------------------------------------------------------------------------------
// return index of keyword (or -1)
// allowed_keywords has to be 0-terminated
static int scan_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const char **allowed_keywords) {
    int    result = -1;
    size_t start  = next_non_white(line, scan_pos);
    scan_pos      = start;

    aw_assert(!error);

    if (start == string::npos) {
EXPECTED :
        string keywords = list_keywords(allowed_keywords);
        error           = GBS_global_string("%s expected", keywords.c_str());
    }
    else {
        const char *current = line.c_str()+start;

        int i = 0;
        for (; allowed_keywords[i]; ++i) {
            int found_keyword_size = isKeyword(current, allowed_keywords[i]);
            if (found_keyword_size) {
                result    = i;
                scan_pos += found_keyword_size;
                break;
            }
        }
        if (!allowed_keywords[i]) goto EXPECTED;
        aw_assert(!error);
        scan_pos = eat_para_separator(line, scan_pos, error);
    }
    return result;
}

//  -----------------------------------------------------------------------------------------------------------
//      static void scan_string_or_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error,
//  -----------------------------------------------------------------------------------------------------------
static void scan_string_or_keyword_parameter(const string& line, size_t& scan_pos, GB_ERROR& error,
                                             string& string_para, int& keyword_index, // result parameters
                                             const char **allowed_keywords) {
    // if keyword_index != -1 -> keyword found
    //                  == -1 -> string_para contains string-parameter

    aw_assert(!error);

    string_para = scan_string_parameter(line, scan_pos, error);
    if (!error) {
        keyword_index = -1;
    }
    else {                      // no string
        error         = 0;      // ignore error - test for keywords
        keyword_index = scan_keyword_parameter(line, scan_pos, error, allowed_keywords);
        if (keyword_index == -1) { // no keyword
            error = GBS_global_string("string parameter or %s", error);
        }
    }
}

//  -----------------------------------------------------------------------------------------------
//      static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  -----------------------------------------------------------------------------------------------
static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);
    bool   neg    = false;

    aw_assert(!error);

    while (start != string::npos) {
        char c = line[start];
        if (c != '+' && c != '-') break;

        start             = next_non_white(line, start+1);
        if (c == '-') neg = !neg;
        continue;
    }

    if (start == string::npos || !isdigit(line[start]) ) {
        scan_pos = start;
        error    = "digits (or+-) expected";
    }
    else {
        size_t end = line.find_first_not_of("0123456789", start);
        scan_pos   = eat_para_separator(line, end, error);
        if (!error) {
            aw_assert(end != string::npos);
            result = atoi(line.substr(start, end-start+1).c_str());
        }
    }

    return neg ? -result : result;
}
//  -------------------------------------------------------------------------------------------------------------------
//      static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long min, long max)
//  -------------------------------------------------------------------------------------------------------------------
// with range-check
static long scan_long_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long min, long max) {
    aw_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);
    if (!error) {
        if (result<min || result>max) {
            scan_pos = old_scan_pos;
            error    = GBS_global_string("value %li is outside allowed range (%li-%li)", result, min, max);
        }
    }
    return result;
}

//  ------------------------------------------------------------------------------------------------------------------
//      static long scan_optional_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long if_empty)
//  ------------------------------------------------------------------------------------------------------------------
static long scan_optional_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, long if_empty) {
    aw_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);
    if (error) {
        error    = 0;           // ignore and test for empty parameter
        scan_pos = old_scan_pos;
        scan_pos = eat_para_separator(line, scan_pos, error);

        if (error) {
            error    = "expected number or empty parameter";
            scan_pos = old_scan_pos;
        }
        else {
            result = if_empty;
        }
    }
    return result;
}


//  ---------------------------------------------------------------------------------------------------------------------------
//      static int scan_flag_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& allowed_flags)
//  ---------------------------------------------------------------------------------------------------------------------------
// return 0..n-1 ( = position in 'allowed_flags')
// or error is set
static int scan_flag_parameter(const string& line, size_t& scan_pos, GB_ERROR& error, const string& allowed_flags) {
    aw_assert(!error);
    int    result = 0;
    size_t start  = next_non_white(line, scan_pos);
    scan_pos      = start;

    if (start == string::npos) {
        error    = GBS_global_string("One of '%s' expected", allowed_flags.c_str());
    }
    else {
        char   found       = line[start];
        char   upper_found = toupper(found);
        size_t pos         = allowed_flags.find(upper_found);

        if (pos != string::npos) {
            result   = pos;
            scan_pos = eat_para_separator(line, start+1, error);
        }
        else {
            error = GBS_global_string("One of '%s' expected (found '%c')", allowed_flags.c_str(), found);
        }
    }
    return result;
}
//  -----------------------------------------------------------------------------------------------
//      static bool scan_bool_parameter(const string& line, size_t& scan_pos, GB_ERROR& error)
//  -----------------------------------------------------------------------------------------------
static bool scan_bool_parameter(const string& line, size_t& scan_pos, GB_ERROR& error) {
    aw_assert(!error);
    size_t old_scan_pos = scan_pos;
    long   result       = scan_long_parameter(line, scan_pos, error);

    if (!error && (result != 0) && (result != 1)) {
        scan_pos = old_scan_pos;
        error    = "'0' or '1' expected";
    }
    return result != 0;
}

//  ------------------------------------
//      inline char *inputMaskDir()
//  ------------------------------------
inline char *inputMaskDir() {
    return AWT_unfold_path("lib/inputMasks", "ARBHOME");
}

//  -----------------------------------------------------------------
//      inline string inputMaskFullname(const string& mask_name)
//  -----------------------------------------------------------------
inline string inputMaskFullname(const string& mask_name) {
    char   *dir      = inputMaskDir();
    string  fullname = string(dir)+'/'+mask_name;
    free(dir);
    return fullname;
}

#define ARB_INPUT_MASK_ID "ARB-Input-Mask"

//  -----------------------------------------------------------------------------------------------------------------
//      static awt_input_mask_descriptor *quick_scan_input_mask(const string& mask_name, const string& filename)
//  -----------------------------------------------------------------------------------------------------------------
static awt_input_mask_descriptor *quick_scan_input_mask(const string& mask_name, const string& filename) {
    FILE     *in     = fopen(filename.c_str(), "rt");
    size_t    lineNo = 0;
    GB_ERROR  error  = 0;

    if (in) {
        string   line;
        error = readLine(in, line, lineNo);

        if (!error && line == ARB_INPUT_MASK_ID) {
            bool   done = false;
            string title;
            string itemtype;

            while (!feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line[0] == '#') continue; // ignore comments
                if (line == "@MASK_BEGIN") { done = true; break; }

                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos)  {
                    continue; // ignore all other lines
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE") itemtype = value;
                    else if (parameter == "TITLE") title  = value;
                }
            }

            if (!error && done) {
                if (itemtype == "") {
                    error = "No itemtype defined";
                }
                else {
                    if (title == "") title = mask_name;
                    return new awt_input_mask_descriptor(title.c_str(), mask_name.c_str(), itemtype.c_str());
                }
            }
        }
    }

    if (error) {
        aw_message(GBS_global_string("%s (while scanning user-mask '%s')", error, filename.c_str()));
    }

#if defined(DEBUG)
    printf("Skipping '%s' (not a input mask)\n", filename.c_str());
#endif // DEBUG
    return 0;
}

//  ----------------------------------------------------------------------------
//      static void AWT_edit_input_mask(AW_window *aww, AW_CL cl_mask_name)
//  ----------------------------------------------------------------------------
static void AWT_edit_input_mask(AW_window *aww, AW_CL cl_mask_name) {
    const string *mask_name = (const string *)cl_mask_name;
    string        fullmask  = inputMaskFullname(*mask_name);
    awt_edit(aww->get_root(), fullmask.c_str());
}

//  ---------------------------------
//      input mask container :
//  ---------------------------------
typedef SmartPtr<awt_input_mask>        awt_input_mask_ptr;
typedef map<string, awt_input_mask_ptr> InputMaskList; // contains all active masks
static InputMaskList                    input_mask_list;

//  ---------------------------------------------------------------------------------
//      static void awt_input_mask_awar_changed_cb(AW_root *root, AW_CL cl_mask)
//  ---------------------------------------------------------------------------------
static void awt_input_mask_awar_changed_cb(AW_root *root, AW_CL cl_mask) {
    awt_input_mask *mask = (awt_input_mask*)(cl_mask);
    mask->relink();
}
//  -------------------------------------------------------------------
//      static void link_mask_to_database(awt_input_mask_ptr mask)
//  -------------------------------------------------------------------
static void link_mask_to_database(awt_input_mask_ptr mask) {
    awt_input_mask_global         *global = mask->mask_global();
    const awt_item_type_selector  *sel    = global->get_selector();
    AW_root                       *root   = global->get_root();

    sel->add_awar_callbacks(root, awt_input_mask_awar_changed_cb, (AW_CL)(&*mask));
    awt_input_mask_awar_changed_cb(root, (AW_CL)(&*mask));
}

//  -----------------------------------------------------------------------------------------------------------------------------------
//      static void awt_open_input_mask(AW_window *aww, AW_CL cl_mask_name, AW_CL cl_mask_to_open, bool reload, bool hide_current)
//  -----------------------------------------------------------------------------------------------------------------------------------
static void awt_open_input_mask(AW_window *aww, AW_CL cl_mask_name, AW_CL cl_mask_to_open, bool reload, bool hide_current) {
    const string            *mask_name    = (const string *)cl_mask_name;
    const string            *mask_to_open = (const string *)cl_mask_to_open;
    InputMaskList::iterator  mask_iter    = input_mask_list.find(*mask_name);

    if (mask_iter != input_mask_list.end()) {
        awt_input_mask_ptr     mask   = mask_iter->second;
        awt_input_mask_global *global = mask->mask_global();

        if (reload) mask->set_reload_on_reinit(true);
        if (hide_current) mask->hide();
        GB_ERROR error = AWT_initialize_input_mask(global->get_root(), global->get_gb_main(), global->get_selector(), mask_to_open->c_str());
        if (error && hide_current) mask->show();
    }
}

static void AWT_reload_input_mask(AW_window *aww, AW_CL cl_mask_name) {
    awt_open_input_mask(aww, cl_mask_name, cl_mask_name, true, true);
}
static void AWT_open_input_mask(AW_window *aww, AW_CL cl_mask_name, AW_CL cl_mask_to_open) {
    awt_open_input_mask(aww, cl_mask_name, cl_mask_to_open, false, false);
}
static void AWT_change_input_mask(AW_window *aww, AW_CL cl_mask_name, AW_CL cl_mask_to_open) {
    awt_open_input_mask(aww, cl_mask_name, cl_mask_to_open, false, true);
}



static void AWT_input_mask_browse_url(AW_window *aww, AW_CL cl_url_srt, AW_CL cl_mask_ptr) {
    AW_root                      *root     = aww->get_root();
    const string                 *url_srt  = (const string *)cl_url_srt;
    const awt_input_mask         *mask     = (const awt_input_mask *)cl_mask_ptr;
    const awt_input_mask_global  *global   = mask->mask_global();
    const awt_item_type_selector *selector = global->get_selector();
    GBDATA                       *gbd      = selector->current(root);

    if (!gbd) {
        aw_message(GBS_global_string("You have to select a %s first", awt_itemtype_names[selector->get_item_type()]));
    }
    else {
        char     *name  = root->awar(selector->get_self_awar())->read_string();
        GB_ERROR  error = awt_open_ACISRT_URL_by_gbd(root, global->get_gb_main(), gbd, name, url_srt->c_str());
        if (error) aw_message(error);
        free(name);
    }
}



// ##########################################
// ##########################################
// ###                                    ###
// ##          User Mask Commands          ##
// ###                                    ###
// ##########################################
// ##########################################

enum MaskCommand  {
    CMD_TEXTFIELD,
    CMD_NUMFIELD,
    CMD_CHECKBOX,
    CMD_RADIO,
    CMD_OPENMASK,
    CMD_CHANGEMASK,
    CMD_TEXT,
    CMD_SELF,
    CMD_WWW,
    CMD_NEW_LINE,
    CMD_NEW_SECTION,

    MASK_COMMANDS,
    CMD_UNKNOWN = MASK_COMMANDS
};

struct MaskCommandDefinition {
    const char  *cmd_name;
    MaskCommand  cmd;
};

static struct MaskCommandDefinition mask_command[MASK_COMMANDS+1] =
{
 { "TEXTFIELD", CMD_TEXTFIELD },
 { "NUMFIELD", CMD_NUMFIELD },
 { "CHECKBOX", CMD_CHECKBOX },
 { "RADIO", CMD_RADIO },
 { "OPENMASK", CMD_OPENMASK },
 { "CHANGEMASK", CMD_CHANGEMASK },
 { "TEXT", CMD_TEXT },
 { "SELF", CMD_SELF },
 { "NEW_LINE", CMD_NEW_LINE },
 { "NEW_SECTION", CMD_NEW_SECTION },
 { "WWW", CMD_WWW },

 { 0, CMD_UNKNOWN}
};

//  ---------------------------------------------------------------
//      inline MaskCommand findCommand(const string& cmd_name)
//  ---------------------------------------------------------------
inline MaskCommand findCommand(const string& cmd_name) {
    int mc = 0;

    for (; mask_command[mc].cmd != CMD_UNKNOWN; ++mc) {
        if (mask_command[mc].cmd_name == cmd_name) {
            return mask_command[mc].cmd;
        }
    }
    return CMD_UNKNOWN;
}

//  -----------------------------------------------------------------------------------------------------------
//      static void parse_CMD_RADIO(string& line, size_t& scan_pos, GB_ERROR& error, const string& command,
//  -----------------------------------------------------------------------------------------------------------
static void parse_CMD_RADIO(string& line, size_t& scan_pos, GB_ERROR& error, const string& command,
                            awt_input_handler_ptr& handler1, awt_input_handler_ptr& handler2, awt_input_mask_global *global) {
    string         label, data_path;
    int            default_position, orientation;
    vector<string> buttons;
    vector<string> values;
    bool           allow_edit    = false;
    int            width;
    int            edit_position = -1;

    label                        = scan_string_parameter(line, scan_pos, error);
    if (!error) data_path        = scan_string_parameter(line, scan_pos, error);
    if (!error) default_position = scan_long_parameter(line, scan_pos, error);
    if (!error) {
        orientation = scan_flag_parameter(line, scan_pos, error, "HVXY");
        orientation = orientation&1;
    }
    while (!error && !was_last_parameter) {
        string but = scan_string_parameter(line, scan_pos, error);
        string val = "";
        if (!error) {
            int keyword_index;
            const char *allowed_keywords[] = { "ALLOW_EDIT", 0};
            scan_string_or_keyword_parameter(line, scan_pos, error, val, keyword_index, allowed_keywords);

            if (!error) {
                if (keyword_index != -1) { // found keyword
                    if (allow_edit) error = "ALLOW_EDIT is allowed only once for each RADIO";

                    if (!error) width = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                    if (!error) val   = scan_string_parameter(line, scan_pos, error);

                    if (!error) {
                        allow_edit    = true;
                        edit_position = buttons.size()+1; // positions are 1..n
                        buttons.push_back(but);
                        values.push_back(val);
                    }
                }
                else { // found string
                    buttons.push_back(but);
                    values.push_back(val);
                }
            }
        }
    }
    check_last_parameter(error, command);

    if (!error && (buttons.size()<2)) error = "You have to define at least 2 buttons.";

    if (!error && allow_edit && edit_position != default_position) {
        error = GBS_global_string("Invalid default %i (must be index of ALLOW_EDIT: %i )", default_position, edit_position);
    }
    if (!error && (default_position<1 || size_t(default_position)>buttons.size())) {
        error = GBS_global_string("Invalid default %i (valid: 1..%i)", default_position, buttons.size());
    }

    if (!error) {
        if (allow_edit) {
            // create radio-button + textfield
            handler1 = new awt_radio_button(global, data_path, label, default_position-1, orientation, buttons, values);
            handler2 = new awt_input_field(global, data_path, "", width, "", GB_STRING);
        }
        else {
            handler1 = new awt_radio_button(global, data_path, label, default_position-1, orientation, buttons, values);
        }
    }
}

//  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//      static awt_input_mask_ptr awt_create_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const string& mask_name, GB_ERROR& error)
//  ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
static awt_input_mask_ptr awt_create_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const string& mask_name, GB_ERROR& error) {
    size_t             lineNo = 0;
    awt_input_mask_ptr mask;

    error = 0;

    FILE *in = 0;
    {
        string  fullfile = inputMaskFullname(mask_name);
        in               = fopen(fullfile.c_str(), "rt");
        if (!in) error   = GBS_global_string("Can't open '%s'", fullfile.c_str());
    }

    if (!error) {
        bool   mask_began = false;
        bool   mask_ended = false;

        // data to be scanned :
        string        title;
        string        itemtypename;
        int           x_spacing   = 5;
        int           y_spacing   = 3;
        bool          edit_reload = false;
        awt_item_type itemtype;

        string line;
        size_t err_pos = 0;     // 0 = unknown; string::npos = at end of line; else position+1;
        error          = readLine(in, line, lineNo);

        if (!error && line != ARB_INPUT_MASK_ID) {
            error = "'" ARB_INPUT_MASK_ID "' expected";
        }

        while (!error && !mask_began && !feof(in)) {
            error = readLine(in, line, lineNo);
            if (error) break;

            if (line[0] == '#') continue; // ignore comments

            if (line == "@MASK_BEGIN") mask_began = true;
            else  {
                size_t at = line.find('@');
                size_t eq = line.find('=', at);

                if (at == string::npos || eq == string::npos) {
                    continue;
                }
                else {
                    string parameter = line.substr(at+1, eq-at-1);
                    string value     = line.substr(eq+1);

                    if (parameter == "ITEMTYPE") itemtypename    = value;
                    else if (parameter == "TITLE") title         = value;
                    else if (parameter == "X_SPACING") x_spacing = atoi(value.c_str());
                    else if (parameter == "Y_SPACING") y_spacing = atoi(value.c_str());
                    else if (parameter == "EDIT") edit_reload    = atoi(value.c_str()) != 0;
                    else {
                        error = GBS_global_string("Unknown parameter '%s'", parameter.c_str());
                    }
                }
            }
        }

        if (!error && !mask_began) error = "@MASK_BEGIN expected";

        // check data :
        if (!error) {
            if (title == "") title = string("Untitled (")+mask_name+")";
            itemtype = AWT_getItemType(itemtypename);

            if (itemtype == AWT_IT_UNKNOWN)         error = GBS_global_string("Unknown @ITEMTYPE '%s'", itemtypename.c_str());
            if (itemtype != sel->get_item_type())   error = GBS_global_string("Mask is designed for @ITEMTYPE '%s' (current @ITEMTYPE '%s')",
                                                                              itemtypename.c_str(), awt_itemtype_names[sel->get_item_type()]);
        }

        // create mask
        if (!error) {
            static int awar_root_counter = 0;
            string     awar_root         = GBS_global_string("/tmp/input_mask_%i", awar_root_counter++);
            mask                         = new awt_input_mask(root, gb_main, mask_name, awar_root, itemtype, sel);
        }

        // create window
        if (!error) {
            aw_assert(!mask.Null());
            AW_window_simple*& aws = mask->get_window();
            aws = new AW_window_simple;
            aws->init(root, "INPUT_MASK", title.c_str(), 200, 100);
            aws->load_xfig(0, AW_TRUE);

            aws->auto_space(x_spacing, y_spacing);
            aws->at_newline();

            aws->callback((AW_CB0)AW_POPDOWN); aws->create_button("CLOSE", "CLOSE", "C");
            aws->callback( AW_POPUP_HELP,(AW_CL)"input_mask.hlp"); aws->create_button("HELP","HELP","H");

            if (edit_reload) {
                aws->callback( AWT_edit_input_mask, (AW_CL)&mask->get_maskname()); aws->create_button("EDIT","EDIT","E");
                aws->callback( AWT_reload_input_mask, (AW_CL)&mask->get_maskname()); aws->create_button("RELOAD","RELOAD","R");
            }

            aws->at_newline();

            vector<int> horizontal_lines; // y-positions of horizontal lines

            // read mask itself :
            while (!error && !mask_ended && !feof(in)) {
                error = readLine(in, line, lineNo);
                if (error) break;

                if (line[0] == '#') continue;

                if (line == "@MASK_END") {
                    mask_ended = true;
                }
                else  {
                PARSE_REST_OF_LINE:
                    was_last_parameter = false;
                    size_t start       = next_non_white(line, 0);
                    if (start != string::npos) { // line contains sth
                        size_t after_command = line.find_first_of(string(" \t("), start);
                        if (after_command == string::npos) {
                            string command = line.substr(start);
                            error          = GBS_global_string("arguments missing after '%s'", command.c_str());
                        }
                        else {
                            string command    = line.substr(start, after_command-start);
                            size_t paren_open = line.find('(', after_command);
                            if (paren_open == string::npos) {
                                error = GBS_global_string("'(' expected after '%s'", command.c_str());
                            }
                            else {
                                size_t                scan_pos = paren_open+1;
                                awt_input_handler_ptr handler;
                                awt_input_handler_ptr radio_edit_handler;
                                MaskCommand           cmd      = findCommand(command);

                                //  --------------------------------------
                                //      code for different commands :
                                //  --------------------------------------

                                if (cmd == CMD_TEXTFIELD) {
                                    string label, data_path;
                                    long   width;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) width     = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_input_field(mask->mask_global(), data_path, label, width, "", GB_STRING);
                                }
                                else if (cmd == CMD_NUMFIELD) {
                                    string label, data_path;
                                    long    width;

                                    long min = LONG_MIN;
                                    long max = LONG_MAX;

                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) width     = scan_long_parameter(line, scan_pos, error, MIN_TEXTFIELD_SIZE, MAX_TEXTFIELD_SIZE);
                                    if (!was_last_parameter) {
                                        if (!error) min = scan_optional_parameter(line, scan_pos, error, LONG_MIN);
                                        if (!error) max = scan_optional_parameter(line, scan_pos, error, LONG_MAX);
                                    }
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_numeric_input_field(mask->mask_global(), data_path, label, width, 0, min, max);
                                }
                                else if (cmd == CMD_CHECKBOX) {
                                    string label, data_path;
                                    bool   checked        = false;
                                    label                 = scan_string_parameter(line, scan_pos, error);
                                    if (!error) data_path = scan_string_parameter(line, scan_pos, error);
                                    if (!error) checked   = scan_bool_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) handler = new awt_check_box(mask->mask_global(), data_path, label, checked);
                                }
                                else if (cmd == CMD_RADIO) {
                                    parse_CMD_RADIO(line, scan_pos, error, command, handler, radio_edit_handler, mask->mask_global());
                                }
                                else if (cmd == CMD_OPENMASK || cmd == CMD_CHANGEMASK) {
                                    string label, mask_to_start;
                                    label                     = scan_string_parameter(line, scan_pos, error);
                                    if (!error) mask_to_start = scan_string_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        char  *key = GBS_string_2_key(label.c_str());
                                        AW_CB  cb  = cmd == CMD_OPENMASK ? AWT_open_input_mask : AWT_change_input_mask;

                                        aws->callback( cb, (AW_CL)&mask->get_maskname(), (AW_CL)new string(mask_to_start));
                                        aws->button_length(label.length()+2);
                                        aws->create_button(key, label.c_str());

                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_WWW) {
                                    string button_text, url_srt;
                                    button_text         = scan_string_parameter(line, scan_pos, error);
                                    if (!error) url_srt = scan_string_parameter(line, scan_pos, error, true);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        char *key = GBS_string_2_key(button_text.c_str());

                                        aws->callback(AWT_input_mask_browse_url, (AW_CL)new string(url_srt), (AW_CL)&*mask);
                                        aws->button_length(button_text.length()+2);
                                        aws->create_button(key, button_text.c_str());

                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_TEXT) {
                                    string text;
                                    text = scan_string_parameter(line, scan_pos, error);
                                    check_last_parameter(error, command);

                                    if (!error) {
                                        char *key = GBS_string_2_key(text.c_str());
                                        aws->button_length(text.length()+2);
                                        aws->create_button(key, text.c_str());
                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_SELF) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        const awt_item_type_selector *sel              = mask->mask_global()->get_selector();
                                        string                        button_awar_name = sel->get_self_awar();
                                        char                         *key              = GBS_string_2_key(button_awar_name.c_str());

                                        aws->button_length(sel->get_self_awar_content_length());
                                        aws->create_button(key, button_awar_name.c_str());
                                        free(key);
                                    }
                                }
                                else if (cmd == CMD_NEW_LINE) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        int width, height;
                                        aws->get_window_size(width, height);
                                        aws->at_shift(0, 2*SEC_YBORDER+SEC_LINE_WIDTH);
                                    }
                                }
                                else if (cmd == CMD_NEW_SECTION) {
                                    check_no_parameter(line, scan_pos, error, command);
                                    if (!error) {
                                        int width, height;
                                        aws->get_window_size(width, height);
                                        horizontal_lines.push_back(height);
                                        aws->at_shift(0, 2*SEC_YBORDER+SEC_LINE_WIDTH);
                                    }
                                }
                                else if (cmd == CMD_UNKNOWN) {
                                    error = GBS_global_string("Unknown command '%s'", command.c_str());
                                }
                                else {
                                    error = GBS_global_string("No handler for '%s'", command.c_str());
                                    aw_assert(0);
                                }

                                //  --------------------------
                                //      insert handler(s)
                                //  --------------------------

                                if (!handler.Null() && !error) {
                                    if (!radio_edit_handler.Null()) { // special radio handler
                                        const awt_radio_button *radio = dynamic_cast<const awt_radio_button*>(&*handler);
                                        assert(radio);

                                        int x_start, y_start;

                                        aws->get_at_position(&x_start, &y_start);

                                        mask->add_handler(handler);
                                        handler->build_widget(aws);

                                        int x_end, y_end, dummy;
                                        aws->get_at_position(&x_end, &dummy);
                                        aws->at_newline();
                                        aws->get_at_position(&dummy, &y_end);

                                        int height    = y_end-y_start;
                                        int each_line = height/radio->no_of_toggles();
                                        int y_offset  = each_line*(radio->default_toggle());

                                        aws->at(x_end+x_spacing, y_start+y_offset);

                                        mask->add_handler(radio_edit_handler);
                                        radio_edit_handler->build_widget(aws);

                                        radio_edit_handler.SetNull();
                                    }
                                    else {
                                        mask->add_handler(handler);
                                        handler->build_widget(aws);
                                    }
                                }

                                // parse rest of line or abort
                                if (!error) {
                                    line = line.substr(scan_pos);
                                    goto PARSE_REST_OF_LINE;
                                }
                                err_pos = scan_pos;
                                if (err_pos != string::npos) err_pos++; // because 0 means unknown
                            }
                        }
                    }
                    else { // reached the end of the current line
                        aws->at_newline();
                    }
                }
            }

            if (!error) {
                if (!horizontal_lines.empty()) { // draw all horizontal lines
                    int width, height;
                    aws->get_window_size(width, height);
                    for (vector<int>::const_iterator yi = horizontal_lines.begin(); yi != horizontal_lines.end(); ++yi) {
                        int y = (*yi)+SEC_YBORDER;
                        aws->draw_line(SEC_XBORDER, y, width-SEC_XBORDER, y, SEC_LINE_WIDTH, AW_TRUE);
                    }
                }

                // fit the window
                aws->window_fit();
            }

            // link to database
            if (!error) link_mask_to_database(mask);
        }

        if (error) {
            if (err_pos == 0) { // don't knows exact error position
                error = GBS_global_string("%s in line #%u", error, lineNo);
            }
            else if (err_pos == 0) {
                error = GBS_global_string("%s at end of line #%u", error, lineNo);
            }
            else {
                int    wanted         = 35;
                size_t end            = line.length();
                string context;
                context.reserve(wanted);
                bool   last_was_space = false;

                for (size_t ex = err_pos-1; ex<end && wanted>0; ++ex) {
                    char ch            = line[ex];
                    bool this_is_space = ch == ' ';

                    if (!this_is_space || !last_was_space) {
                        context.append(1, ch);
                        --wanted;
                    }
                    last_was_space = this_is_space;
                }

                error = GBS_global_string("%s in line #%u at '%s...'", error, lineNo, context.c_str());
            }
        }

        fclose(in);
    }

    return mask;
}

//  -------------------------------------------------------------------------------------------------------------------------------------
//      GB_ERROR AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* mask_name)
//  -------------------------------------------------------------------------------------------------------------------------------------
GB_ERROR AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* mask_name) {
    InputMaskList::iterator mask_iter = input_mask_list.find(mask_name);
    GB_ERROR                error     = 0;

    // erase mask (so it loads again from scratch)
    if (mask_iter != input_mask_list.end() &&
        mask_iter->second->reload_on_reinit()) // reload wanted ?
    {
        awt_input_mask_ptr old_mask = mask_iter->second;
        input_mask_list.erase(mask_iter);
        mask_iter                   = input_mask_list.end();

        old_mask->hide();
        static list<awt_input_mask_ptr> mask_collector;
        mask_collector.push_back(old_mask);
    }

    if (mask_iter == input_mask_list.end()) { // mask not loaded yet
        awt_input_mask_ptr newMask = awt_create_input_mask(root, gb_main, sel, mask_name, error);
        if (error) {
            error = GBS_global_string("Error creating input-mask from %s (%s)",
                                      mask_name, error);
        }
        else {
            input_mask_list[mask_name] = newMask;
            mask_iter                  = input_mask_list.find(mask_name);
        }
    }

    if (!error) {
        AW_window_simple *aws  = mask_iter->second->get_window();
        aws->show();
    }

    if (error) aw_message(error);
    return error;
}

//  --------------------------------------
//      void awt_input_mask::relink()
//  --------------------------------------
void awt_input_mask::relink() {
    const awt_item_type_selector  *sel     = global.get_selector();
    GBDATA                        *gb_item = sel->current(global.get_root());

    for (awt_input_handler_list::iterator h = handlers.begin(); h != handlers.end(); ++h) {
        (*h)->link_to(gb_item);
    }
}

//  -----------------------------------------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor::awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_)
//  -----------------------------------------------------------------------------------------------------------------------------------
awt_input_mask_descriptor::awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_) {
    title        = strdup(title_);
    maskname     = strdup(maskname_);
    itemtypename = strdup(itemtypename_);
}
//  ----------------------------------------------------------------
//      awt_input_mask_descriptor::~awt_input_mask_descriptor()
//  ----------------------------------------------------------------
awt_input_mask_descriptor::~awt_input_mask_descriptor() {
    free(itemtypename);
    free(maskname);
    free(title);
}
//  -----------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor::awt_input_mask_descriptor(const awt_input_mask_descriptor& other)
//  -----------------------------------------------------------------------------------------------------
awt_input_mask_descriptor::awt_input_mask_descriptor(const awt_input_mask_descriptor& other) {
    title        = strdup(other.title);
    maskname     = strdup(other.maskname);
    itemtypename = strdup(other.itemtypename);
}
//  ------------------------------------------------------------------------------------------------------------------
//      awt_input_mask_descriptor& awt_input_mask_descriptor::operator = (const awt_input_mask_descriptor& other)
//  ------------------------------------------------------------------------------------------------------------------
awt_input_mask_descriptor& awt_input_mask_descriptor::operator = (const awt_input_mask_descriptor& other) {
    if (this != &other) {
        free(itemtypename);
        free(maskname);
        free(title);

        title        = strdup(other.title);
        maskname     = strdup(other.maskname);
        itemtypename = strdup(other.itemtypename);
    }

    return *this;
}

static int scanned_existing_input_masks = false;
static vector<awt_input_mask_descriptor> existing_masks;

//  ------------------------------------------------
//      static void scan_existing_input_masks()
//  ------------------------------------------------
static void scan_existing_input_masks() {
    aw_assert(!scanned_existing_input_masks);

    char *dirname = inputMaskDir();
    DIR  *dirp    = opendir(dirname);

    if (!dirp) {
        fprintf(stderr, "The path '%s' is invalid.\n", dirname);
    }
    else {
        struct dirent *dp;
        for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
            struct stat st;
            string      maskname = dp->d_name;
            string      fullname = inputMaskFullname(maskname);

            if (stat(fullname.c_str(), &st)) continue;
            if (!S_ISREG(st.st_mode)) continue;
            // now we have a regular file

            if (maskname.find(".mask") != string::npos) {
                awt_input_mask_descriptor *descriptor = quick_scan_input_mask(maskname, fullname);
                if (descriptor) { // we found a input mask file
                    existing_masks.push_back(*descriptor);
                    delete descriptor;
                }
            }
        }
    }

    free(dirname);
    scanned_existing_input_masks = true;
}

//  ---------------------------------------------------------------------
//      const awt_input_mask_descriptor *AWT_look_input_mask(int id)
//  ---------------------------------------------------------------------
const awt_input_mask_descriptor *AWT_look_input_mask(int id) {
    if (!scanned_existing_input_masks) scan_existing_input_masks();

    if (id<0 || size_t(id) >= existing_masks.size()) return 0;

    const awt_input_mask_descriptor *descriptor = &existing_masks[id];
    return descriptor;
}

//  -------------------------------------------------------------------
//      awt_item_type AWT_getItemType(const string& itemtype_name)
//  -------------------------------------------------------------------
awt_item_type AWT_getItemType(const string& itemtype_name) {
    awt_item_type type = AWT_IT_UNKNOWN;

    for (int i = AWT_IT_UNKNOWN+1; i<AWT_IT_TYPES; ++i) {
        if (itemtype_name == awt_itemtype_names[i]) {
            type = awt_item_type(i);
            break;
        }
    }

    return type;
}

//  ------------------------------------------------------------------------------------------------------------------------------------------------------------
//      void AWT_create_mask_submenu(AW_window_menu_modes *awm, awt_item_type wanted_item_type, void (*open_window_cb)(AW_window* aww, AW_CL cl_id, AW_CL))
//  ------------------------------------------------------------------------------------------------------------------------------------------------------------
void AWT_create_mask_submenu(AW_window_menu_modes *awm, awt_item_type wanted_item_type, void (*open_window_cb)(AW_window* aww, AW_CL cl_id, AW_CL)) {
    // add a user mask submenu at current position
    bool found = false;

    for (int id = 0; ;++id) {
        const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
        if (!descriptor) break;

        awt_item_type item_type = AWT_getItemType(descriptor->get_itemtypename());

        if (item_type == wanted_item_type) {
            if (!found) awm->insert_sub_menu(0, "User Masks", "");

            found               = true;
            char *macroname2key = GBS_string_2_key(descriptor->get_maskname());
#if defined(DEBUG) && 0
            printf("added user-mask '%s' with id=%i\n", descriptor->get_maskname(), id);
#endif // DEBUG
            awm->insert_menu_topic(macroname2key, descriptor->get_title(), "", "input_mask.hlp", AWM_ALL, open_window_cb, (AW_CL)id, (AW_CL)0);
            free(macroname2key);
        }
        else if (item_type == AWT_IT_UNKNOWN) {
            aw_message(GBS_global_string("Unkown @ITEMTYPE '%s' in '%s'", descriptor->get_itemtypename(), descriptor->get_maskname()));
        }
    }

    if (found) awm->close_sub_menu();
}


