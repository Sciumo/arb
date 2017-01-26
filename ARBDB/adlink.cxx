// =============================================================== //
//                                                                 //
//   File      : adlink.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_main.h"
#include "gb_data.h"

GBDATA *GB_follow_link(GBDATA *gb_link) { // @@@ obsolete (will never resolve) -> elim
    char *s;
    const char *link;
    char c;
    GB_MAIN_TYPE *Main = GB_MAIN(gb_link);
    GBDATA *result;
    GB_Link_Follower lf;
    link = (char *)GB_read_link_pntr(gb_link);
    if (!link) return 0;
    s = const_cast<char*>(strchr(link, ':'));
    if (!s) {
        GB_export_errorf("Your link '%s' does not contain a ':' character", link);
        return 0;
    }
    c = *s;
    *s = 0;
    lf = (GB_Link_Follower)GBS_read_hash(Main->resolve_link_hash, link);
    *s = c;
    if (!lf) {
        GB_export_errorf("Your link tag '%s' is unknown to the system", link);
        return 0;
    }
    result = lf(GB_get_root(gb_link), gb_link, s+1);
    return result;
}

