// ============================================================= //
//                                                               //
//   File      : RegExpr.cxx                                     //
//   Purpose   : Wrapper for ARBDB regular expressions           //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2009   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "RegExpr.hxx"

#include <arb_match.h>
#include <arb_mem.h>

#include <regex.h>

using namespace std;

struct GBS_regex { regex_t compiled; }; // definition exists twice (see ../../ARBDB/admatch.c)

RegExpr::RegExpr(const std::string& expression_, bool ignore_case_) :
    expression(expression_),
    ignore_case(ignore_case_),
    comreg(0),
    matches(0),
    failure(0)
{}

RegExpr::~RegExpr() {
    if (comreg) GBS_free_regexpr(comreg);
    if (failure) delete failure;
    delete [] matches; 
}

bool RegExpr::compile() const {
    if (!comreg && !failure) {
        delete [] matches; matches = NULL;

        GB_ERROR error = 0;
        comreg = GBS_compile_regexpr(expression.c_str(), ignore_case ? GB_IGNORE_CASE : GB_MIND_CASE, &error);
        if (error) {
            failure = new string(error);
        }
        re_assert(contradicted(comreg, failure));
    }
    return comreg != 0;
}

void RegExpr::perform_match(const char *str, size_t offset) const {
    /* Searches for first match (and submatches) in 'str'
     *
     * sets member 'matches' to array of match + subexpression matches (heap-copy)
     * or to NULL if nothing matched
     *
     * If 'offset' > 0, then str is searched from position 'offset'.
     * In this case it is assumed, that we are not at line start!
     */

    delete [] matches; matches = NULL;

    size_t      subs      = subexpr_count();
    regmatch_t *possMatch = ARB_alloc<regmatch_t>(subs+1);
    int         eflags    = offset ? REG_NOTBOL : 0;
    int         res       = regexec(&comreg->compiled, str+offset, subs+1, possMatch, eflags);

    if (res != REG_NOMATCH) {
        matches  = new RegMatch[subs+1];
        for (size_t s = 0; s <= subs; s++) {
            if (possMatch[s].rm_so != -1) { // real match
                matches[s] = RegMatch(possMatch[s].rm_so+offset, possMatch[s].rm_eo+offset);
            }
        }
        re_assert(matches[0].didMatch()); // complete match has to be found
    }
    free(possMatch);
}

const RegMatch *RegExpr::match(const std::string& versus, size_t offset) const {
    if (!comreg) {
        // lazy compilation
        if (!compile()) return NULL;
    }
    perform_match(versus.c_str(), offset);
    return (matches && matches[0].didMatch()) ? &matches[0] : NULL;
}

size_t RegExpr::subexpr_count() const {
    if (!comreg) {
        // lazy compilation
        if (!compile()) return 0;
    }
    return comreg->compiled.re_nsub;
}

const RegMatch *RegExpr::subexpr_match(size_t subnr) const {
    // get subexpression match from last 'match()'
    // (or NULL if subexpression 'subnr' did not match)
    //
    // 'subnr' is in range [1..subexpr_count()]

    const RegMatch *result = 0;
    if (matches) {
        size_t subs = subexpr_count();
        re_assert(subnr >= 1 && subnr <= subs); // illegal subexpression index
        if (subnr >= 1 && subnr <= subs) {
            if (matches[subnr].didMatch()) result = &matches[subnr];
        }
    }
    return result;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static arb_test::match_expectation got_a_match(const RegExpr& exp, const RegMatch *match) {
    using namespace  arb_test;
    const string    *expression_failed = exp.has_failed();

    expectation_group expected(that(match).does_differ_from_NULL());
    expected.add(that(expression_failed).is_equal_to_NULL());
    if (expression_failed) {
        const char *expression_failed_msg = expression_failed->c_str();
        expected.add(that(expression_failed_msg).is_equal_to("(unwanted)"));
    }
    return all().ofgroup(expected);
}
static arb_test::match_expectation got_no_match(const RegExpr& exp, const RegMatch *match) {
    using namespace  arb_test;
    const string    *expression_failed = exp.has_failed();

    expectation_group expected(that(match).is_equal_to_NULL());
    expected.add(that(expression_failed).is_equal_to_NULL());
    if (expression_failed) {
        const char *expression_failed_msg = expression_failed->c_str();
        expected.add(that(expression_failed_msg).is_equal_to("(unwanted)"));
    }
    return all().ofgroup(expected);
}
static arb_test::match_expectation fails_to_compile(const RegExpr& exp,const char *error) {
    using namespace  arb_test;
    const string    *expression_failed = exp.has_failed();

    expectation_group expected(that(expression_failed).does_differ_from_NULL());
    if (expression_failed) {
        const char *expression_failed_msg = expression_failed->c_str();
        expected.add(that(expression_failed_msg).is_equal_to(error));
    }
    return all().ofgroup(expected);
}

#define TEST_REGEX_FAILS_TO_COMPILE(regexpr,error) do{  \
        RegExpr exp(regexpr, false);                    \
        TEST_EXPECTATION(fails_to_compile(exp,error));  \
    }while(0)

