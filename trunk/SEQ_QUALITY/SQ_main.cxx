#include <stdio.h>
#include <stdlib.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <aw_awars.hxx>

#include "seq_quality.h"
#include "SQ_functions.h"

extern GBDATA *gb_main;

// --------------------------------------------------------------------------------

#define AWAR_SQ_PERM "seq_quality/"     // saved in properties
#define AWAR_SQ_TEMP "tmp/seq_quality/" // not saved in properties

#define AWAR_SQ_TREE AWAR_SQ_TEMP "tree_name"

#define AWAR_SQ_WEIGHT_BASES     AWAR_SQ_PERM "weight_bases"
#define AWAR_SQ_WEIGHT_DEVIATION AWAR_SQ_PERM "weight_deviation"
#define AWAR_SQ_WEIGHT_HELIX     AWAR_SQ_PERM "weight_helix"
#define AWAR_SQ_WEIGHT_CONSENSUS AWAR_SQ_PERM "weight_consensus"
#define AWAR_SQ_WEIGHT_IUPAC     AWAR_SQ_PERM "weight_iupac"
#define AWAR_SQ_WEIGHT_GC        AWAR_SQ_PERM "weight_gc"
#define AWAR_SQ_MARK             AWAR_SQ_PERM "mark"


void SQ_create_awars(AW_root *aw_root, AW_default aw_def) {
    {
        char *default_tree = aw_root->awar(AWAR_TREE)->read_string();
        aw_root->awar_string(AWAR_SQ_TREE, default_tree, aw_def);
        free(default_tree);
    }
    aw_root->awar_int(AWAR_SQ_WEIGHT_BASES, 10, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_DEVIATION, 15, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_HELIX, 15, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_CONSENSUS, 50, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_IUPAC, 5, aw_def);
    aw_root->awar_int(AWAR_SQ_WEIGHT_GC, 5, aw_def);
    aw_root->awar_int(AWAR_SQ_MARK, 36, aw_def);
}

// --------------------------------------------------------------------------------

