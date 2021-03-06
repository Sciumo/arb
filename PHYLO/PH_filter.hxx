// =========================================================== //
//                                                             //
//   File      : PH_filter.hxx                                 //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef PH_FILTER_HXX
#define PH_FILTER_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

class PH_filter : virtual Noncopyable {
    char *filter;         // 0 1
    long  filter_len;
    long  real_len;       // how many 1
    long  update;

public:


    PH_filter();
    ~PH_filter();

    char *init(long size);

    float *calculate_column_homology();
};

#else
#error PH_filter.hxx included twice
#endif

