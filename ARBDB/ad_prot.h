/*
 * ARB database interface.
 *
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 *
 */

#ifndef AD_PROT_H
#define AD_PROT_H

#ifndef P_
# error P_ is not defined
#endif

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* adsort.c */
void GB_sort P_((void **array, size_t first, size_t behind_last, gb_compare_function compare, void *client_data));
int GB_string_comparator P_((const void *v0, const void *v1, void *unused));

/* adlang1.c */
NOT4PERL void GB_set_export_sequence_hook P_((gb_export_sequence_cb escb));
void GB_set_ACISRT_trace P_((int enable));
int GB_get_ACISRT_trace P_((void));

/* adstring.c */
char *GB_find_all_files P_((const char *dir, const char *mask, GB_BOOL filename_only));
char *GB_find_latest_file P_((const char *dir, const char *mask));
void GB_raise_critical_error P_((const char *msg));
GB_ERROR GB_export_error P_((const char *error));
GB_ERROR GB_export_errorf P_((const char *templat, ...)) __ATTR__FORMAT(1);
GB_ERROR GB_export_IO_error P_((const char *action, const char *filename));
GB_ERROR GB_print_error P_((void));
NOT4PERL GB_ERROR GB_get_error P_((void)) __ATTR__DEPRECATED;
GB_BOOL GB_have_error P_((void));
GB_ERROR GB_await_error P_((void));
void GB_clear_error P_((void));
GB_ERROR GB_failedTo_error P_((const char *do_something, const char *special, GB_ERROR error));
GB_ERROR GB_append_exportedError P_((GB_ERROR error));
void GBS_reuse_buffer P_((GB_CSTR global_buffer));
GB_CSTR GBS_global_string P_((const char *templat, ...)) __ATTR__FORMAT(1);
char *GBS_global_string_copy P_((const char *templat, ...)) __ATTR__FORMAT(1);
const char *GBS_global_string_to_buffer P_((char *buffer, size_t bufsize, const char *templat, ...)) __ATTR__FORMAT(3);
size_t GBS_last_global_string_size P_((void));
char *GBS_string_2_key_with_exclusions P_((const char *str, const char *additional));
char *GBS_string_2_key P_((const char *str));
GB_ERROR GB_check_key P_((const char *key)) __ATTR__USERESULT;
GB_ERROR GB_check_link_name P_((const char *key)) __ATTR__USERESULT;
GB_ERROR GB_check_hkey P_((const char *key)) __ATTR__USERESULT;
char *GBS_remove_escape P_((char *com));
char *GBS_escape_string P_((const char *str, const char *chars_to_escape, char escape_char));
char *GBS_unescape_string P_((const char *str, const char *escaped_chars, char escape_char));
struct GBS_strstruct *GBS_stropen P_((long init_size));
char *GBS_strclose P_((struct GBS_strstruct *strstr));
void GBS_strforget P_((struct GBS_strstruct *strstr));
GB_BUFFER GBS_mempntr P_((struct GBS_strstruct *strstr));
long GBS_memoffset P_((struct GBS_strstruct *strstr));
void GBS_str_cut_tail P_((struct GBS_strstruct *strstr, int byte_count));
void GBS_strncat P_((struct GBS_strstruct *strstr, const char *ptr, size_t len));
void GBS_strcat P_((struct GBS_strstruct *strstr, const char *ptr));
void GBS_strnprintf P_((struct GBS_strstruct *strstr, long len, const char *templat, ...)) __ATTR__FORMAT(3);
void GBS_chrcat P_((struct GBS_strstruct *strstr, char ch));
void GBS_intcat P_((struct GBS_strstruct *strstr, long val));
void GBS_floatcat P_((struct GBS_strstruct *strstr, double val));
char *GBS_eval_env P_((GB_CSTR p));
char *GBS_find_lib_file P_((const char *filename, const char *libprefix, int warn_when_not_found));
char **GBS_read_dir P_((const char *dir, const char *mask));
long GBS_gcgchecksum P_((const char *seq));
uint32_t GB_checksum P_((const char *seq, long length, int ignore_case, const char *exclude));
uint32_t GBS_checksum P_((const char *seq, int ignore_case, const char *exclude));
char *GBS_extract_words P_((const char *source, const char *chars, float minlen, GB_BOOL sort_output));
size_t GBS_shorten_repeated_data P_((char *data));
NOT4PERL void GB_install_error_handler P_((gb_error_handler_type aw_message_handler));
void GB_internal_error P_((const char *message));
void GB_internal_errorf P_((const char *templat, ...)) __ATTR__FORMAT(1);
void GB_warning P_((const char *message));
void GB_warningf P_((const char *templat, ...)) __ATTR__FORMAT(1);
NOT4PERL void GB_install_warning P_((gb_warning_func_type warn));
void GB_information P_((const char *message));
void GB_informationf P_((const char *templat, ...)) __ATTR__FORMAT(1);
NOT4PERL void GB_install_information P_((gb_information_func_type info));
int GB_status P_((double val));
NOT4PERL void GB_install_status P_((gb_status_func_type func));
int GB_status2 P_((const char *templat, ...)) __ATTR__FORMAT(1);
NOT4PERL void GB_install_status2 P_((gb_status_func2_type func2));
char *GBS_merge_tagged_strings P_((const char *s1, const char *tag1, const char *replace1, const char *s2, const char *tag2, const char *replace2));
char *GBS_string_eval_tagged_string P_((GBDATA *gb_main, const char *s, const char *dt, const char *tag, const char *srt, const char *aci, GBDATA *gbd));
char *GB_read_as_tagged_string P_((GBDATA *gbd, const char *tagi));
void GBS_fwrite_string P_((const char *strngi, FILE *out));
char *GBS_fread_string P_((FILE *in));
char *GBS_fconvert_string P_((char *buffer));
char *GBS_replace_tabs_by_spaces P_((const char *text));
int GBS_strscmp P_((const char *s1, const char *s2));
const char *GBS_readable_size P_((unsigned long long size));
char *GBS_trim P_((const char *str));