static void sq_calc_seq_quality_cb(AW_window *aww) {


    AW_root  *aw_root = aww->get_root();
    GB_ERROR  error   = 0;
    GBT_TREE *tree    = 0;

    {
        char *treename = aw_root->awar(AWAR_SQ_TREE)->read_string(); // contains "????" if no tree is selected
        if (treename && strcmp(treename, "????") != 0) {
            GB_push_transaction(gb_main);
            tree = GBT_read_tree(gb_main, treename, sizeof(GBT_TREE));
            if (tree){
		error = GBT_link_tree(tree,gb_main,GB_FALSE);
	    }
            else{
		aw_message(GBS_global_string("Cannot read tree '%s' -- group specific calculations skipped.\n   Treating all available sequences as one group!", treename));
	    }
            GB_pop_transaction(gb_main);
        }
        else aw_message("No tree selected -- group specific calculations skipped.");
        free(treename);
    }

    if (!error) {
        //error = SQ_reset_quality_calcstate(gb_main);
    }

    // if tree == 0 -> do basic quality calculations that are possible without tree information
    // otherwise    -> use all groups found in tree and compare sequences against the groups they are contained in

    if (!error) {
      //const char *option = "number_of_bases";// "consensus_conformity";

        /*
          "option" is variable which is passed to function "SQ_get_value()".
          SQ_get_value() returns the values that are stored in the specific containers used for alignment quality evaluation.
          Right now the options you have are:

          number_of_bases
          percent_of_bases
          diff_from_average
          number_of_helix
          number_of_weak_helix
          number_of_no_helix
          value_of_evaluation
	  consensus
	  evaluation
        */

        int weight_bases                = aw_root->awar(AWAR_SQ_WEIGHT_BASES)->read_int();
        int weight_diff_from_average    = aw_root->awar(AWAR_SQ_WEIGHT_DEVIATION)->read_int();
	int weight_helix                = aw_root->awar(AWAR_SQ_WEIGHT_HELIX)->read_int();
	int weight_consensus            = aw_root->awar(AWAR_SQ_WEIGHT_CONSENSUS)->read_int();
 	int weight_iupac                = aw_root->awar(AWAR_SQ_WEIGHT_IUPAC)->read_int();
 	int weight_gc                   = aw_root->awar(AWAR_SQ_WEIGHT_GC)->read_int();
	int mark                        = aw_root->awar(AWAR_SQ_MARK)->read_int();

        /*
          The "weight_..."  -values are passed to the function "SQ_evaluate()".
          SQ_evaluate() generates the final estimation for the quality of an alignment.
          It takes the values from the different containers, which are generated by the other functions, weights them
          and calculates a final value. The final value is stored in "value_of_evaluation" (see options).
          So, with the "weight_..."  -values one can customise how important a value stored in a contaier becomes
          for the final result.
        */

	if(tree==0){
	    SQ_GroupData* globalData = new SQ_GroupData_RNA;
	    SQ_count_nr_of_species(gb_main);
	    aw_openstatus("Calculating pass 1 of 2 ...");
	    SQ_pass1_no_tree(globalData, gb_main);
	    aw_closestatus();
	    aw_openstatus("Calculating pass 2 of 2 ...");
	    SQ_pass2_no_tree(globalData, gb_main);
	    aw_closestatus();
	    //int value = SQ_get_value_no_tree(gb_main, option);
	    //aw_message(GBS_global_string("Value in container %s : %i",option, value));
	    delete globalData;

	}
	else {
	    aw_openstatus("Calculating pass 1 of 2...");
            SQ_reset_counters(tree);
	    SQ_GroupData* globalData = new SQ_GroupData_RNA;
	    SQ_calc_and_apply_group_data(tree, gb_main, globalData);
	    aw_closestatus();
	    SQ_reset_counters(tree);
	    aw_openstatus("Calculating pass 2 of 2...");
	    SQ_calc_and_apply_group_data2(tree, gb_main, globalData);
	    SQ_evaluate(gb_main, weight_bases, weight_diff_from_average, weight_helix, weight_consensus, weight_iupac, weight_gc);
	    aw_closestatus();
	    SQ_reset_counters(tree);
	    aw_openstatus("Marking Sequences...");
	    SQ_count_nr_of_species(gb_main);
	    SQ_mark_species(gb_main, mark);
	    aw_closestatus();
	    delete globalData;
	}

    }

    if (error) {
        aw_message(error);
    }
}

// create window for sequence quality calculation (called only once)
AW_window *SQ_create_seq_quality_window(AW_root *aw_root, AW_CL) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "CALC_SEQ_QUALITY", "CALCULATE SEQUENCE QUALITY");
    aws->load_xfig("seq_quality.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"seq_quality.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("base");
    aws->create_input_field(AWAR_SQ_WEIGHT_BASES, 3);

    aws->at("deviation");
    aws->create_input_field(AWAR_SQ_WEIGHT_DEVIATION, 3);

    aws->at("no_helices");
    aws->create_input_field(AWAR_SQ_WEIGHT_HELIX, 3);

    aws->at("consensus");
    aws->create_input_field(AWAR_SQ_WEIGHT_CONSENSUS, 3);

    aws->at("iupac");
    aws->create_input_field(AWAR_SQ_WEIGHT_IUPAC, 3);

    aws->at("gc_proportion");
    aws->create_input_field(AWAR_SQ_WEIGHT_GC, 3);

    aws->at("mark");
    aws->create_input_field(AWAR_SQ_MARK, 3);

    aws->at("tree");
    awt_create_selection_list_on_trees(gb_main,(AW_window *)aws, AWAR_SQ_TREE);

    aws->at("go");
    aws->callback(sq_calc_seq_quality_cb);
    aws->highlight();
    aws->create_button("GO", "GO", "G");

    return aws;
}
