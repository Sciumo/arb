/*
 * ARB toolkit.
 *
 * This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 *
 */

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


/* adtools.c */
char **GBT_get_alignment_names P_((GBDATA *gbd));
GB_ERROR GBT_check_alignment_name P_((const char *alignment_name));
GBDATA *GBT_create_alignment P_((GBDATA *gbd, const char *name, long len, long aligned, long security, const char *type));
NOT4PERL GB_ERROR GBT_check_alignment P_((GBDATA *gb_main, GBDATA *preset_alignment, GB_HASH *species_name_hash));
GB_ERROR GBT_rename_alignment P_((GBDATA *gbMain, const char *source, const char *dest, int copy, int dele));
GBDATA *GBT_find_or_create P_((GBDATA *Main, const char *key, long delete_level));
GB_ERROR GBT_check_data P_((GBDATA *Main, const char *alignment_name));
GB_ERROR GBT_check_lengths P_((GBDATA *Main, const char *alignment_name));
GB_ERROR GBT_format_alignment P_((GBDATA *Main, const char *alignment_name));
GB_ERROR GBT_insert_character P_((GBDATA *Main, char *alignment_name, long pos, long count, char *char_delete));
GB_ERROR GBT_delete_tree P_((GBT_TREE *tree));
GB_ERROR GBT_write_group_name P_((GBDATA *gb_group_name, const char *new_group_name));
GB_ERROR GBT_write_tree P_((GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree));
GB_ERROR GBT_write_plain_tree P_((GBDATA *gb_main, GBDATA *gb_tree, char *tree_name, GBT_TREE *tree));
GB_ERROR GBT_write_tree_rem P_((GBDATA *gb_main, const char *tree_name, const char *remark));
GBT_TREE *GBT_read_tree_and_size P_((GBDATA *gb_main, const char *tree_name, long structure_size, int *tree_size));
GBT_TREE *GBT_read_tree P_((GBDATA *gb_main, const char *tree_name, long structure_size));
GBT_TREE *GBT_read_plain_tree P_((GBDATA *gb_main, GBDATA *gb_ctree, long structure_size, GB_ERROR *error));
long GBT_count_nodes P_((GBT_TREE *tree));
GB_ERROR GBT_link_tree_using_species_hash P_((GBT_TREE *tree, GB_BOOL show_status, GB_HASH *species_hash, int *zombies, int *duplicates));
GB_ERROR GBT_link_tree P_((GBT_TREE *tree, GBDATA *gb_main, GB_BOOL show_status, int *zombies, int *duplicates));
void GBT_unlink_tree P_((GBT_TREE *tree));
void GBT_scale_tree P_((GBT_TREE *tree, double length_scale, double bootstrap_scale));
char *GBT_newick_comment P_((const char *comment, GB_BOOL escape));
GBT_TREE *GBT_load_tree P_((const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr));
GBDATA *GBT_get_tree P_((GBDATA *gb_main, const char *tree_name));
long GBT_size_of_tree P_((GBDATA *gb_main, const char *tree_name));
char *GBT_find_largest_tree P_((GBDATA *gb_main));
char *GBT_find_latest_tree P_((GBDATA *gb_main));
const char *GBT_tree_info_string P_((GBDATA *gb_main, const char *tree_name, int maxTreeNameLen));
GB_ERROR GBT_check_tree_name P_((const char *tree_name));
char **GBT_get_tree_names_and_count P_((GBDATA *Main, int *countPtr));
char **GBT_get_tree_names P_((GBDATA *Main));
char *GBT_get_next_tree_name P_((GBDATA *gb_main, const char *tree));
void GBT_free_names P_((char **names));
GB_CSTR *GBT_get_species_names_of_tree P_((GBT_TREE *tree));
char *GBT_existing_tree P_((GBDATA *Main, const char *tree));
GB_ERROR GBT_export_tree P_((GBDATA *gb_main, FILE *out, GBT_TREE *tree, GB_BOOL triple_root));
GBDATA *GBT_create_item P_((GBDATA *gb_item_data, const char *itemname, const char *name));
GBDATA *GBT_create_species_rel_species_data P_((GBDATA *gb_species_data, const char *name));
GBDATA *GBT_create_species P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_create_SAI P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_add_data P_((GBDATA *species, const char *ali_name, const char *key, GB_TYPES type));
GB_ERROR GBT_write_sequence P_((GBDATA *gb_data, const char *ali_name, long ali_len, const char *sequence));
GBDATA *GBT_gen_accession_number P_((GBDATA *gb_species, const char *ali_name));
int GBT_is_partial P_((GBDATA *gb_species, int default_value, int define_if_undef));
GBDATA *GBT_get_species_data P_((GBDATA *gb_main));
GBDATA *GBT_first_marked_species_rel_species_data P_((GBDATA *gb_species_data));
GBDATA *GBT_first_marked_species P_((GBDATA *gb_main));
GBDATA *GBT_next_marked_species P_((GBDATA *gb_species));
GBDATA *GBT_first_species_rel_species_data P_((GBDATA *gb_species_data));
GBDATA *GBT_first_species P_((GBDATA *gb_main));
GBDATA *GBT_next_species P_((GBDATA *gb_species));
GBDATA *GBT_find_species_rel_species_data P_((GBDATA *gb_species_data, const char *name));
GBDATA *GBT_find_species P_((GBDATA *gb_main, const char *name));
long GBT_get_item_count P_((GBDATA *gb_main, const char *item_container_name));
long GBT_get_species_count P_((GBDATA *gb_main));
long GBT_get_SAI_count P_((GBDATA *gb_main));
GBDATA *GBT_first_marked_extended P_((GBDATA *gb_extended_data));
GBDATA *GBT_next_marked_extended P_((GBDATA *gb_extended));
GBDATA *GBT_first_SAI P_((GBDATA *gb_main));
GBDATA *GBT_next_SAI P_((GBDATA *gb_extended));
GBDATA *GBT_find_SAI P_((GBDATA *gb_main, const char *name));
GBDATA *GBT_first_SAI_rel_exdata P_((GBDATA *gb_extended_data));
GBDATA *GBT_find_SAI_rel_exdata P_((GBDATA *gb_extended_data, const char *name));
char *GBT_create_unique_species_name P_((GBDATA *gb_main, const char *default_name));
void GBT_mark_all P_((GBDATA *gb_main, int flag));
void GBT_mark_all_that P_((GBDATA *gb_main, int flag, int (*condition )(GBDATA *, void *), void *cd));
long GBT_count_marked_species P_((GBDATA *gb_main));
char *GBT_store_marked_species P_((GBDATA *gb_main, int unmark_all));
NOT4PERL GB_ERROR GBT_with_stored_species P_((GBDATA *gb_main, const char *stored, species_callback doit, int *clientdata));
GB_ERROR GBT_restore_marked_species P_((GBDATA *gb_main, const char *stored_marked));
GBDATA *GBT_read_sequence P_((GBDATA *gb_species, const char *aliname));
GB_CSTR GBT_read_name P_((GBDATA *gb_item));
char *GBT_get_default_alignment P_((GBDATA *gb_main));
GB_ERROR GBT_set_default_alignment P_((GBDATA *gb_main, const char *alignment_name));
char *GBT_get_default_helix P_((GBDATA *gb_main));
char *GBT_get_default_helix_nr P_((GBDATA *gb_main));
char *GBT_get_default_ref P_((GBDATA *gb_main));
GBDATA *GBT_get_alignment P_((GBDATA *gb_main, const char *aliname));
long GBT_get_alignment_len P_((GBDATA *gb_main, const char *aliname));
GB_ERROR GBT_set_alignment_len P_((GBDATA *gb_main, const char *aliname, long new_len));
int GBT_get_alignment_aligned P_((GBDATA *gb_main, const char *aliname));
char *GBT_get_alignment_type_string P_((GBDATA *gb_main, const char *aliname));
GB_alignment_type GBT_get_alignment_type P_((GBDATA *gb_main, const char *aliname));
GB_BOOL GBT_is_alignment_protein P_((GBDATA *gb_main, const char *alignment_name));
GB_ERROR GBT_check_arb_file P_((const char *name));
char **GBT_scan_db P_((GBDATA *gbd, const char *datapath));
void GBT_install_message_handler P_((GBDATA *gb_main));
void GBT_message P_((GBDATA *gb_main, const char *msg));
GB_ERROR GBT_begin_rename_session P_((GBDATA *gb_main, int all_flag));
GB_ERROR GBT_rename_species P_((const char *oldname, const char *newname, GB_BOOL ignore_protection));
GB_ERROR GBT_abort_rename_session P_((void));
GB_ERROR GBT_commit_rename_session P_((int (*show_status )(double gauge ), int (*show_status_text )(const char *)));
GBDATA **GBT_gen_species_array P_((GBDATA *gb_main, long *pspeccnt));
char *GBT_read_string P_((GBDATA *gb_container, const char *fieldpath));
char *GBT_read_as_string P_((GBDATA *gb_container, const char *fieldpath));
const char *GBT_read_char_pntr P_((GBDATA *gb_container, const char *fieldpath));
NOT4PERL long *GBT_read_int P_((GBDATA *gb_container, const char *fieldpath));
NOT4PERL double *GBT_read_float P_((GBDATA *gb_container, const char *fieldpath));
char *GBT_readOrCreate_string P_((GBDATA *gb_container, const char *fieldpath, const char *default_value));
const char *GBT_readOrCreate_char_pntr P_((GBDATA *gb_container, const char *fieldpath, const char *default_value));
NOT4PERL long *GBT_readOrCreate_int P_((GBDATA *gb_container, const char *fieldpath, long default_value));
NOT4PERL double *GBT_readOrCreate_float P_((GBDATA *gb_container, const char *fieldpath, double default_value));
GB_ERROR GBT_write_string P_((GBDATA *gb_container, const char *fieldpath, const char *content));
GB_ERROR GBT_write_int P_((GBDATA *gb_container, const char *fieldpath, long content));
GB_ERROR GBT_write_float P_((GBDATA *gb_container, const char *fieldpath, double content));
GBDATA *GB_test_link_follower P_((GBDATA *gb_main, GBDATA *gb_link, const char *link));
GBDATA *GBT_open P_((const char *path, const char *opent, const char *disabled_path));
GB_ERROR GBT_remote_action P_((GBDATA *gb_main, const char *application, const char *action_name));
GB_ERROR GBT_remote_awar P_((GBDATA *gb_main, const char *application, const char *awar_name, const char *value));
const char *GBT_remote_read_awar P_((GBDATA *gb_main, const char *application, const char *awar_name));
const char *GBT_remote_touch_awar P_((GBDATA *gb_main, const char *application, const char *awar_name));
NOT4PERL GB_ERROR GBT_get_gene_positions P_((GBDATA *gb_gene, int whichPos, long *pos_begin, long *pos_end));
char *GBT_read_gene_sequence P_((GBDATA *gb_gene, GB_BOOL use_revComplement));
char *GB_generate_notification P_((GBDATA *gb_main, void (*cb )(const char *message, void *client_data ), const char *message, void *client_data));
GB_ERROR GB_remove_last_notification P_((GBDATA *gb_main));
GB_ERROR GB_notify P_((GBDATA *gb_main, int id, const char *message));