/* admatch.c */
GBS_MATCHER *GBS_compile_matcher P_((const char *search_expr, GB_CASE case_flag));
void GBS_free_matcher P_((GBS_MATCHER *matcher));
GBS_REGEX *GBS_compile_regexpr P_((const char *regexpr, GB_CASE case_flag, GB_ERROR *error));
void GBS_free_regexpr P_((GBS_REGEX *toFree));
const char *GBS_unwrap_regexpr P_((const char *regexpr_in_slashes, GB_CASE *case_flag, GB_ERROR *error));
const char *GBS_regmatch_compiled P_((const char *str, GBS_REGEX *comreg, size_t *matchlen));
const char *GBS_regmatch P_((const char *str, const char *regExpr, size_t *matchlen, GB_ERROR *error));
char *GBS_regreplace P_((const char *str, const char *regReplExpr, GB_ERROR *error));
GB_CSTR GBS_find_string P_((GB_CSTR str, GB_CSTR substr, int match_mode));
GB_BOOL GBS_string_matches P_((const char *str, const char *search, GB_CASE case_sens));
GB_BOOL GBS_string_matches_regexp P_((const char *str, const GBS_MATCHER *expr));
char *GBS_string_eval P_((const char *insource, const char *icommand, GBDATA *gb_container));

