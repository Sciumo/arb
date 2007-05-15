#ifndef P_
#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif
#endif


/* ORS_C_main.cxx */
void get_indata P_((void));
char *cgi_var P_((char *name));
char *cgi_var_or_nil P_((char *name));
void set_cgi_var P_((char *name, char *content));
char *OC_cgi_var_2_filename P_((char *cv, char *lib_file, char *kontext));
void print_content_lines P_((char *title));
void save_environment_for_debugger P_((void));
int main P_((int argc, char **argv));

/* ORS_C_PT.cxx */
int init_pt_local_struct P_((void));
char *OC_probe_pt_look_for_server P_((int server_nr));
void probe_match P_((int server_nr, char *probe_seq, int max_mismatches, int weighted_mis, int reversed, int complement, int max_hits));
void probe_find P_((int server_nr, char *probe_seq));
void output_html_match_result P_((void));
char *output_java_match_result P_((char *filename, char *path_of_tree));
void free_results P_((void));

/* ORS_C_html.cxx */
void quit_with_error P_((const char *message));
char *a2html P_((char *text));
void print_header P_((void));
void output_list_as_html_options P_((char *list, char *selected, int public_mode));
void output_list_as_html_options_special P_((char *list, char *selected, int public_mode, int from, int every));
void OC_html_www_home P_((char *content, int mode, char *param, int row, int col));
void OC_html_probe_id_link P_((char *content, int mode, char *param, int row, int col));
void OC_html_userpath_link P_((char *content, int mode, char *param, int row, int col));
void output_2_lists_as_text_table P_((char *list1, int width1, char *between, char *list2, char *eol));
void output_list_as_2_col_table P_((char *list1, int width1, char *between, char *eol));
void OC_output_pdb_list P_((OC_pdb_list *list, char *select, char *how));
void OC_output_pdb_list_elem P_((OC_pdb_list *elem, char *select, char *how, char *owner));
void OC_output_links P_((char *list, char seperator, char *prefix, char *eol));
char *list_of_pub_parents P_((char *highest, char *lowest));
char *OC_build_sel_userpath P_((char *dad, char *me));
void print_path_to_images P_((void));

/* ORS_C_java.cxx */
GB_INLINE void t2j_write_nibble P_((int value));
GB_INLINE void t2j_flush_nibble P_((void));
GB_INLINE void t2j_flush_bits P_((void));
GB_INLINE void t2j_write_bit P_((int bit));
void t2j_write_byte P_((int value));
char *t2j_write_int2 P_((unsigned int value));
char *t2j_write_uint P_((unsigned int value, int or_cmd));
CAT_node_id t2j_get_focus_right_bound P_((CAT_node_id focus));
long t2j_compare_two_hitems P_((struct t2j_s1_item_header *hi0, struct t2j_s1_item_header *hi1));
long t2j_compare_two_ids P_((long i1, long i2));
void t2j_write_range P_((int start_of_range, int end_of_range));
void t2j_indexlist_2_OTF P_((long *buffer, long buf_size));
GB_ERROR T2J_send_tree P_((CAT_node_id focus, char *modifier_string));
GB_ERROR T2J_transform P_((char *path_of_tree, char *modifier_string, struct T2J_transfer_struct *data, CAT_node_id focus, FILE *out));
GB_ERROR T2J_send_bit_coded_tree P_((char *path_of_tree, FILE *out));
CAT_node_id *t2j_mcreate_level_indexing P_((void));
GB_ERROR T2J_send_branch_lengths P_((char *path_of_tree, FILE *out));
struct T2J_transfer_struct *T2J_read_query_result_from_data P_((char *data, CAT_FIELDS catfield));
struct T2J_transfer_struct *T2J_read_query_result_from_pts P_((char *data));
struct T2J_transfer_struct *T2J_read_query_result_from_file P_((char *path, CAT_FIELDS catfield));
GB_ERROR T2J_transfer_fullnames1 P_((char *path_of_tree, FILE *out));
GB_ERROR T2J_transfer_fullnames2 P_((char *path_of_tree, FILE *out));
GB_ERROR T2J_transfer_group_names P_((char *path_of_tree, FILE *out));
long t2j_get_deepest_node_that_contains_all_selected P_((CAT_node_id nn, char *selected_ids, long nselected, CAT_node_id *focusout));
char *T2J_get_selection P_((char *path_of_tree, char *sel, const char *varname, int all_nodes, CAT_node_id *focusout, char **maxnodeout, double *maxnodehits));
int main P_((int argc, char **argv));

