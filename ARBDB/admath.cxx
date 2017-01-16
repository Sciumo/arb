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
#include <algorithm>

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include "gb_local.h"

double GB_log_fak(int n) {
    // returns log(n!)
    static int     cached = 0;
    static double *res    = 0;

    if (n<=1) return 0.0;       // log 1 = 0

    if (n > cached) {
        int new_size = std::max(n + 50, cached*4/3);
        gb_assert(new_size>(cached+1));
        ARB_realloc(res, new_size);

        res[0] = 0.0;
        for (int i=cached+1; i<new_size; i++) {
            res[i] = res[i-1]+log((double)i);
        }
        cached = new_size-1;
    }
    return res[n];
}

// -------------------------------------------
//      portable random number generation

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

void TEST_log_fak() {
    const double EPS = 0.000001;

    for (int loop = 0; loop<=1; ++loop) {
        TEST_ANNOTATE(GBS_global_string("loop=%i", loop));

        TEST_EXPECT_SIMILAR(GB_log_fak(0),    0.0,      EPS);
        TEST_EXPECT_SIMILAR(GB_log_fak(1),    0.0,      EPS);
        TEST_EXPECT_SIMILAR(GB_log_fak(2),    0.693147, EPS);
        TEST_EXPECT_SIMILAR(GB_log_fak(3),    1.791759, EPS);
        TEST_EXPECT_SIMILAR(GB_log_fak(10),  15.104413, EPS);
        TEST_EXPECT_SIMILAR(GB_log_fak(30),  74.658236, EPS);
        TEST_EXPECT_SIMILAR(GB_log_fak(70), 230.439044, EPS);
        TEST_EXPECT_SIMILAR(GB_log_fak(99), 359.134205, EPS);
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
