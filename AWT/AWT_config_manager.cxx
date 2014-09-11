//  ==================================================================== //
//                                                                       //
//    File      : AWT_config_manager.cxx                                 //
//    Purpose   :                                                        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "awt_config_manager.hxx"
#include "awt.hxx"
#include <arbdb.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>
#include <arb_strarray.h>

#include <map>
#include <string>
#include "awt_sel_boxes.hxx"
#include <arb_defs.h>

using namespace std;

// --------------------------
//      AWT_configuration

class AWT_configuration : virtual Noncopyable {
private:
    string id;

    AWT_store_config_to_string  store;
    AWT_load_config_from_string load;

    AW_CL client1; // client data for callbacks above
    AW_CL client2;

    AW_default default_file;

public:
    AWT_configuration(AW_default default_file_, const char *id_, AWT_store_config_to_string store_, AWT_load_config_from_string load_, AW_CL cl1, AW_CL cl2)
        : id(id_),
          store(store_),
          load(load_),
          client1(cl1),
          client2(cl2),
          default_file(default_file_)
    {}

    bool operator<(const AWT_configuration& other) const { return id<other.id; }

    string get_awar_name(const string& subname) const { return string("general_configs/")+id+'/'+subname; }
    AW_awar *get_awar(const string& subname, const char *default_value = "") const {
        string   awar_name = get_awar_name(subname);
        return AW_root::SINGLETON->awar_string(awar_name.c_str(), default_value, default_file);
    }

    string get_awar_value(const string& subname, const char *default_value = "") const {
        char   *value  = get_awar(subname, default_value)->read_string();
        string  result = value;
        free(value);
        return result;
    }
    void set_awar_value(const string& subname, const string& new_value) const {
        get_awar(subname)->write_string(new_value.c_str());
    }

    const char *get_id() const { return id.c_str(); }

    char *Store() const { return store(client1, client2); }
    GB_ERROR Restore(const string& s) const {
        GB_ERROR error = 0;

        if (s.empty()) error = "empty/nonexistant config";
        else load(s.c_str(), client1, client2);

        return error;
    }

    GB_ERROR Save(const char* filename, const string& awar_name); // AWAR content -> FILE
    GB_ERROR Load(const char* filename, const string& awar_name); // FILE -> AWAR content
};

#define HEADER    "ARB_CONFIGURATION"
#define HEADERLEN 17

GB_ERROR AWT_configuration::Save(const char* filename, const string& awar_name) {
    awt_assert(strlen(HEADER) == HEADERLEN);

    printf("Saving config to '%s'..\n", filename);

    FILE     *out   = fopen(filename, "wt");
    GB_ERROR  error = 0;
    if (!out) {
        error = GB_export_IO_error("saving", filename);
    }
    else {
        fprintf(out, HEADER ":%s\n", id.c_str());
        string content = get_awar_value(awar_name);
        fputs(content.c_str(), out);
        fclose(out);
    }
    return error;
}

GB_ERROR AWT_configuration::Load(const char* filename, const string& awar_name) {
    GB_ERROR error = 0;

    printf("Loading config from '%s'..\n", filename);

    char *content = GB_read_file(filename);
    if (!content) {
        error = GB_await_error();
    }
    else {
        if (strncmp(content, HEADER ":", HEADERLEN+1) != 0) {
            error = "Unexpected content (" HEADER " missing)";
        }
        else {
            char *id_pos = content+HEADERLEN+1;
            char *nl     = strchr(id_pos, '\n');

            if (!nl) {
                error = "Unexpected content (no ID)";
            }
            else {
                *nl++ = 0;
                if (strcmp(id_pos, id.c_str()) != 0) {
                    error = GBS_global_string("Wrong config (id=%s, expected=%s)", id_pos, id.c_str());
                }
                else {
                    set_awar_value(awar_name, nl);
                }
            }
        }

        if (error) {
            error = GBS_global_string("Error: %s (while reading %s)", error, filename);
        }

        free(content);
    }

    return error;
}

void remove_from_configs(const string& config, string& existing_configs) {
    ConstStrArray cfgs;
    GBT_split_string(cfgs, existing_configs.c_str(), ';');

    ConstStrArray remaining;
    for (int i = 0; cfgs[i]; ++i) {
        if (strcmp(cfgs[i], config.c_str()) != 0) {
            remaining.put(cfgs[i]);
        }
    }

    char *rest       = GBT_join_names(remaining, ';');
    existing_configs = rest;
    free(rest);
}

static string config_prefix = "cfg_";