#define TEST_REGEX_MATCHES(str,regexpr,igCase,exp_match) do {           \
        RegExpr exp(regexpr, igCase);                                   \
        const RegMatch *match = exp.match(str);                         \
        TEST_EXPECTATION(got_a_match(exp,match));                       \
        TEST_EXPECT_EQUAL(match->extract(str).c_str(), exp_match);      \
    } while(0)

#define TEST_REGEX_DOESNT_MATCH(str,regexpr,igCase) do {        \
        RegExpr exp(regexpr, igCase);                           \
        const RegMatch *match = exp.match(str);                 \
        TEST_EXPECTATION(got_no_match(exp,match));              \
    } while(0)

#define TEST_REGEX_MATCHES_SUB1(str,regexpr,igCase,exp_match,exp_sub1match) do {        \
        RegExpr exp(regexpr, igCase);                                                   \
        const RegMatch *match = exp.match(str);                                         \
        TEST_EXPECTATION(got_a_match(exp,match));                                       \
        TEST_EXPECT_EQUAL(match->extract(str).c_str(), exp_match);                      \
        match = exp.subexpr_match(1);                                                   \
        TEST_REJECT_NULL(match);                                                        \
        TEST_EXPECT_EQUAL(match->extract(str).c_str(), exp_sub1match);                  \
    } while(0)

void TEST_regexpr() {
    TEST_REGEX_MATCHES("bla", "^bla$", false, "bla");

    TEST_REGEX_DOESNT_MATCH("3;1406", "^bla$", true);

    // disable the following section at will (arb does not depend on it)
#if defined(DARWIN)
    TEST_REGEX_FAILS_TO_COMPILE("*",              "repetition-operator operand invalid");
    TEST_REGEX_FAILS_TO_COMPILE("[[:fantasy:]]*", "invalid character class");
    TEST_REGEX_FAILS_TO_COMPILE("x{2",            "braces not balanced");
    TEST_REGEX_FAILS_TO_COMPILE("x{2-5}",         "invalid repetition count(s)");
    TEST_REGEX_FAILS_TO_COMPILE("x{5,2}",         "invalid repetition count(s)");
#else // !DARWIN
    TEST_REGEX_FAILS_TO_COMPILE("*",              "Invalid preceding regular expression");
    TEST_REGEX_FAILS_TO_COMPILE("[[:fantasy:]]*", "Invalid character class name");
    TEST_REGEX_FAILS_TO_COMPILE("x{2",            "Unmatched \\{");
    TEST_REGEX_FAILS_TO_COMPILE("x{2-5}",         "Invalid content of \\{\\}");
    TEST_REGEX_FAILS_TO_COMPILE("x{5,2}",         "Invalid content of \\{\\}");
#endif

    // RESULT_MODIFIED_OSX: regex isn't portable :/
#if defined(DARWIN)
    TEST_REGEX_FAILS_TO_COMPILE("^bla|", "empty (sub)expression");
#else // !DARWIN
    TEST_REGEX_MATCHES("3;1406", "^bla|", true, "");
#endif

    TEST_REGEX_MATCHES_SUB1("3;1406",                            "^[0-9]+;([0-9]+)$",                true,  "3;1406",        "1406");
    TEST_REGEX_MATCHES_SUB1(" Find  CAPITAL \t WoRD ",           "[[:space:]]+([A-Z]+)[[:space:]]+", false, "  CAPITAL \t ", "CAPITAL");
    TEST_REGEX_MATCHES_SUB1("--ajsd--aksjdh----alsdjkasldja---", "-+([a-z]{5,8})-+",                 true,  "--aksjdh----",  "aksjdh");
}
TEST_PUBLISH(TEST_regexpr);

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