/* arbdb.c */
char *GB_rel P_((void *struct_address, long rel_address));
NOT4PERL GB_ERROR GB_safe_atof P_((const char *str, double *res));
double GB_atof P_((const char *str));
GB_BUFFER GB_give_buffer P_((size_t size));
GB_BUFFER GB_increase_buffer P_((size_t size));
int GB_give_buffer_size P_((void));
GB_BUFFER GB_give_buffer2 P_((long size));
int GB_is_in_buffer P_((GB_CBUFFER ptr));
char *GB_check_out_buffer P_((GB_CBUFFER buffer));
GB_BUFFER GB_give_other_buffer P_((GB_CBUFFER buffer, long size));
void GB_init_gb P_((void));
void GB_atclose P_((GBDATA *gbd, void (*fun )(GBDATA *gb_main, void *client_data ), void *client_data));
void GB_close P_((GBDATA *gbd));
long GB_read_int P_((GBDATA *gbd));
int GB_read_byte P_((GBDATA *gbd));
void *GB_read_pointer P_((GBDATA *gbd));
double GB_read_float P_((GBDATA *gbd));
long GB_read_count P_((GBDATA *gbd));
long GB_read_memuse P_((GBDATA *gbd));
GB_CSTR GB_read_pntr P_((GBDATA *gbd));
GB_CSTR GB_read_char_pntr P_((GBDATA *gbd));
char *GB_read_string P_((GBDATA *gbd));
long GB_read_string_count P_((GBDATA *gbd));
GB_CSTR GB_read_link_pntr P_((GBDATA *gbd));
char *GB_read_link P_((GBDATA *gbd));
long GB_read_link_count P_((GBDATA *gbd));
long GB_read_bits_count P_((GBDATA *gbd));
GB_CSTR GB_read_bits_pntr P_((GBDATA *gbd, char c_0, char c_1));
char *GB_read_bits P_((GBDATA *gbd, char c_0, char c_1));
GB_CSTR GB_read_bytes_pntr P_((GBDATA *gbd));
long GB_read_bytes_count P_((GBDATA *gbd));
char *GB_read_bytes P_((GBDATA *gbd));
GB_CUINT4 *GB_read_ints_pntr P_((GBDATA *gbd));
long GB_read_ints_count P_((GBDATA *gbd));
GB_UINT4 *GB_read_ints P_((GBDATA *gbd));
GB_CFLOAT *GB_read_floats_pntr P_((GBDATA *gbd));
long GB_read_floats_count P_((GBDATA *gbd));
float *GB_read_floats P_((GBDATA *gbd));
char *GB_read_as_string P_((GBDATA *gbd));
long GB_read_from_ints P_((GBDATA *gbd, long index));
double GB_read_from_floats P_((GBDATA *gbd, long index));
GB_ERROR GB_write_byte P_((GBDATA *gbd, int i));
GB_ERROR GB_write_int P_((GBDATA *gbd, long i));
GB_ERROR GB_write_pointer P_((GBDATA *gbd, void *pointer));
GB_ERROR GB_write_float P_((GBDATA *gbd, double f));
GB_ERROR GB_write_pntr P_((GBDATA *gbd, const char *s, long bytes_size, long stored_size));
GB_ERROR GB_write_string P_((GBDATA *gbd, const char *s));
GB_ERROR GB_write_link P_((GBDATA *gbd, const char *s));
GB_ERROR GB_write_bits P_((GBDATA *gbd, const char *bits, long size, const char *c_0));
GB_ERROR GB_write_bytes P_((GBDATA *gbd, const char *s, long size));
GB_ERROR GB_write_ints P_((GBDATA *gbd, const GB_UINT4 *i, long size));
GB_ERROR GB_write_floats P_((GBDATA *gbd, const float *f, long size));
GB_ERROR GB_write_as_string P_((GBDATA *gbd, const char *val));
int GB_read_security_write P_((GBDATA *gbd));
int GB_read_security_read P_((GBDATA *gbd));
int GB_read_security_delete P_((GBDATA *gbd));
int GB_get_my_security P_((GBDATA *gbd));
GB_ERROR GB_write_security_write P_((GBDATA *gbd, unsigned long level));
GB_ERROR GB_write_security_read P_((GBDATA *gbd, unsigned long level));
GB_ERROR GB_write_security_delete P_((GBDATA *gbd, unsigned long level));
GB_ERROR GB_write_security_levels P_((GBDATA *gbd, unsigned long readlevel, unsigned long writelevel, unsigned long deletelevel));
GB_ERROR GB_change_my_security P_((GBDATA *gbd, int level, const char *passwd));
void GB_push_my_security P_((GBDATA *gbd));
void GB_pop_my_security P_((GBDATA *gbd));
GB_TYPES GB_read_type P_((GBDATA *gbd));
char *GB_read_key P_((GBDATA *gbd));
GB_CSTR GB_read_key_pntr P_((GBDATA *gbd));
GBQUARK GB_key_2_quark P_((GBDATA *gbd, const char *s));
GBQUARK GB_get_quark P_((GBDATA *gbd));
GB_BOOL GB_has_key P_((GBDATA *gbd, const char *key));
long GB_read_clock P_((GBDATA *gbd));
long GB_read_transaction P_((GBDATA *gbd));
GBDATA *GB_get_father P_((GBDATA *gbd));
GBDATA *GB_get_grandfather P_((GBDATA *gbd));
GBDATA *GB_get_root P_((GBDATA *gbd));
GB_BOOL GB_check_father P_((GBDATA *gbd, GBDATA *gb_maybefather));
int GB_rename P_((GBDATA *gbc, const char *new_key));
GBDATA *GB_create P_((GBDATA *father, const char *key, GB_TYPES type));
GBDATA *GB_create_container P_((GBDATA *father, const char *key));
GB_ERROR GB_delete P_((GBDATA *source));
GB_ERROR GB_copy P_((GBDATA *dest, GBDATA *source));
GB_ERROR GB_copy_with_protection P_((GBDATA *dest, GBDATA *source, GB_BOOL copy_all_protections));
char *GB_get_subfields P_((GBDATA *gbd));
GB_ERROR GB_set_compression P_((GBDATA *gb_main, GB_COMPRESSION_MASK disable_compression));
GB_ERROR GB_set_temporary P_((GBDATA *gbd));
GB_ERROR GB_clear_temporary P_((GBDATA *gbd));
GB_BOOL GB_is_temporary P_((GBDATA *gbd));
GB_BOOL GB_in_temporary_branch P_((GBDATA *gbd));
GB_ERROR GB_push_local_transaction P_((GBDATA *gbd));
GB_ERROR GB_pop_local_transaction P_((GBDATA *gbd));
GB_ERROR GB_push_transaction P_((GBDATA *gbd));
GB_ERROR GB_pop_transaction P_((GBDATA *gbd));
GB_ERROR GB_begin_transaction P_((GBDATA *gbd));
GB_ERROR GB_no_transaction P_((GBDATA *gbd));
GB_ERROR GB_abort_transaction P_((GBDATA *gbd));
GB_ERROR GB_commit_transaction P_((GBDATA *gbd));
GB_ERROR GB_end_transaction P_((GBDATA *gbd, GB_ERROR error));
void GB_end_transaction_show_error P_((GBDATA *gbd, GB_ERROR error, void (*error_handler )(GB_ERROR )));
int GB_get_transaction_level P_((GBDATA *gbd));
GB_ERROR GB_update_server P_((GBDATA *gbd));
NOT4PERL GB_BOOL GB_inside_callback P_((GBDATA *of_gbd, enum gb_call_back_type cbtype));
GBDATA *GB_get_gb_main_during_cb P_((void));
NOT4PERL const void *GB_read_old_value P_((void));
long GB_read_old_size P_((void));
char *GB_get_callback_info P_((GBDATA *gbd));
GB_ERROR GB_add_priority_callback P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata, int priority));
GB_ERROR GB_add_callback P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata));
void GB_remove_callback P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata));
void GB_remove_all_callbacks_to P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func));
GB_ERROR GB_ensure_callback P_((GBDATA *gbd, enum gb_call_back_type type, GB_CB func, int *clientdata));
GB_ERROR GB_release P_((GBDATA *gbd));
int GB_testlocal P_((GBDATA *gbd));
int GB_nsons P_((GBDATA *gbd));
void GB_disable_quicksave P_((GBDATA *gbd, const char *reason));
GB_ERROR GB_resort_data_base P_((GBDATA *gb_main, GBDATA **new_order_list, long listsize));
GB_ERROR GB_resort_system_folder_to_top P_((GBDATA *gb_main));
GB_ERROR GB_write_usr_public P_((GBDATA *gbd, long flags));
long GB_read_usr_public P_((GBDATA *gbd));
long GB_read_usr_private P_((GBDATA *gbd));
GB_ERROR GB_write_usr_private P_((GBDATA *gbd, long ref));
void GB_write_flag P_((GBDATA *gbd, long flag));
int GB_read_flag P_((GBDATA *gbd));
void GB_touch P_((GBDATA *gbd));
GB_ERROR GB_print_debug_information P_((void *dummy, GBDATA *gb_main));
int GB_info P_((GBDATA *gbd));
long GB_number_of_subentries P_((GBDATA *gbd));

