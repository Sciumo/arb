// #define FINDCORR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>

#include <servercntrl.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>

#include <aw_preset.hxx>

AW_HEADER_MAIN

AW_window *DI_create_matrix_window(AW_root *aw_root);
void       DI_create_matrix_variables(AW_root *aw_root, AW_default aw_def, AW_default db);
#ifdef FINDCORR
AW_window *bc_create_main_window(AW_root *awr);
void       bc_create_bc_variables(AW_root *awr, AW_default awd);
#endif

GBDATA *GLOBAL_gb_main; // global gb_main for arb_dist


static void DI_timer(AW_root *aw_root, AW_CL cl_gbmain, AW_CL cd2) {
    GBDATA *gb_main = reinterpret_cast<GBDATA*>(cl_gbmain);
    {
        GB_transaction ta(gb_main);
        GB_tell_server_dont_wait(gb_main); // trigger database callbacks
    }
    aw_root->add_timed_callback(500, DI_timer, cl_gbmain, cd2);
}

int main(int argc, char **argv)
{
    AW_root     *aw_root;
    AW_default   aw_default;
    AW_window   *aww;
    AWT_graphic *awd;

    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        fprintf(stderr,
                "Usage: arb_dist\n"
                "Is called from ARB.\n"
                );
        exit(-1);
    }

    aw_initstatus();
    aw_root    = new AW_root;
    aw_default = aw_root->open_default(".arb_prop/dist.arb");
    aw_root->init_variables(aw_default);
    aw_root->init_root("ARB_DIST", false);

    {
        struct arb_params *params;
        params  = arb_trace_argv(&argc, argv);
        if (argc==2) {
            freedup(params->db_server, argv[1]);
        }
        GLOBAL_gb_main = GB_open(params->db_server, "rw");
        if (!GLOBAL_gb_main) {
            aw_message(GB_await_error());
            exit(-1);
        }

#if defined(DEBUG)
        AWT_announce_db_to_browser(GLOBAL_gb_main, GBS_global_string("ARB-database (%s)", params->db_server));
#endif // DEBUG

        free_arb_params(params);
    }

    DI_create_matrix_variables(aw_root, aw_default, GLOBAL_gb_main);
#ifdef FINDCORR
    bc_create_bc_variables(aw_root, aw_default);
#endif
    ARB_init_global_awars(aw_root, aw_default, GLOBAL_gb_main);

    awd = (AWT_graphic *)0;
    aww = DI_create_matrix_window(aw_root);
    aww->show();

    AWT_install_cb_guards();
    
    aw_root->add_timed_callback(2000, DI_timer, AW_CL(GLOBAL_gb_main), 0);
    aw_root->main_loop();
}

void AD_map_viewer(GBDATA *dummy, AD_MAP_VIEWER_TYPE)
    {
    AWUSE(dummy);
}
