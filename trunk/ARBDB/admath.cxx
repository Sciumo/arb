// =============================================================== //
//                                                                 //
//   File      : admath.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cmath>
#include <ctime>

#include "gb_local.h"

double GB_log_fak(int n) {
    // returns log(n!)
    static int     max_n = 0;
    static double *res   = 0;

    if (n<=1) return 0.0;       // log 1 = 0

    if (n >= max_n) {
        // @@@ should not recalculate already calculated results
        double sum = 0;

        max_n = n + 100;
        freeset(res, ARB_calloc<double>(max_n));

        for (int i=1; i<max_n; i++) {
            sum += log((double)i);
            res[i] = sum;
        }
    }
    return res[n];
}

// -------------------------------------------
//      portable random number generation

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

class RandomNumberGenerator : virtual Noncopyable {
    typedef boost::minstd_rand    base_generator_type;
    typedef boost::uniform_real<> distribution_type;
    typedef boost::variate_generator<base_generator_type&, distribution_type> generator_type;

    unsigned             usedSeed; // unused (for debugging purposes only)
    base_generator_type  base;
    generator_type      *generator;


public:
    RandomNumberGenerator(unsigned seed) :
        usedSeed(seed),
        base(seed),
        generator(new generator_type(base, distribution_type(0, 1)))
    {}
    ~RandomNumberGenerator() { delete generator; }

    void reseed(unsigned seed) {
        base.seed(seed);

        delete generator;
        generator = new generator_type(base, distribution_type(0, 1));
        usedSeed  = seed;

        get_rand(); // the 1st random number depends too much on the seed => drop it
    }

    double get_rand() {
        return (*generator)();
    }
};

static RandomNumberGenerator gen(time(0));

void GB_random_seed(unsigned seed) {
    /*! normally you should not use GB_random_seed.
     * Use it only to reproduce some pseudo-random result (e.g. in unittests)
     */
    gen.reseed(seed);
}

int GB_random(int range) {
    // produces a random number in range [0 .. range-1]
    return int(gen.get_rand()*((double)range));
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_random() {
    // test random numbers are portable (=reproducable on all tested systems)
    GB_random_seed(42);

    TEST_EXPECT_EQUAL(GB_random(1000), 571);
    TEST_EXPECT_EQUAL(GB_random(1000), 256);
    TEST_EXPECT_EQUAL(GB_random(1000), 447);
    TEST_EXPECT_EQUAL(GB_random(1000), 654);

    int rmax = -1;
    int rmin = 99999;
    for (int i = 0; i<2000; ++i) {
        int r = GB_random(1000);
        rmax  = std::max(r, rmax);
        rmin  = std::min(r, rmin);
    }

    TEST_EXPECT_EQUAL(rmin, 0);
    TEST_EXPECT_EQUAL(rmax, 999);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