/* admath.c */
double GB_log_fak P_((int n));
double GB_frandom P_((void));
int GB_random P_((int range));

/* adoptimize.c */
GB_ERROR GB_optimize P_((GBDATA *gb_main));

/* adsystem.c */
struct DictData *GB_get_dictionary P_((GBDATA *gb_main, const char *key));
GB_ERROR GB_set_dictionary P_((GBDATA *gb_main, const char *key, const struct DictData *dd));
void GB_free_dictionary P_((struct DictData *dd));

/* adindex.c */
GB_ERROR GB_create_index P_((GBDATA *gbd, const char *key, GB_CASE case_sens, long estimated_size));
NOT4PERL void GB_dump_indices P_((GBDATA *gbd));
GB_ERROR GB_request_undo_type P_((GBDATA *gb_main, GB_UNDO_TYPE type)) __ATTR__USERESULT;
GB_UNDO_TYPE GB_get_requested_undo_type P_((GBDATA *gb_main));
GB_ERROR GB_undo P_((GBDATA *gb_main, GB_UNDO_TYPE type)) __ATTR__USERESULT;
char *GB_undo_info P_((GBDATA *gb_main, GB_UNDO_TYPE type));
GB_ERROR GB_set_undo_mem P_((GBDATA *gbd, long memsize));