static void correct_key_name_cb(AW_root*, AW_awar *awar) {
    string  with_prefix = config_prefix+awar->read_char_pntr();
    char   *corrected   = GBS_string_2_key(with_prefix.c_str());
    awar->write_string(corrected+config_prefix.length());
    free(corrected);
}

static void restore_cb(AW_window *, AWT_configuration *config) {
    string   cfgName   = config->get_awar_value("current");
    string   awar_name = config_prefix+cfgName;
    GB_ERROR error     = config->Restore(config->get_awar_value(awar_name));
    aw_message_if(error);
}
static void store_cb(AW_window *, AWT_configuration *config) {
    string existing = config->get_awar_value("existing");
    string cfgName  = config->get_awar_value("current");

    remove_from_configs(cfgName, existing); // remove selected from existing configs

    existing = existing.empty() ? cfgName : (string(cfgName)+';'+existing);
    {
        char   *config_string = config->Store();
        string  awar_name     = config_prefix+cfgName;
        config->set_awar_value(awar_name, config_string);
        free(config_string);
    }
    config->set_awar_value("existing", existing);
}
static void delete_cb(AW_window *, AWT_configuration *config) {
    string existing = config->get_awar_value("existing");
    string cfgName  = config->get_awar_value("current");
    remove_from_configs(cfgName, existing); // remove selected from existing configs
    config->set_awar_value("current", "");
    config->set_awar_value("existing", existing);
}
static void load_cb(AW_window *, AWT_configuration *config) {
    string   cfgName = config->get_awar_value("current");
    GB_ERROR error   = NULL;

    if (cfgName.empty()) error = "Please enter or select target config";
    else {
        char *loadMask = GBS_global_string_copy("%s_*", config->get_id());
        char *filename = aw_file_selection("Load config from file", "$(ARBCONFIG)", loadMask, ".arbcfg");
        if (filename) {
            string awar_name = config_prefix+cfgName;

            error = config->Load(filename, awar_name);
            if (!error) {
                // after successful load restore and store config
                restore_cb(NULL, config);
                store_cb(NULL, config);
            }
            free(filename);
        }
        free(loadMask);
    }
    aw_message_if(error);
}
static void save_cb(AW_window *, AWT_configuration *config) {
    string   cfgName = config->get_awar_value("current");
    GB_ERROR error   = NULL;

    if (cfgName.empty()) error = "Please enter or select config to save";
    else {
        char *saveAs   = GBS_global_string_copy("%s_%s", config->get_id(), cfgName.c_str());
        char *filename = aw_file_selection("Save config to file", "$(ARBCONFIG)", saveAs, ".arbcfg");
        if (filename) {
            store_cb(NULL, config);
            string awar_name = config_prefix+cfgName;
            error            = config->Save(filename, awar_name);
            free(filename);
        }
        free(saveAs);
    }
    aw_message_if(error);
}

static void refresh_config_sellist_cb(AW_root*, AWT_configuration *config, AW_selection_list *sel) {
    string        cfgs_str = config->get_awar_value("existing");
    ConstStrArray cfgs;
    GBT_split_string(cfgs, cfgs_str.c_str(), ';');
    sel->init_from_array(cfgs, "<new>", "");
}

static AW_window *create_config_manager_window(AW_root *, AWT_configuration *config, AW_window *aww) {
    AW_window_simple *aws = new AW_window_simple;

    char *title = GBS_global_string_copy("Configurations for '%s'", aww->get_window_title());
    char *id    = GBS_global_string_copy("%s_config",               aww->get_window_id());

    aws->init(aww->get_root(), id, title);
    aws->load_xfig("awt/manage_config.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE");

    aws->at("help");
    aws->callback(makeHelpCallback("configurations.hlp"));
    aws->create_button("HELP", "HELP");

    // create awars
    AW_awar *awar_existing = config->get_awar("existing");
    AW_awar *awar_current  = config->get_awar("current");
    awar_current->add_callback(makeRootCallback(correct_key_name_cb, awar_current));

    AW_selection_list *sel = awt_create_selection_list_with_input_field(aws, awar_current->awar_name, "box", "name");

    awar_existing->add_callback(makeRootCallback(refresh_config_sellist_cb, config, sel));
    awar_existing->touch(); // fills selection list

    aws->auto_space(0, 3);
    aws->button_length(10);
    aws->at("button");

    int xpos = aws->get_at_xposition();
    int ypos = aws->get_at_yposition();

    int yoffset = 0; // irrelevant for 1st loop

    struct but {
        void (*cb)(AW_window*, AWT_configuration*);
        const char *id;
        const char *label;
        const char *mnemonic;
    } butDef[] = {
        { restore_cb, "RESTORE", "Restore", "R" },
        { store_cb,   "STORE",   "Store",   "S" },
        { delete_cb,  "DELETE",  "Delete",  "D" },
        { load_cb,    "LOAD",    "Load",    "L" },
        { save_cb,    "SAVE",    "Save",    "v" },
    };
    const int buttons = ARRAY_ELEMS(butDef);
    for (int b = 0; b<buttons; ++b) {
        const but& B = butDef[b];

        if (b>0) aws->at(xpos, ypos + b*yoffset);

        aws->callback(makeWindowCallback(B.cb, config));
        aws->create_button(B.id, B.label, B.mnemonic);

        if (!b) {
            aws->at_newline();
            int ypos2 = aws->get_at_yposition();
            yoffset   = ypos2-ypos;
            aws->at_x(xpos);
        }
    }

    return aws;
}