/* adseqcompr.c */
GB_ERROR GBT_compress_sequence_tree2 P_((GBDATA *gb_main, const char *tree_name, const char *ali_name));
void GBT_compression_test P_((void *dummy, GBDATA *gb_main));

/* adtables.c */
GB_ERROR GBT_install_table_link_follower P_((GBDATA *gb_main));
GBDATA *GBT_open_table P_((GBDATA *gb_table_root, const char *table_name, GB_BOOL read_only));
GBDATA *GBT_first_table P_((GBDATA *gb_main));
GBDATA *GBT_next_table P_((GBDATA *gb_table));
GBDATA *GBT_first_table_entry P_((GBDATA *gb_table));
GBDATA *GBT_first_marked_table_entry P_((GBDATA *gb_table));
GBDATA *GBT_next_table_entry P_((GBDATA *gb_table_entry));
GBDATA *GBT_next_marked_table_entry P_((GBDATA *gb_table_entry));
GBDATA *GBT_find_table_entry P_((GBDATA *gb_table, const char *id));
GBDATA *GBT_open_table_entry P_((GBDATA *gb_table, const char *id));
GBDATA *GBT_first_table_field P_((GBDATA *gb_table));
GBDATA *GBT_first_marked_table_field P_((GBDATA *gb_table));
GBDATA *GBT_next_table_field P_((GBDATA *gb_table_field));
GBDATA *GBT_next_marked_table_field P_((GBDATA *gb_table_field));
GBDATA *GBT_find_table_field P_((GBDATA *gb_table, const char *id));
GB_TYPES GBT_get_type_of_table_entry_field P_((GBDATA *gb_table, const char *fieldname));
GB_ERROR GBT_savely_write_table_entry_field P_((GBDATA *gb_table, GBDATA *gb_entry, const char *fieldname, const char *value_in_ascii_format));
GBDATA *GBT_open_table_field P_((GBDATA *gb_table, const char *fieldname, GB_TYPES type_of_field));

/* adRevCompl.c */
char GBT_complementNucleotide P_((char c, char T_or_U));
char *GBT_reverseNucSequence P_((const char *s, int len));
char *GBT_complementNucSequence P_((const char *s, int len, char T_or_U));
GB_ERROR GBT_determine_T_or_U P_((GB_alignment_type alignment_type, char *T_or_U, const char *supposed_target));
void GBT_reverseComplementNucSequence P_((char *seq, long length, char T_or_U));

#ifdef __cplusplus
}
#endif
