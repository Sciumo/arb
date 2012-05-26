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

ARB_ERROR enter_stage_1_build_tree(PT_main *, char *tname) __ATTR__USERESULT;
ARB_ERROR enter_stage_3_load_tree(PT_main *, const char *tname) __ATTR__USERESULT;

/* PT_debug.cxx */
void PT_dump_tree_statistics(void);
void PT_dump_POS_TREE_recursive(POS_TREE *IF_DEBUG (pt), const char *IF_DEBUG (prefix), FILE *IF_DEBUG (out));
void PT_dump_POS_TREE(POS_TREE *IF_DEBUG (node), FILE *IF_DEBUG (out));
void PT_dump_POS_TREE_to_file(const char *IF_DEBUG (dumpfile));

/* PT_etc.cxx */
void set_table_for_PT_N_mis(int ignored_Nmismatches, int when_less_than_Nmismatches);
void pt_export_error(PT_local *locs, const char *error);
const char *virt_name(PT_probematch *ml);
const char *virt_fullname(PT_probematch *ml);
char *ptpd_read_names(PT_local *locs, const char *names_list, const char *checksums, const char *&error);
bytestring *PT_unknown_names(PT_pdc *pdc);
void PT_init_base_string_counter(char *str, char initval, int size);
void PT_inc_base_string_count(char *str, char initval, char maxval, int size);

/* PT_family.cxx */
int find_family(PT_family *ffinder, bytestring *species);

/* PT_findEx.cxx */
int PT_find_exProb(PT_exProb *pep, int dummy_1x);

/* PT_io.cxx */
int compress_data(char *probestring);
ARB_ERROR probe_read_data_base(const char *name, bool readOnly) __ATTR__USERESULT;
int probe_compress_sequence(char *seq, int seqsize);
char *probe_read_alignment(int j, int *psize);
void probe_read_alignments(void);
void PT_build_species_hash(void);
long PT_abs_2_rel(long pos);

/* PT_main.cxx */
int server_shutdown(PT_main *pm, aisc_string passwd);
int broadcast(PT_main *main, int dummy_1x);
int ARB_main(int argc, const char *argv[]);

/* PT_match.cxx */
char *reverse_probe(char *probe);
int PT_complement(int base);
void complement_probe(char *probe);
int probe_match(PT_local *locs, aisc_string probestring);
char *get_match_overlay(PT_probematch *ml);
bytestring *match_string(PT_local *locs);
bytestring *MP_match_string(PT_local *locs);
bytestring *MP_all_species_string(PT_local *);
int MP_count_all_species(PT_local *);

/* PT_new_design.cxx */
double ptnd_check_split(PT_local *locs, char *probe, int pos, char ref);
char *get_design_info(PT_tprobes *tprobe);
char *get_design_hinfo(PT_tprobes *tprobe);
int PT_start_design(PT_pdc *pdc, int dummy_1x);

/* PT_prefixtree.cxx */
void PTM_finally_free_all_mem(void);
PTM2 *PT_init(void);
POS_TREE *PT_add_to_chain(POS_TREE *node, const DataLoc &loc);
POS_TREE *PT_change_leaf_to_node(POS_TREE *node);
POS_TREE *PT_leaf_to_chain(POS_TREE *node);
POS_TREE *PT_create_leaf(POS_TREE **pfather, PT_BASES base, const DataLoc &loc);
void PTD_clear_fathers(POS_TREE *node);
void PTD_put_longlong(FILE *out, ULONG i);
void PTD_put_int(FILE *out, ULONG i);
void PTD_put_short(FILE *out, ULONG i);
void PTD_debug_nodes(void);
long PTD_write_leafs_to_disk(FILE *out, POS_TREE *node, long pos, long *pnodepos, int *pblock, ARB_ERROR &error);
ARB_ERROR PTD_read_leafs_from_disk(const char *fname, POS_TREE **pnode) __ATTR__USERESULT;

/* probe_tree.h */
template <typename T >int PT_forwhole_chain(POS_TREE *node, T func);
template <typename T >int PT_withall_tips(POS_TREE *node, T func);

#else
#error pt_prototypes.h included twice
#endif /* PT_PROTOTYPES_H */