/* adperl.c */
GB_UNDO_TYPE GBP_undo_type P_((char *type));
int GBP_search_mode P_((char *search_mode));
const char *GBP_type_to_string P_((GB_TYPES type));
GB_TYPES GBP_gb_types P_((char *type_name));
GB_UNDO_TYPE GBP_undo_types P_((const char *type_name));
const char *GBP_undo_type_2_string P_((GB_UNDO_TYPE type));

/* adlink.c */
GBDATA *GB_follow_link P_((GBDATA *gb_link));
GB_ERROR GB_install_link_follower P_((GBDATA *gb_main, const char *link_type, GB_Link_Follower link_follower));

/* adsocket.c */
void GB_usleep P_((long usec));
GB_ULONG GB_time_of_file P_((const char *path));
long GB_size_of_file P_((const char *path));
long GB_mode_of_file P_((const char *path));
long GB_mode_of_link P_((const char *path));
GB_BOOL GB_is_regularfile P_((const char *path));
GB_BOOL GB_is_executablefile P_((const char *path));
GB_BOOL GB_is_privatefile P_((const char *path, GB_BOOL read_private));
GB_BOOL GB_is_readablefile P_((const char *filename));
GB_BOOL GB_is_directory P_((const char *path));
long GB_getuid_of_file P_((char *path));
int GB_unlink P_((const char *path));
void GB_unlink_or_warn P_((const char *path, GB_ERROR *error));
char *GB_follow_unix_link P_((const char *path));
GB_ERROR GB_symlink P_((const char *name1, const char *name2));
GB_ERROR GB_set_mode_of_file P_((const char *path, long mode));
GB_ERROR GB_rename_file P_((const char *oldpath, const char *newpath));
char *GB_read_fp P_((FILE *in));
char *GB_read_file P_((const char *path));
char *GB_map_FILE P_((FILE *in, int writeable));
char *GB_map_file P_((const char *path, int writeable));
long GB_size_of_FILE P_((FILE *in));
GB_ULONG GB_time_of_day P_((void));
long GB_last_saved_clock P_((GBDATA *gb_main));
GB_ULONG GB_last_saved_time P_((GBDATA *gb_main));
GB_ERROR GB_textprint P_((const char *path)) __ATTR__USERESULT;
GB_CSTR GB_getcwd P_((void));
GB_ERROR GB_system P_((const char *system_command)) __ATTR__USERESULT;
GB_ERROR GB_xterm P_((void)) __ATTR__USERESULT;
GB_ERROR GB_xcmd P_((const char *cmd, GB_BOOL background, GB_BOOL wait_only_if_error)) __ATTR__USERESULT;
char *GB_executable P_((GB_CSTR exe_name));
char *GB_find_executable P_((GB_CSTR description_of_executable, ...)) __ATTR__SENTINEL;
GB_CSTR GB_getenvUSER P_((void));
GB_CSTR GB_getenvHOME P_((void));
GB_CSTR GB_getenvARBHOME P_((void));
GB_CSTR GB_getenvARBMACRO P_((void));
GB_CSTR GB_getenvARBMACROHOME P_((void));
GB_CSTR GB_getenvARBCONFIG P_((void));
GB_CSTR GB_getenvPATH P_((void));
GB_CSTR GB_getenvARB_GS P_((void));
GB_CSTR GB_getenvARB_PDFVIEW P_((void));
GB_CSTR GB_getenvARB_TEXTEDIT P_((void));
GB_CSTR GB_getenvDOCPATH P_((void));
GB_CSTR GB_getenvHTMLDOCPATH P_((void));
GB_CSTR GB_getenv P_((const char *env));
int GB_host_is_local P_((const char *hostname));
GB_ULONG GB_get_physical_memory P_((void));
GB_CSTR GB_append_suffix P_((const char *name, const char *suffix));
GB_CSTR GB_get_full_path P_((const char *anypath));
GB_CSTR GB_concat_path P_((GB_CSTR anypath_left, GB_CSTR anypath_right));
GB_CSTR GB_concat_full_path P_((const char *anypath_left, const char *anypath_right));
GB_CSTR GB_path_in_ARBHOME P_((const char *relative_path_left, const char *anypath_right));
GB_CSTR GB_path_in_ARBLIB P_((const char *relative_path_left, const char *anypath_right));
FILE *GB_fopen_tempfile P_((const char *filename, const char *fmode, char **res_fullname));
char *GB_create_tempfile P_((const char *name));
char *GB_unique_filename P_((const char *name_prefix, const char *suffix));
void GB_remove_on_exit P_((const char *filename));
void GB_split_full_path P_((const char *fullpath, char **res_dir, char **res_fullname, char **res_name_only, char **res_suffix));

