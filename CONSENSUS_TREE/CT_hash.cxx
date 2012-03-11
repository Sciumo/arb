// ============================================================= //
//                                                               //
//   File      : CT_hash.cxx                                     //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "CT_hash.hxx"
#include "CT_ntree.hxx"
#include "CT_mem.hxx"
#include "CT_ctree.hxx"

#include <arbdbt.h>

// Hashtabelle fuer parts

static int    max_part_percent = 0;
static HNODE *Hashlist[HASH_MAX];
static HNODE *Sortedlist     = NULL;

static void hash_free() {
    //! free Hashtable and Sortedlist
    int i;
    HNODE *hnp, *hnp_help;

    for (i=0; i< HASH_MAX; i++) {
        hnp = Hashlist[i];
        while (hnp) {
            hnp_help = hnp->next;
            part_free(hnp->part);
            free(hnp);
            hnp = hnp_help;
        }
        Hashlist[i] = NULL;
    }

    hnp = Sortedlist;
    while (hnp) {
        hnp_help = hnp->next;
        part_free(hnp->part);
        free(hnp);
        hnp = hnp_help;
    }
    Sortedlist = NULL;
}

void hash_init() {
    //! initialize Hashtable
    arb_assert(!Sortedlist); // forgot to call hash_cleanup
    arb_assert(!max_part_percent);

    for (int i = 0; i<HASH_MAX; ++i) {
        Hashlist[i] = NULL;
    }
}

void hash_cleanup() {
    //! free old data
    hash_free();
    max_part_percent = 0;
}

PART *hash_getpart() {
    /*! return the first element (with the highest hitnumber) from the linear sorted
     * list and calculate percentaged appearance of this partition in all trees and
     * calculate the average pathlength.
     * The element is removed from the list afterwards
     */
    HNODE *hnp;
    PART *p;

    if (!Sortedlist) return NULL;

    hnp = Sortedlist;
    Sortedlist = hnp->next;
    p = hnp->part;
    free(hnp);
    p->len /= (float) p->percent;
    p->percent *= 10000;
    p->percent /= get_tree_count();

    return  p;
}

inline void track_max_part_percent(int pc) { if (pc>max_part_percent) max_part_percent = pc; }

void hash_insert(PART*& part, int weight) {
    /*! insert part in hashtable with weight (destroys/reassigns part)
     * @param part the one to insert, is destructed afterwards
     * @param weight  the weight of the part
     */

    part_standard(part);

    int key = part_key(part);
    key %= HASH_MAX;

    HNODE *hp = 0;
    if (Hashlist[key]) {
        for (hp=Hashlist[key]; hp; hp=hp->next) {
            if (parts_equal(hp->part, part)) break;
        }
    }

    part->percent  = weight;
    part->len     *= weight;

    if (hp) {
        hp->part->percent += part->percent;
        hp->part->len     += part->len;
        part_free(part);
    }
    else {
        hp            = (HNODE *) getmem(sizeof(HNODE));
        hp->part      = part;
        hp->next      = Hashlist[key];
        Hashlist[key] = hp;
    }

    part = NULL;
    track_max_part_percent(hp->part->percent);
}


void build_sorted_list() {
    /*! sort the current hash list in a linear sorted list, the current hash
     * is empty afterwards. I use a simple trick speed up the function:
     * build for every hitnumber one list and put all elements with that hitnumber
     * in it. After that i connect the list together, what results in a sorted list
     */

    arb_assert(!Sortedlist);
    
    HNODE *hnp, *hnp_help;
    HNODE **heads, **tails, *head, *tail;
    int i, idx;

    heads = (HNODE **) getmem(max_part_percent*sizeof(HNODE *));
    tails = (HNODE**)  getmem(max_part_percent*sizeof(HNODE *));

    // build one list for each countvalue

    for (i=0; i< HASH_MAX; i++) {
        hnp = Hashlist[i];
        while (hnp) {
            hnp_help = hnp->next;
            idx = hnp->part->percent-1;
            if (heads[idx]) {
                hnp->next = heads[idx];
                heads[idx] = hnp;
            }
            else {
                heads[idx] = tails[idx] = hnp;
                hnp->next = NULL;
            }

            hnp = hnp_help;
        }
        Hashlist[i] = NULL;
    }

    head = NULL;
    tail = NULL;
    // concatenate lists
    for (i=max_part_percent-1; i>=0; i--) {
        if (heads[i]) {
            if (!head) {
                head = heads[i];
                tail = tails[i];
            }
            else {
                tail->next = heads[i];
                tail = tails[i];
            }
        }
    }

    free(heads);
    free(tails);

    Sortedlist = head;
}

