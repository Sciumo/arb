#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <iostream.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include "awt_seq_dna.hxx"
#include "awt_assert.hxx"

char	*AP_sequence_parsimony::table;


/*************************8


**************************/



/*****************************


******************************/

AP_sequence_parsimony::AP_sequence_parsimony(AP_tree_root *rooti) : AP_sequence(rooti)
{
	sequence = 0;
}

AP_sequence_parsimony::~AP_sequence_parsimony(void)
{
	if (sequence != 0) delete sequence;
	sequence = 0;
}

AP_sequence *AP_sequence_parsimony::dup(void)
{
	return (AP_sequence *)new AP_sequence_parsimony(root);
}

void AP_sequence_parsimony::build_table(void)
{
	table = (char *)AP_create_dna_to_ap_bases();
}



/************************************************************************
combine(const AP_sequence *lefts, const AP_sequence *rights)
set(char *isequence)

for wagner & fitch parsimony algorithm
is_set_flag is used by AP_tree_nlen::parsimony_rek()
*************************************************************************/

void AP_sequence_parsimony::set(char *isequence)
{
    register char *s,*d,*f1,c;
    register const uchar *simplify;
    sequence_len = root->filter->real_len;
    sequence = new char[sequence_len+1];
    memset(sequence,AP_N,(size_t)sequence_len+1);
    s = isequence;
    d = sequence;
    f1 = root->filter->filter_mask;
    simplify = root->filter->simplify;
    if (!table) this->build_table();
    if (root->filter->bootstrap){
        int iseqlen = strlen(isequence);
        int i;
        for (i=root->filter->real_len-1;i>=0;i--){
            int pos = root->filter->bootstrap[i]; // enthaelt zufallsfolge
            if (pos >= iseqlen) continue;
            c = s[pos];         // @@@ muss ueber mapping tabelle aufgefaltet werden 10/99
            d[i] = table[simplify[c]];
        }
    }else{
        register char *f = root->filter->filter_mask;
        register int i = root->filter->filter_len;
        while ( (c = (*s++)) ) {
            if (!i) break;
            i--;
            if (*(f++)) {
                *(d++) = table[simplify[c]];
            }
        }
    }
    update = AP_timer();
    is_set_flag = AP_TRUE;
    cashed_real_len = -1.0;
}

long global_combineCount;

AP_FLOAT AP_sequence_parsimony::combine( const AP_sequence *lefts, const AP_sequence *rights) {
// 	register char *p1,*p2,*p;
// 	register char c1,c2;
// 	register long result;

	const AP_sequence_parsimony *left = (const AP_sequence_parsimony *)lefts;
	const AP_sequence_parsimony *right = (const AP_sequence_parsimony *)rights;

	if (sequence == 0)	{
		sequence_len = root->filter->real_len;
		sequence = new char[root->filter->real_len +1];
	}

	const char *p1 = left->sequence;
	const char *p2 = right->sequence;
	char       *p  = sequence;

// 	result         = 0;
// 	char *end_seq1 = p1 + sequence_len;

    GB_UINT4 *w        = 0;
    char     *mutpsite = 0;

    if (mutation_per_site) { // count site specific mutations in mutation_per_site
        w        = root->weights->weights;
	    mutpsite = mutation_per_site;
    }
    else if (root->weights->dummy_weights) { // no weights, no mutation_per_site
        ;
    }
    else { // weighted (but don't count mutation_per_site)
        w = root->weights->weights;
    }

    long result = 0;

    for (long idx = 0; idx<sequence_len; ++idx) {
        char c1 = p1[idx];
        char c2 = p2[idx];

        awt_assert(c1 != 0); // not a base and not a gap -- what should it be ?
        awt_assert(c2 != 0);

        if ((c1&c2) == 0) {    // bases are distinct (that means we count a mismatch)
            p[idx] = c1|c2;     // mix distinct bases

            if (p[idx]&AP_S) { // contains a gap
                if (idx>0 && (p[idx-1]&AP_S)) { // last position also contained gap
                    if ((c1 == AP_S && p1[idx-1] == AP_S) ||
                        (c2 == AP_S && p2[idx-1] == AP_S))
                    {
                        continue; // skip multiple gaps
                    }
                }
            }

            if (mutpsite) mutpsite[idx]++; // count mutations per site (unweighted)
            result += w ? w[idx] : 1; // count weighted or simple
        }
        else {
            p[idx] = c1&c2; // store what's common for both bases
        }

        awt_assert(p[idx] != 0);
    }

// 	if (this->mutation_per_site){
// 	    register GB_UINT4 *w;
// 	    w = root->weights->weights;
// 	    char *mutpsite = mutation_per_site;
// 	    while ( p1 < end_seq1 ) {
//             c1 = *(p1);
//             c2 = *(p2);
//             p1++; p2++;
//             if ( !(c1&c2)) {
//                 *(p++) = c1 | c2;
//                 mutpsite[0]++; // count one mutation
//                 result += *w; // count one mutation (weighted)
//             }else{
//                 *(p++) = c1&c2;
//             }
//             w++;
//             mutpsite++;
// 	    }
// 	}else 	if (root->weights->dummy_weights){
// 	    while ( p1 < end_seq1 ) {
//             c1 = *(p1);
//             c2 = *(p2);
//             p1++; p2++;
//             if ( !(c1&c2)) {
//                 *(p++) = c1 | c2;
//                 result += 1; // count one mutation
//             }else{
//                 *(p++) = c1&c2;
//             }
// 	    }
// 	}else{
// 	    register GB_UINT4 *w;
// 	    w = root->weights->weights;
// 	    while ( p1 < end_seq1 ) {
//             c1 = *(p1);
//             c2 = *(p2);
//             p1++; p2++;
//             if ( !(c1&c2)) {
//                 *(p++) = c1 | c2;
//                 result += *(w++); // count one mutation (weighted)
//             }else{
//                 *(p++) = c1&c2;
//                 w++;
//             }
// 	    }
// 	}

	global_combineCount++;
	this->is_set_flag = AP_TRUE;
	cashed_real_len = -1.0;
	return (AP_FLOAT)result;
}


AP_FLOAT AP_sequence_parsimony::real_len(void)	// count all bases
{
	if (!sequence) return -1.0;
	if (cashed_real_len>=0.0) return cashed_real_len;
	char hits[256];
	register long sum,i;
	register char *p;
	register GB_UINT4 *w;
	sum = 0;
	p = sequence;
	for (i=0;i<256;i++){	// count ambigous characters half
        hits[i] = 1;
	}
	hits[AP_A] = 2;		// real characters full
	hits[AP_C] = 2;
	hits[AP_G] = 2;
	hits[AP_T] = 2;
	hits[AP_S] = 0;		// count no gaps
	hits[AP_N] = 0;		// no Ns
	w = root->weights->weights;

	for ( i = sequence_len; i ;i-- ) {	// all but no gaps
		sum += hits[*(p++)] * *(w++);
	}
	cashed_real_len = sum * .5;
	return sum * .5;
}