/* adcomm.c */
GB_ERROR GBCMS_open P_((const char *path, long timeout, GBDATA *gb_main));
void GBCMS_shutdown P_((GBDATA *gbd));
GB_BOOL GBCMS_accept_calls P_((GBDATA *gbd, GB_BOOL wait_extra_time));
long GB_read_clients P_((GBDATA *gbd));
GB_BOOL GB_is_server P_((GBDATA *gbd));
GB_BOOL GB_is_client P_((GBDATA *gbd));
GBDATA *GBCMC_find P_((GBDATA *gbd, const char *key, GB_TYPES type, const char *str, GB_CASE case_sens, enum gb_search_types gbs));
int GBCMC_system P_((GBDATA *gbd, const char *ss));
GB_ERROR GB_tell_server_dont_wait P_((GBDATA *gbd));
GB_CSTR GB_get_hostname P_((void));
GB_ERROR GB_install_pid P_((int mode));
const char *GB_date_string P_((void));

/* adhash.c */
long GBS_get_a_prime P_((long above_or_equal_this));
GB_HASH *GBS_create_hash P_((long user_size, GB_CASE case_sens));
GB_HASH *GBS_create_dynaval_hash P_((long user_size, GB_CASE case_sens, void (*freefun )(long )));
void GBS_dynaval_free P_((long val));
void GBS_optimize_hash P_((GB_HASH *hs));
char *GBS_hashtab_2_string P_((GB_HASH *hash));
char *GBS_string_2_hashtab P_((GB_HASH *hash, char *data));
long GBS_read_hash P_((const GB_HASH *hs, const char *key));
long GBS_write_hash P_((GB_HASH *hs, const char *key, long val));
long GBS_write_hash_no_strdup P_((GB_HASH *hs, char *key, long val));
long GBS_incr_hash P_((GB_HASH *hs, const char *key));
double GBS_hash_mean_access_costs P_((GB_HASH *hs));
void GBS_free_hash_entries P_((GB_HASH *hs));
void GBS_free_hash P_((GB_HASH *hs));
void GBS_clear_hash_statistic_summary P_((const char *id));
void GBS_print_hash_statistic_summary P_((const char *id));
void GBS_calc_hash_statistic P_((GB_HASH *hs, const char *id, int print));
void GBS_hash_do_loop P_((GB_HASH *hs, gb_hash_loop_type func, void *client_data));
long GBS_hash_count_elems P_((GB_HASH *hs));
long GBS_hash_count_value P_((GB_HASH *hs, long val));
const char *GBS_hash_next_element_that P_((GB_HASH *hs, const char *last_key, GB_BOOL (*condition )(const char *key, long val, void *cd ), void *cd));
void GBS_hash_do_sorted_loop P_((GB_HASH *hs, gb_hash_loop_type func, gbs_hash_compare_function sorter, void *client_data));
int GBS_HCF_sortedByKey P_((const char *k0, long v0, const char *k1, long v1));
GB_HASHI *GBS_create_hashi P_((long user_size));
long GBS_read_hashi P_((GB_HASHI *hs, long key));
long GBS_write_hashi P_((GB_HASHI *hs, long key, long val));
void GBS_free_hashi P_((GB_HASHI *hs));
char *GB_set_cache_size P_((GBDATA *gbd, long size));

