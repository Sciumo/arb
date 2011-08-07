// ============================================================= //
//                                                               //
//   File      : CT_part.hxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef CT_PART_HXX
#define CT_PART_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

typedef unsigned int PELEM;

typedef struct {
    PELEM   *p;
    GBT_LEN  len;               // Length between two knots
    int      percent;           // Count how often this partion appears
    char    *source;            // From which tree comes the partition
} PART;

void  part_init(int len);
void  part_print(PART *p);
PART *part_new();
PART *part_root();
void  part_setbit(PART *p, int pos);
int   son(PART *son, PART *father);
int   brothers(PART *p1, PART *p2); void part_invert(PART *p);
void  part_or(PART *s, PART *d);
void  part_copy(PART *s, PART *d);
void  part_standard(PART *p);
int   calc_index(PART *p);
void  part_free(PART *p);
int   part_cmp(PART *p1, PART *p2); int part_key(PART *p);
void  part_setlen(PART *p, GBT_LEN len);
void  part_setperc(PART *p, int perc);
void  part_addperc(PART *p, int perc);

#else
#error CT_part.hxx included twice
#endif // CT_PART_HXX
