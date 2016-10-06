/* This file is generated by aisc_mkpt.
 * Any changes you make here will be overwritten later!
 */

#ifndef TREECALLBACKS_HXX
#define TREECALLBACKS_HXX


/* TreeCallbacks.cxx */

#ifndef TREEDISPLAY_HXX
#include <TreeDisplay.hxx>
#endif

void nt_mode_event(UNFIXED, TREE_canvas *ntw, AWT_COMMAND_MODE mode);
void NT_mark_all_cb(UNFIXED, AWT_canvas *ntw, int mark_mode);
void NT_insert_mark_submenus(AW_window_menu_modes *awm, AWT_canvas *ntw, int insert_as_submenu);
void NT_expand_marked_cb(UNFIXED, AWT_canvas *ntw);
void NT_insert_collapse_submenu(AW_window_menu_modes *awm, AWT_canvas *ntw);
GB_ERROR NT_with_displayed_tree_do(AWT_canvas *ntw, bool (*displayed_tree_cb)(TreeNode *tree, GB_ERROR& error));
void NT_resort_tree_cb(UNFIXED, TREE_canvas *ntw, TreeOrder order);
void NT_reset_lzoom_cb(UNFIXED, TREE_canvas *ntw);
void NT_reset_pzoom_cb(UNFIXED, TREE_canvas *ntw);
void NT_set_tree_style(UNFIXED, TREE_canvas *ntw, AP_tree_display_type type);
void NT_remove_leafs(UNFIXED, TREE_canvas *ntw, AWT_RemoveType mode);
void NT_remove_bootstrap(UNFIXED, TREE_canvas *ntw);
void NT_toggle_bootstrap100(UNFIXED, TREE_canvas *ntw);
void NT_reset_branchlengths(UNFIXED, TREE_canvas *ntw);
void NT_multifurcate_tree(AWT_canvas *ntw, const TreeNode ::multifurc_limits& below);
void NT_move_boot_branch(UNFIXED, TREE_canvas *ntw, int direction);
void NT_scale_tree(UNFIXED, TREE_canvas *ntw);
void NT_jump_cb(UNFIXED, TREE_canvas *ntw, AP_tree_jump_type jumpType);
void TREE_auto_jump_cb(UNFIXED, TREE_canvas *ntw, bool tree_change);
void NT_reload_tree_event(AW_root *awr, TREE_canvas *ntw, bool unzoom_and_expose);
void TREE_recompute_cb(UNFIXED, AWT_canvas *ntw);
void TREE_GC_changed_cb(GcChange whatChanged, AWT_canvas *ntw);
void NT_reinit_treetype(UNFIXED, TREE_canvas *ntw);
void NT_remove_species_in_tree_from_hash(AP_tree *tree, GB_HASH *hash);

#else
#error TreeCallbacks.hxx included twice
#endif /* TREECALLBACKS_HXX */
