/*
 * Internal toolkit.
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
GBT_TREE *gbt_read_tree_rek P_((char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree, GB_ERROR *error));
int gbt_sum_leafs P_((GBT_TREE *tree));
GB_CSTR *gbt_fill_species_names P_((GB_CSTR *des, GBT_TREE *tree));
void gbt_export_tree_node_print_remove P_((char *str));
void gbt_export_tree_rek P_((GBT_TREE *tree, FILE *out));
GB_ERROR gbt_rename_tree_rek P_((GBT_TREE *tree, int tree_index));

/* adseqcompr.c */
char *gb_compress_seq_by_master P_((const char *master, int master_len, int master_index, GBQUARK q, const char *seq, long seq_len, long *memsize, int old_flag));
char *gb_compress_sequence_by_master P_((GBDATA *gbd, const char *master, int master_len, int master_index, GBQUARK q, const char *seq, int seq_len, long *memsize));
char *gb_uncompress_by_sequence P_((GBDATA *gbd, const char *ss, long size, GB_ERROR *error, long *new_size));

/* adtables.c */
GBDATA *gbt_table_link_follower P_((GBDATA *gb_main, GBDATA *gb_link, const char *link));

/* adChangeKey.c */
GB_ERROR gbt_set_type_of_changekey P_((GBDATA *gb_main, const char *field_name, GB_TYPES type, const char *change_key_path));

#ifdef __cplusplus
}
#endif