/* ORS_C_user.cxx */
void OC_save_userdb P_((void));
void OC_login_user P_((char *password));
void OC_logout_user P_((void));
void OC_dailypw_2_userpath P_((void));
void OC_get_sel_userdata_from_server P_((void));
char *OC_read_user_field_if_userpath_set P_((char *sel_userpath, char *fieldname));
char *OC_read_user_field P_((char *sel_userpath, char *fieldname));
char *list_of_users P_((char *keyword));
void work_on_user P_((void));
void work_on_sel_user P_((char *action));

/* ORS_C_lib.cxx */
void OC_write_log_message P_((char *comment));
void OC_server_error_if_not_empty P_((char *locs_error));
int init_local_com_struct P_((void));
char *ORS_look_for_server P_((void));
void init_server_communication P_((void));
char *ORS_read_a_line_in_a_file P_((char *filename, char *env));
void OC_normalize_seq P_((char *seq, char *allowed_bases));
void OC_calculate_seq_and_target_seq P_((char **seq1, char **seq2, char *allowed_bases));

/* ORS_C_java_main.cxx */
void ORS_C_make_clean P_((char *data));
GB_ERROR ORS_C_try_to_do_java_function P_((void));

/* ORS_C_scripts.cxx */
char *ors_remove_leading_spaces P_((char *data));
FILE *ors_tcp_open P_((char *mach_name, int socket_id));
char *ors_form_2_java P_((char *data));
GB_ERROR ors_ovp_2_java P_((char *id, char *data, char *path_of_tree, FILE *out));
GB_ERROR ORS_file_2_java P_((char *command, char *tree, FILE *fd));
void ors_client_error P_((char *error));
GB_ERROR ORS_C_exec_script P_((char *tree, char *command));

/* ORS_C_probe.cxx */
void OC_save_probedb P_((void));
void OC_put_probe_field P_((char *field_section, char *field_name, char *field_data));
void OC_put_probe_ta_id P_((int ta_id));
char *OC_get_probe_field P_((char *field_section, char *field_name));
int OC_get_probe_ta_id P_((void));
void OC_work_on_probe P_((char *action));
void OC_probe_select P_((char *probe_id));
char *OC_probe_query P_((int max_count));
char *OC_get_probe_list P_((int type, int count));
OC_pdb_list *OC_create_pdb_list_elem P_((void));
void OC_pdb_list_insert P_((OC_pdb_list **head, OC_pdb_list *elem));
void OC_pdb_list_delete P_((OC_pdb_list *head, OC_pdb_list *elem));
OC_pdb_list *OC_next_pdb_list_elem P_((OC_pdb_list *head, OC_pdb_list *elem));
OC_pdb_list *OC_next_ors_gl_pdb_list_elem P_((OC_pdb_list *elem));
OC_pdb_list *OS_find_pdb_list_elem_by_name P_((OC_pdb_list *head, char *section, char *name));
void OC_read_probedb_fields_into_pdb_list P_((void));
void OC_clear_pdb_fields_in_server P_((void));
void OC_send_pdb_fields_to_server P_((char *selection));
void OC_get_pdb_fields_from_server P_((char *probe_id, char *selection));
void OC_get_pdb_fields_from_cgi_vars P_((char *selection));
void OC_output_html_probe_list P_((int type, char *headline));
char *OC_read_file_into_list P_((char *file_name));
void OC_probe_user_transfer P_((char *from_userpath, char *to_userpath, char *probe_id));

/* ORS_lib.cxx */
char *ORS_crypt P_((char *password));
char *ORS_export_error P_((char *templat, ...));
char *ORS_sprintf P_((char *templat, ...));
char *ORS_time_and_date_string P_((int type, long time));
int ORS_str_char_count P_((char *str, char seek));
int ORS_strncmp P_((char *str1, char *str2));
int ORS_strncase_tail_cmp P_((char *str1, char *str2));
int ORS_strncasecmp P_((char *str1, char *str2));
char *ORS_trim P_((char *str));
int ORS_contains_word P_((char *buffer, char *word, char seperator));
int ORS_seq_matches_target_seq P_((char *seq, char *target, int complemented));
char *ORS_calculate_4gc_2at P_((char *sequence));
char *ORS_calculate_gc_ratio P_((char *sequence));
int ORS_strcmp P_((char *str1, char *str2));
int OC_find_next_chr P_((char **start, char **end, char chr));
int ORS_is_parent_of P_((char *user1, char *user2));
int ORS_is_parent_or_equal P_((char *user1, char *user2));
char *ORS_get_parent_of P_((char *userpath));
char *ORS_str_to_upper P_((char *str1));

/* ORS_C_rtok.cxx */
void OC_output_html_page P_((char *new_html));
void OC_output_html_buffer P_((char *page, char *html));

#undef P_