static void destroy_AWT_configuration(AWT_configuration *c, AW_window*) { delete c; }

void AWT_insert_config_manager(AW_window *aww, AW_default default_file_, const char *id, AWT_store_config_to_string store_cb,
                               AWT_load_config_from_string load_cb, AW_CL cl1, AW_CL cl2, const char *macro_id)
{
    /*! inserts a config-button into aww
     * @param default_file_ db where configs will be stored
     * @param id unique id (has to be a key)
     * @param store_cb creates a string from current state
     * @param load_cb restores state from string
     * @param macro_id custom macro id (normally NULL will do)
     */
    AWT_configuration * const config = new AWT_configuration(default_file_, id, store_cb, load_cb, cl1, cl2);

    aww->button_length(0); // -> autodetect size by size of graphic
    aww->callback(makeCreateWindowCallback(create_config_manager_window, destroy_AWT_configuration, config, aww));
    aww->create_button(macro_id ? macro_id : "SAVELOAD_CONFIG", "#conf_save.xpm");
}

static GB_ERROR decode_escapes(string& s) {
    string::iterator f = s.begin();
    string::iterator t = s.begin();

    for (; f != s.end(); ++f, ++t) {
        if (*f == '\\') {
            ++f;
            if (f == s.end()) return GBS_global_string("Trailing \\ in '%s'", s.c_str());
            switch (*f) {
                case 'n':
                    *t = '\n';
                    break;
                case 'r':
                    *t = '\r';
                    break;
                case 't':
                    *t = '\t';
                    break;
                default:
                    *t = *f;
                    break;
            }
        }
        else {
            *t = *f;
        }
    }

    s.erase(t, f);

    return 0;
}

static void encode_escapes(string& s, const char *to_escape) {
    string neu;
    neu.reserve(s.length()*2+1);

    for (string::iterator p = s.begin(); p != s.end(); ++p) {
        if (*p == '\\' || strchr(to_escape, *p) != 0) {
            neu = neu+'\\'+*p;
        }
        else if (*p == '\n') { neu = neu+"\\n"; }
        else if (*p == '\r') { neu = neu+"\\r"; }
        else if (*p == '\t') { neu = neu+"\\t"; }
        else { neu = neu+*p; }
    }
    s = neu;
}

typedef map<string, string> config_map;
struct AWT_config_mapping {
    config_map cmap;

    config_map::iterator entry(const string &e) { return cmap.find(e); }

    config_map::iterator begin() { return cmap.begin(); }
    config_map::const_iterator end() const { return cmap.end(); }
    config_map::iterator end() { return cmap.end(); }
};

// -------------------
//      AWT_config

AWT_config::AWT_config(const char *config_char_ptr)
    : mapping(new AWT_config_mapping)
    , parse_error(0)
{
    // parse string in format "key1='value1';key2='value2'"..
    // and put values into a map.
    // assumes that keys are unique

    string      configString(config_char_ptr);
    config_map& cmap  = mapping->cmap;
    size_t      pos   = 0;

    while (!parse_error) {
        size_t equal = configString.find('=', pos);
        if (equal == string::npos) break;

        if (configString[equal+1] != '\'') {
            parse_error = "expected quote \"'\"";
            break;
        }
        size_t start = equal+2;
        size_t end   = configString.find('\'', start);
        while (end != string::npos) {
            if (configString[end-1] != '\\') break;
            end = configString.find('\'', end+1);
        }
        if (end == string::npos) {
            parse_error = "could not find matching quote \"'\"";
            break;
        }

        string config_name = configString.substr(pos, equal-pos);
        string value       = configString.substr(start, end-start);

        parse_error = decode_escapes(value);
        if (!parse_error) {
            cmap[config_name] = value;
        }

        pos = end+2;            // skip ';'
    }
}
AWT_config::AWT_config(const AWT_config_mapping *cfgname_2_awar)
    : mapping(new AWT_config_mapping),
      parse_error(0)
{
    const config_map&  awarmap  = cfgname_2_awar->cmap;
    config_map&        valuemap = mapping->cmap;
    AW_root           *aw_root  = AW_root::SINGLETON;

    for (config_map::const_iterator c = awarmap.begin(); c != awarmap.end(); ++c) {
        const string& key(c->first);
        const string& awar_name(c->second);

        char *awar_value = aw_root->awar(awar_name.c_str())->read_as_string();
        valuemap[key]    = awar_value;
        free(awar_value);
    }

    awt_assert(valuemap.size() == awarmap.size());
}