/* adquery.c */
const char *GB_get_GBDATA_path P_((GBDATA *gbd));
GBDATA *GB_find_sub_by_quark P_((GBDATA *father, int key_quark, GBDATA *after));
NOT4PERL GBDATA *GB_find_subcontent_by_quark P_((GBDATA *father, int key_quark, GB_TYPES type, const char *val, GB_CASE case_sens, GBDATA *after));
GBDATA *GB_find P_((GBDATA *gbd, const char *key, long gbs));
GBDATA *GB_find_string P_((GBDATA *gbd, const char *key, const char *str, GB_CASE case_sens, long gbs));
NOT4PERL GBDATA *GB_find_int P_((GBDATA *gbd, const char *key, long val, long gbs));
GBDATA *GB_child P_((GBDATA *father));
GBDATA *GB_nextChild P_((GBDATA *child));
GBDATA *GB_entry P_((GBDATA *father, const char *key));
GBDATA *GB_nextEntry P_((GBDATA *entry));
GBDATA *GB_brother P_((GBDATA *entry, const char *key));
char *GB_first_non_key_char P_((const char *str));
GBDATA *GB_search P_((GBDATA *gbd, const char *fieldpath, long create));
GBDATA *GB_searchOrCreate_string P_((GBDATA *gb_container, const char *fieldpath, const char *default_value));
GBDATA *GB_searchOrCreate_int P_((GBDATA *gb_container, const char *fieldpath, long default_value));
GBDATA *GB_searchOrCreate_float P_((GBDATA *gb_container, const char *fieldpath, double default_value));
GBDATA *GB_search_last_son P_((GBDATA *gbd));
long GB_number_of_marked_subentries P_((GBDATA *gbd));
GBDATA *GB_first_marked P_((GBDATA *gbd, const char *keystring));
GBDATA *GB_next_marked P_((GBDATA *gbd, const char *keystring));
char *GB_command_interpreter P_((GBDATA *gb_main, const char *str, const char *commands, GBDATA *gbd, const char *default_tree_name));

/* ad_save_load.c */
GB_ERROR GB_save P_((GBDATA *gb, const char *path, const char *savetype));
GB_ERROR GB_create_directory P_((const char *path));
GB_ERROR GB_save_in_home P_((GBDATA *gb, const char *path, const char *savetype));
GB_ERROR GB_save_as P_((GBDATA *gb, const char *path, const char *savetype));
GB_ERROR GB_delete_database P_((GB_CSTR filename));
GB_ERROR GB_save_quick_as P_((GBDATA *gb_main, char *path));
GB_ERROR GB_save_quick P_((GBDATA *gb, char *refpath));
void GB_disable_path P_((GBDATA *gbd, const char *path));

/* adcompr.c */
GB_BOOL GB_is_directory_compressed P_((GBDATA *gbd));

/* admalloc.c */
NOT4PERL void *GB_calloc P_((unsigned int nelem, unsigned int elsize));
char *GB_strdup P_((const char *p));
char *GB_strduplen P_((const char *p, unsigned len));
char *GB_strpartdup P_((const char *start, const char *end));
char *GB_strndup P_((const char *start, int len));
NOT4PERL void *GB_recalloc P_((void *ptr, unsigned int oelem, unsigned int nelem, unsigned int elsize));
void GB_memerr P_((void));

/* ad_load.c */
void GB_set_next_main_idx P_((long idx));
GBDATA *GB_login P_((const char *cpath, const char *opent, const char *user));
GBDATA *GB_open P_((const char *path, const char *opent));
void GB_set_verbose P_((void));

/* adTest.c */
const char *GB_get_type_name P_((GBDATA *gbd));
const char *GB_get_db_path P_((GBDATA *gbd));
void GB_dump_db_path P_((GBDATA *gbd));
void GB_dump P_((GBDATA *gbd));
void GB_dump_no_limit P_((GBDATA *gbd));
GB_ERROR GB_fix_database P_((GBDATA *gb_main));

