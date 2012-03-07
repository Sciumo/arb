// =============================================================== //
//                                                                 //
//   File      : di_matr.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_MATR_HXX
#define DI_MATR_HXX

#ifndef AP_PRO_A_NUCS_HXX
#include <AP_pro_a_nucs.hxx>
#endif
#ifndef AP_TREE_HXX
#include <AP_Tree.hxx>
#endif
#ifndef AP_MATRIX_HXX
#include <AP_matrix.hxx>
#endif

#define di_assert(cond) arb_assert(cond)

#define AWAR_DIST_PREFIX           "dist/"
#define AWAR_DIST_CORR_TRANS       AWAR_DIST_PREFIX "correction/trans"
#define AWAR_DIST_SAVE_MATRIX_BASE "tmp/" AWAR_DIST_PREFIX "save_matrix"

#define AWAR_DIST_DIST_PREFIX AWAR_DIST_PREFIX "dist/"
#define AWAR_DIST_MIN_DIST    AWAR_DIST_DIST_PREFIX "lower"
#define AWAR_DIST_MAX_DIST    AWAR_DIST_DIST_PREFIX "upper"


enum DI_TRANSFORMATION {
    DI_TRANSFORMATION_NONE,
    DI_TRANSFORMATION_SIMILARITY,
    DI_TRANSFORMATION_JUKES_CANTOR,
    DI_TRANSFORMATION_FELSENSTEIN,

    DI_TRANSFORMATION_PAM,
    DI_TRANSFORMATION_CATEGORIES_HALL,
    DI_TRANSFORMATION_CATEGORIES_BARKER,
    DI_TRANSFORMATION_CATEGORIES_CHEMICAL,

    DI_TRANSFORMATION_HAESCH,
    DI_TRANSFORMATION_KIMURA,
    DI_TRANSFORMATION_OLSEN,
    DI_TRANSFORMATION_FELSENSTEIN_VOIGT,
    DI_TRANSFORMATION_OLSEN_VOIGT,
    DI_TRANSFORMATION_ML
};

enum DI_MATRIX_TYPE {
    DI_MATRIX_FULL,
    DI_MATRIX_COMPRESSED
};

class DI_MATRIX;
class AW_root;
class AP_sequence_parsimony;
class AP_sequence_simple_protein;

class DI_ENTRY : virtual Noncopyable {
public:
    DI_ENTRY(GBDATA *gbd, class DI_MATRIX *phmatri);
    DI_ENTRY(char *namei, class DI_MATRIX *phmatri);
    ~DI_ENTRY();

    DI_MATRIX                  *phmatrix;
    AP_sequence                *sequence;
    AP_sequence_parsimony      *sequence_parsimony; // if exist ok
    AP_sequence_simple_protein *sequence_protein;
    long                        seq_len;
    char                       *name;
    char                       *full_name;
    AP_FLOAT                    gc_bias;
    int                         group_nr;           // species belongs to group number xxxx
};

typedef long DI_MUT_MATR[AP_MAX][AP_MAX];

enum DI_SAVE_TYPE {
    DI_SAVE_PHYLIP_COMP,
    DI_SAVE_READABLE,
    DI_SAVE_TABBED
};

class BI_helix;

enum LoadWhat { DI_LOAD_ALL, DI_LOAD_MARKED, DI_LOAD_LIST };

typedef void (*DI_MATRIX_CB)();

class DI_MATRIX : virtual Noncopyable {
    friend class DI_ENTRY;

    GBDATA  *gb_species_data;
    long     seq_len;
    char     cancel_columns[256];
    AW_root *aw_root;                               // only link
    long     entries_mem_size;
    AliView *aliview;

public:
    bool             is_AA;
    DI_ENTRY       **entries;
    long             nentries;
    AP_smatrix      *matrix;
    DI_MATRIX_TYPE   matrix_type;

    DI_MATRIX(const AliView& aliview, AW_root *awr);
    ~DI_MATRIX();

    GBDATA *get_gb_main() const { return aliview->get_gb_main(); }
    const char *get_aliname() const { return aliview->get_aliname(); }
    const AliView *get_aliview() const { return aliview; }

    char *load(LoadWhat what, GB_CSTR sort_tree_name, bool show_warnings, GBDATA **species_list);
    char *unload();
    const char *save(char *filename, enum DI_SAVE_TYPE type);

    void    clear(DI_MUT_MATR &hits);
    void    make_sym(DI_MUT_MATR &hits);
    void    rate_write(DI_MUT_MATR &hits, FILE *out);
    long    *create_helix_filter(BI_helix *helix, const AP_filter *filter);
    // 0 non helix 1 helix; compressed filter
    GB_ERROR calculate_rates(DI_MUT_MATR &hrates, DI_MUT_MATR &nrates, DI_MUT_MATR &pairs, long *filter);
    GB_ERROR haeschoe(const char *path);
    double  corr(double dist, double b, double & sigma);
    GB_ERROR calculate(AW_root *awr, char *cancel, double alpha, DI_TRANSFORMATION transformation, bool *aborted_flag);
    char *calculate_overall_freqs(double rel_frequencies[AP_MAX], char *cancel_columns);
    GB_ERROR calculate_pro(DI_TRANSFORMATION transformation, bool *aborted_flag);
    void analyse();

    int   search_group(GBT_TREE *node, GB_HASH *hash, long *groupcnt, char *groupname, DI_ENTRY **groups);   // @@ OLIVER
    char *compress(GBT_TREE *tree);
};

class DI_GLOBAL_MATRIX : virtual Noncopyable {
    DI_MATRIX    *matrix;
    DI_MATRIX_CB  changed_cb;

    void announce_change() { if (changed_cb) changed_cb(); }

public:
    DI_GLOBAL_MATRIX() : matrix(NULL), changed_cb(NULL) {}
    ~DI_GLOBAL_MATRIX() { forget(); }

    DI_MATRIX *get() { return matrix; }
    void set(DI_MATRIX *new_global) { di_assert(!matrix); matrix = new_global; announce_change(); }
    void forget() { delete matrix; matrix = NULL; announce_change(); }
    void replaceBy(DI_MATRIX *new_global) { forget(); set(new_global); }

    bool exists() const { return matrix != NULL; }

    void set_changed_cb(DI_MATRIX_CB cb) {
        announce_change();
        changed_cb = cb;
    }

    DI_MATRIX *swap(DI_MATRIX *other) {
        DI_MATRIX *prev = matrix;
        matrix          = other;
        announce_change();
        return prev;
    }

    void forget_if_not_has_type(DI_MATRIX_TYPE wanted_type) {
        if (matrix && matrix->matrix_type != wanted_type) {
            forget();
        }
    }
};

extern DI_GLOBAL_MATRIX GLOBAL_MATRIX;

class WeightedFilter;
struct save_matrix_params {
    const char           *awar_base;
    const WeightedFilter *weighted_filter;
};

AW_window *DI_create_save_matrix_window(AW_root *aw_root, save_matrix_params *save_params);

#else
#error di_matr.hxx included twice
#endif // DI_MATR_HXX
