/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef PT_PROTOTYPES_H
#define PT_PROTOTYPES_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* PT_buildtree.cxx */

 class DataLoc;

long PTD_save_partial_tree(FILE *out, POS_TREE *node, char *partstring, int partsize, long pos, long *ppos, ARB_ERROR &error);
ARB_ERROR enter_stage_1_build_tree(PT_main *, char *tname) __ATTR__USERESULT;
ARB_ERROR enter_stage_3_load_tree(PT_main *, const char *tname) __ATTR__USERESULT;

/* PT_etc.cxx */
void set_table_for_PT_N_mis(int ignored_Nmismatches, int when_less_than_Nmismatches);
void pt_export_error(PT_local *locs, const char *error);
const char *virt_name(PT_probematch *ml);
const char *virt_fullname(PT_probematch *ml);
int *table_copy(int *mis_table, int length);
void table_add(int *mis_tabled, int *mis_tables, int length);
char *ptpd_read_names(PT_local *locs, const char *names_list, const char *checksums, const char *&error);
bytestring *PT_unknown_names(PT_pdc *pdc);
int get_clip_max_from_length(int length);
void PT_init_base_string_counter(char *str, char initval, int size);
void PT_inc_base_string_count(char *str, char initval, char maxval, int size);

/* PT_family.cxx */
int find_family(PT_family *ffinder, bytestring *species);

/* PT_findEx.cxx */
int PT_find_exProb(PT_exProb *pep, int dummy_1x);

/* PT_io.cxx */
int compress_data(char *probestring);
void PT_base_2_string(char *id_string, long len);
ARB_ERROR probe_read_data_base(const char *name, bool readOnly) __ATTR__USERESULT;
int probe_compress_sequence(char *seq, int seqsize);
char *probe_read_alignment(int j, int *psize);
void probe_read_alignments(void);
void PT_build_species_hash(void);
long PT_abs_2_rel(long pos);

/* PT_main.cxx */
ARB_ERROR pt_init_main_struct(PT_main *, const char *filename) __ATTR__USERESULT;
int server_shutdown(PT_main *pm, aisc_string passwd);
int broadcast(PT_main *main, int dummy_1x);
void PT_init_psg(void);
void PT_exit_psg(void);
void PT_exit(void);
GB_ERROR PT_init_map(void) __ATTR__USERESULT;
int ARB_main(int argc, const char *argv[]);

/* PT_match.cxx */
int read_names_and_pos(PT_local *locs, POS_TREE *pt);
int get_info_about_probe(PT_local *locs, char *probe, POS_TREE *pt, int mismatches, double wmismatches, int N_mismatches, int height);
void pt_sort_match_list(PT_local *locs);
char *reverse_probe(char *probe, int probe_length);
int PT_complement(int base);
void complement_probe(char *probe, int probe_length);
void pt_build_pos_to_weight(PT_MATCH_TYPE type, const char *sequence);
int probe_match(PT_local *locs, aisc_string probestring);
char *get_match_overlay(PT_probematch *ml);
bytestring *match_string(PT_local *locs);
bytestring *MP_match_string(PT_local *locs);
bytestring *MP_all_species_string(PT_local *);
int MP_count_all_species(PT_local *);

/* PT_new_design.cxx */
double ptnd_check_split(PT_pdc *pdc, char *probe, int pos, char ref);
char *get_design_info(PT_tprobes *tprobe);
char *get_design_hinfo(PT_tprobes *tprobe);
int PT_start_design(PT_pdc *pdc, int dummy_1x);

/* PT_prefixtree.cxx */
void PT_init_count_bits(void);
void PTM_add_alloc(void *ptr);
void PTM_finally_free_all_mem(void);
char *PTM_get_mem(int size);
int PTM_destroy_mem(void);
void PTM_free_mem(char *data, int size);
void PTM_debug_mem(void);
PTM2 *PT_init(void);
void PT_change_father(POS_TREE *father, POS_TREE *source, POS_TREE *dest);
POS_TREE *PT_add_to_chain(POS_TREE *node, const DataLoc &loc);
POS_TREE *PT_change_leaf_to_node(POS_TREE *node);
POS_TREE *PT_leaf_to_chain(POS_TREE *node);
POS_TREE *PT_create_leaf(POS_TREE **pfather, PT_BASES base, const DataLoc &loc);
void PTD_clear_fathers(POS_TREE *node);
void PTD_put_longlong(FILE *out, ULONG i);
void PTD_put_int(FILE *out, ULONG i);
void PTD_put_short(FILE *out, ULONG i);
void PTD_set_object_to_saved_status(POS_TREE *node, long pos, int size);
long PTD_write_tip_to_disk(FILE *out, POS_TREE *node, long pos);
int ptd_count_chain_entries(char *entry);
void ptd_set_chain_references(char *entry, char **entry_tab);
ARB_ERROR ptd_write_chain_entries(FILE *out, long *ppos, char **entry_tab, int n_entries, int mainapos) __ATTR__USERESULT;
long PTD_write_chain_to_disk(FILE *out, POS_TREE *node, long pos, ARB_ERROR &error);
void PTD_debug_nodes(void);
long PTD_write_node_to_disk(FILE *out, POS_TREE *node, long *r_poss, long pos);
long PTD_write_leafs_to_disk(FILE *out, POS_TREE *node, long pos, long *pnodepos, int *pblock, ARB_ERROR &error);
ARB_ERROR PTD_read_leafs_from_disk(const char *fname, POS_TREE **pnode) __ATTR__USERESULT;

/* PT_debug.cxx */
void PT_dump_tree_statistics(void);
void PT_dump_POS_TREE_recursive(POS_TREE *IF_DEBUG (pt), const char *IF_DEBUG (prefix));
void PT_dump_POS_TREE(POS_TREE *IF_DEBUG (node));

/* probe_tree.h */
template <typename T >int PT_forwhole_chain(POS_TREE *node, T func);
template <typename T >int PT_withall_tips(POS_TREE *node, T func);

#else
#error pt_prototypes.h included twice
#endif /* PT_PROTOTYPES_H */