/* adGene.c */
GB_BOOL GEN_is_genome_db P_((GBDATA *gb_main, int default_value));
GBDATA *GEN_findOrCreate_gene_data P_((GBDATA *gb_species));
GBDATA *GEN_find_gene_data P_((GBDATA *gb_species));
GBDATA *GEN_expect_gene_data P_((GBDATA *gb_species));
GBDATA *GEN_find_gene_rel_gene_data P_((GBDATA *gb_gene_data, const char *name));
GBDATA *GEN_find_gene P_((GBDATA *gb_species, const char *name));
GBDATA *GEN_create_nonexisting_gene_rel_gene_data P_((GBDATA *gb_gene_data, const char *name));
GBDATA *GEN_create_nonexisting_gene P_((GBDATA *gb_species, const char *name));
GBDATA *GEN_find_or_create_gene_rel_gene_data P_((GBDATA *gb_gene_data, const char *name));
GBDATA *GEN_find_or_create_gene P_((GBDATA *gb_species, const char *name));
GBDATA *GEN_first_gene P_((GBDATA *gb_species));
GBDATA *GEN_first_gene_rel_gene_data P_((GBDATA *gb_gene_data));
GBDATA *GEN_next_gene P_((GBDATA *gb_gene));
GBDATA *GEN_first_marked_gene P_((GBDATA *gb_species));
GBDATA *GEN_next_marked_gene P_((GBDATA *gb_gene));
struct GEN_position *GEN_new_position P_((int parts, GB_BOOL joinable));
void GEN_use_uncertainties P_((struct GEN_position *pos));
void GEN_free_position P_((struct GEN_position *pos));
struct GEN_position *GEN_read_position P_((GBDATA *gb_gene));
GB_ERROR GEN_write_position P_((GBDATA *gb_gene, const struct GEN_position *pos));
void GEN_sortAndMergeLocationParts P_((struct GEN_position *location));
const char *GEN_origin_organism P_((GBDATA *gb_pseudo));
const char *GEN_origin_gene P_((GBDATA *gb_pseudo));
GB_BOOL GEN_is_pseudo_gene_species P_((GBDATA *gb_species));
GB_ERROR GEN_organism_not_found P_((GBDATA *gb_pseudo));
GBDATA *GEN_read_pseudo_species_from_hash P_((GB_HASH *pseudo_hash, const char *organism_name, const char *gene_name));
void GEN_add_pseudo_species_to_hash P_((GBDATA *gb_pseudo, GB_HASH *pseudo_hash));
GB_HASH *GEN_create_pseudo_species_hash P_((GBDATA *gb_main, int additionalSize));
GBDATA *GEN_find_pseudo_species P_((GBDATA *gb_main, const char *organism_name, const char *gene_name, GB_HASH *pseudo_hash));
GBDATA *GEN_find_origin_organism P_((GBDATA *gb_pseudo, GB_HASH *organism_hash));
GBDATA *GEN_find_origin_gene P_((GBDATA *gb_pseudo, GB_HASH *organism_hash));
GBDATA *GEN_first_pseudo_species P_((GBDATA *gb_main));
GBDATA *GEN_first_pseudo_species_rel_species_data P_((GBDATA *gb_species_data));
GBDATA *GEN_next_pseudo_species P_((GBDATA *gb_species));
GBDATA *GEN_first_marked_pseudo_species P_((GBDATA *gb_main));
GBDATA *GEN_next_marked_pseudo_species P_((GBDATA *gb_species));
GB_BOOL GEN_is_organism P_((GBDATA *gb_species));
GBDATA *GEN_find_organism P_((GBDATA *gb_main, const char *name));
GBDATA *GEN_first_organism P_((GBDATA *gb_main));
GBDATA *GEN_next_organism P_((GBDATA *gb_organism));
long GEN_get_organism_count P_((GBDATA *gb_main));
GBDATA *GEN_first_marked_organism P_((GBDATA *gb_main));
GBDATA *GEN_next_marked_organism P_((GBDATA *gb_organism));
char *GEN_global_gene_identifier P_((GBDATA *gb_gene, GBDATA *gb_organism));

/* adtcp.c */
const char *GBS_scan_arb_tcp_param P_((const char *ipPort, const char *wantedParam));
const char *GBS_read_arb_tcp P_((const char *env));
const char *const *GBS_get_arb_tcp_entries P_((const char *matching));
const char *GBS_ptserver_logname P_((void));
void GBS_add_ptserver_logentry P_((const char *entry));
char *GBS_ptserver_id_to_choice P_((int i, int showBuild));

/* adhashtools.c */
long GBT_get_species_hash_size P_((GBDATA *gb_main));
void GBT_add_item_to_hash P_((GBDATA *gb_item, GB_HASH *item_hash));
GB_HASH *GBT_create_species_hash P_((GBDATA *gb_main));
GB_HASH *GBT_create_species_hash_sized P_((GBDATA *gb_main, long species_count));
GB_HASH *GBT_create_marked_species_hash P_((GBDATA *gb_main));
GB_HASH *GBT_create_SAI_hash P_((GBDATA *gb_main));
GB_HASH *GBT_create_organism_hash P_((GBDATA *gb_main));

/* adExperiment.c */
GBDATA *EXP_get_experiment_data P_((GBDATA *gb_species));
GBDATA *EXP_find_experiment_rel_exp_data P_((GBDATA *gb_experiment_data, const char *name));
GBDATA *EXP_find_experiment P_((GBDATA *gb_species, const char *name));
GBDATA *EXP_expect_experiment P_((GBDATA *gb_species, const char *name));
GBDATA *EXP_first_experiment_rel_exp_data P_((GBDATA *gb_experiment_data));
GBDATA *EXP_next_experiment P_((GBDATA *gb_experiment));
GBDATA *EXP_find_or_create_experiment_rel_exp_data P_((GBDATA *gb_experiment_data, const char *name));

#ifdef __cplusplus
}
#endif

#else
#error ad_prot.h included twice
#endif /* AD_PROT_H */