AWT_config::~AWT_config() {
    delete mapping;
}

bool AWT_config::has_entry(const char *entry) const {
    awt_assert(!parse_error);
    return mapping->entry(entry) != mapping->end();
}
const char *AWT_config::get_entry(const char *entry) const {
    awt_assert(!parse_error);
    config_map::iterator found = mapping->entry(entry);
    return (found == mapping->end()) ? 0 : found->second.c_str();
}
void AWT_config::set_entry(const char *entry, const char *value) {
    awt_assert(!parse_error);
    mapping->cmap[entry] = value;
}
void AWT_config::delete_entry(const char *entry) {
    awt_assert(!parse_error);
    mapping->cmap.erase(entry);
}

char *AWT_config::config_string() const {
    awt_assert(!parse_error);
    string result;
    for (config_map::iterator e = mapping->begin(); e != mapping->end(); ++e) {
        const string& config_name(e->first);
        string        value(e->second);

        encode_escapes(value, "\'");
        string entry = config_name+"='"+value+'\'';
        if (result.empty()) {
            result = entry;
        }
        else {
            result = result+';'+entry;
        }
    }
    return strdup(result.c_str());
}
GB_ERROR AWT_config::write_to_awars(const AWT_config_mapping *cfgname_2_awar) const {
    GB_ERROR       error = 0;
    GB_transaction ta(AW_ROOT_DEFAULT);
    // Notes:
    // * Opening a TA on AW_ROOT_DEFAULT has no effect, as awar-DB is TA-free and each
    //   awar-change opens a TA anyway.
    // * Motif version did open TA on awar->gb_var (in 1st loop), which would make a
    //   difference IF the 1st awar is bound to main-DB. At best old behavior was obscure.

    awt_assert(!parse_error);
    AW_root *aw_root = AW_root::SINGLETON;
    for (config_map::iterator e = mapping->begin(); !error && e != mapping->end(); ++e) {
        const string& config_name(e->first);
        const string& value(e->second);

        config_map::const_iterator found = cfgname_2_awar->cmap.find(config_name);
        if (found == cfgname_2_awar->end()) {
            error = GBS_global_string("config contains unmapped entry '%s'", config_name.c_str());
        }
        else {
            const string&  awar_name(found->second);
            AW_awar       *awar = aw_root->awar(awar_name.c_str());
            awar->write_as_string(value.c_str());
        }
    }
    return error;
}

// ------------------------------
//      AWT_config_definition

AWT_config_definition::AWT_config_definition()
    : config_mapping(new AWT_config_mapping)
{}

AWT_config_definition::AWT_config_definition(AWT_config_mapping_def *mdef)
    : config_mapping(new AWT_config_mapping)
{
    add(mdef);
}

AWT_config_definition::~AWT_config_definition() {
    delete config_mapping;
}

void AWT_config_definition::add(const char *awar_name, const char *config_name) {
    config_mapping->cmap[config_name] = awar_name;
}
void AWT_config_definition::add(const char *awar_name, const char *config_name, int counter) {
    add(awar_name, GBS_global_string("%s%i", config_name, counter));
}
void AWT_config_definition::add(AWT_config_mapping_def *mdef) {
    while (mdef->awar_name && mdef->config_name) {
        add(mdef->awar_name, mdef->config_name);
        mdef++;
    }
}

char *AWT_config_definition::read() const {
    // creates a string from awar values

    AWT_config current_state(config_mapping);
    return current_state.config_string();
}
void AWT_config_definition::write(const char *config_char_ptr) const {
    // write values from string to awars
    // if the string contains unknown settings, they are silently ignored

    AWT_config wanted_state(config_char_ptr);
    GB_ERROR   error  = wanted_state.parseError();
    if (!error) error = wanted_state.write_to_awars(config_mapping);
    if (error) aw_message(GBS_global_string("Error restoring configuration (%s)", error));
}
