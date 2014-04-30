/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef GDE_PROTO_H
#define GDE_PROTO_H

/* define ARB attributes: */
#ifndef ATTRIBUTES_H
# include <attributes.h>
#endif


/* GDE.cxx */

#ifndef GDE_MENU_H
#include "GDE_menu.h"
#endif

char *GDE_makeawarname(GmenuItem *gmenuitem, long i);
char *GDE_maketmpawarname(GmenuItem *gmenuitem, long i);
void GDE_load_menu(AW_window *awm, AW_active dummy_1x, const char *menulabel);
GB_ERROR GDE_create_var(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main, GDE_get_sequences_cb get_sequences, gde_window_type window_type, AW_CL client_data);

/* GDE_FileIO.cxx */
void Regroup(NA_Alignment *alignment);
char *Calloc(int count, int size);
char *Realloc(char *block, int size);
void Cfree(char *block);
void LoadData(char *filen);
void AppendNA(NA_Base *buffer, int len, NA_Sequence *seq);
void Ascii2NA(char *buffer, int len, int matrix[16]);
int WriteNA_Flat(NA_Alignment *aln, char *filename, int method);
void Warning(const char *s);
void InitNASeq(NA_Sequence *seq, int type);
void NormalizeOffset(NA_Alignment *aln);

/* GDE_Genbank.cxx */
GB_ERROR ReadGen(char *filename, NA_Alignment *dataset);
int WriteGen(NA_Alignment *aln, char *filename, int method);
void SetTime(void *b);

/* GDE_HGLfile.cxx */
int WriteGDE(NA_Alignment *aln, char *filename, int method);
char *uniqueID(void);

/* GDE_ParseMenu.cxx */
GB_ERROR LoadMenus(void);
int Find(const char *target, const char *key);
int Find2(const char *target, const char *key);
void throwError(const char *msg) __ATTR__NORETURN;
void splitEntry(const char *input, char *head, char *tail);

/* GDE_arbdb_io.cxx */

#ifndef GDE_EXTGLOB_H
#include "GDE_extglob.h"
#endif

int ReadArbdb2(NA_Alignment *dataset, AP_filter *filter, GapCompression compress, bool cutoff_stop_codon, TypeInfo typeinfo) __ATTR__USERESULT;
int ReadArbdb(NA_Alignment *dataset, bool marked, AP_filter *filter, GapCompression compress, bool cutoff_stop_codon, TypeInfo typeinfo) __ATTR__USERESULT;
int getelem(NA_Sequence *a, int b);
void putelem(NA_Sequence *a, int b, NA_Base c);

/* GDE_event.cxx */
void GDE_startaction_cb(AW_window *aw, GmenuItem *gmenuitem, AW_CL dummy_1x);

#else
#error GDE_proto.h included twice
#endif /* GDE_PROTO_H */
