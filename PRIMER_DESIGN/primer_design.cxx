#include <cmath>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>

#include "primer_design.hxx"
#include "PRD_Design.hxx"
#include "PRD_SequenceIterator.hxx"

extern GBDATA *gb_main;

void create_primer_design_variables(AW_root *aw_root, AW_default aw_def, AW_default global) {
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_LEFT_MIN,		   0,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_LEFT_MAX,		 200,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_RIGHT_MIN,		1000,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_POS_RIGHT_MAX,		1200,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LENGTH_MIN,		  10,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_LENGTH_MAX,		  20,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_USE,		   0,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_MIN,		1050,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_DIST_MAX,		1200,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_GCRATIO_MIN,		  10,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_GCRATIO_MAX,		  50,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN,	  30,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX,	  80,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST,   0,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_EXPAND_IUPAC,		   1,  aw_def);
    aw_root->awar_int(AWAR_PRIMER_DESIGN_MAX_PAIRS,		  25,  aw_def);
    aw_root->awar_float(AWAR_PRIMER_DESIGN_GC_FACTOR,		   0.5, aw_def);
    aw_root->awar_float(AWAR_PRIMER_DESIGN_TEMP_FACTOR,		   0.5, aw_def);

    aw_root->awar_string(AWAR_PRIMER_TARGET_STRING, ""  ,    global);
}

static AW_window_simple *pdrw = 0;
static AW_selection_list *pdrw_id = 0;

void primer_design_create_result_window(AW_window *aww)
{
    if (pdrw){
        pdrw->show();
        return;
    }
    pdrw = new AW_window_simple;
    pdrw->init(aww->get_root(), "PRD_RESULT", "Primer Design RESULT", 0, 400);
    pdrw->load_xfig("pd_reslt.fig");

    pdrw->at("close");
    pdrw->callback((AW_CB0)AW_POPDOWN);
    pdrw->create_button("CLOSE", "CLOSE", "C");

    pdrw->at("help");
    pdrw->callback(AW_POPUP_HELP, (AW_CL)"primerdesignresult.hlp");
    pdrw->create_button("HELP", "HELP", "H");

    pdrw->at( "result" );
    pdrw_id = pdrw->create_selection_list( AWAR_PRIMER_TARGET_STRING, NULL, "", 40, 5 );
    pdrw->set_selection_list_suffix(pdrw_id, "primer");

    pdrw->at("save");
    pdrw->callback( AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)pdrw_id );
    pdrw->create_button("SAVE", "SAVE", "S");

    pdrw->at("print");
    pdrw->callback( create_print_box_for_selection_lists, (AW_CL)pdrw_id );
    pdrw->create_button("PRINT", "PRINT", "P");

    pdrw->show();
}
void primer_design_event(AW_window *aww) {
  AW_root         *root     = aww->get_root();
  GB_ERROR         error    = 0;
  char            *sequence = 0;
  PRD_Sequence_Pos length;

  {
    GB_transaction  dummy(gb_main);
    char           *selected_species = root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA         *gb_species       = GBT_find_species(gb_main,selected_species);

    if ( !gb_species ) {
      error = "you have to select a species!";
    }
    else {
      const char *alignment = GBT_get_default_alignment(gb_main);
      GBDATA     *gb_seq    = GBT_read_sequence(gb_species, alignment);

      sequence = GB_read_string(gb_seq);
      length = strlen(sequence);
    }
  }

  if ( !error ) {
    int dist_min = root->awar(AWAR_PRIMER_DESIGN_DIST_MIN)->read_int();
    int dist_max = root->awar(AWAR_PRIMER_DESIGN_DIST_MAX)->read_int();

    if (root->awar(AWAR_PRIMER_DESIGN_DIST_USE)->read_int() == 0) {
      dist_min = dist_max = -1;
    }

    PrimerDesign *PD = new PrimerDesign(sequence,
					Range(root->awar(AWAR_PRIMER_DESIGN_POS_LEFT_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_POS_LEFT_MAX)->read_int()),
					Range(root->awar(AWAR_PRIMER_DESIGN_POS_RIGHT_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_POS_RIGHT_MAX)->read_int()),
					Range(root->awar(AWAR_PRIMER_DESIGN_LENGTH_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_LENGTH_MAX)->read_int()),
					Range(dist_min, dist_max),
					Range(root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_GCRATIO_MAX)->read_int()),
					Range(root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MIN)->read_int(), root->awar(AWAR_PRIMER_DESIGN_TEMPERATURE_MAX)->read_int()),
					root->awar(AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST)->read_int(),
					(bool)root->awar(AWAR_PRIMER_DESIGN_EXPAND_IUPAC)->read_int(),
					root->awar(AWAR_PRIMER_DESIGN_MAX_PAIRS)->read_int(),
					root->awar(AWAR_PRIMER_DESIGN_GC_FACTOR)->read_float(),
					root->awar(AWAR_PRIMER_DESIGN_TEMP_FACTOR)->read_float()
					);

#ifdef DEBUG
     PD->run(PrimerDesign::PRINT_PRIMER_PAIRS);
#else
    PD->run(0);
#endif
    if ( !error ) error = PD->get_error();

    if (!error) {
        if (!pdrw_id) primer_design_create_result_window(aww);
        gb_assert(pdrw_id);

        // create result-list:
        pdrw->clear_selection_list(pdrw_id);
        int max_primer_length   = PD->get_max_primer_length();
        int max_position_length = int(log10(PD->get_max_primer_pos()))+1;
        int max_length_length   = int(log10(PD->get_max_primer_length()))+1;

        if (max_position_length<3) max_position_length = 3;
        if (max_length_length<3) max_length_length = 3;

        pdrw->insert_selection(pdrw_id,
                               GBS_global_string(" Rating | %-*s %-*s %-*s G/C Tmp | %-*s %-*s %-*s G/C Tmp",
                                                 max_primer_length, "Left primer",
                                                 max_position_length, "Pos",
                                                 max_length_length, "Len",

                                                 max_primer_length, "Right primer",
                                                 max_position_length, "Pos",
                                                 max_length_length, "Len"),
                               "");

        int r;

        for ( r = 0; ; ++r) {
            const char *primers = 0;
            const char *result  = PD->get_result(r, primers, max_primer_length, max_position_length, max_length_length);
            if (!result) break;
            pdrw->insert_selection(pdrw_id, result, primers);
        }
        if (!r) {
            pdrw->insert_default_selection(pdrw_id, "**** There are no results", "");
        }
        else {
            pdrw->insert_default_selection(pdrw_id, "**** End of list", "");
        }
        pdrw->update_selection_list(pdrw_id);
    }
  }
  if ( sequence ) free(sequence);
  if ( error ) aw_message(error);
}



AW_window *create_primer_design_window( AW_root *root,AW_default def)  {
//   GB_ERROR          error    = 0;
//   char             *sequence = 0;
//   PRD_Sequence_Pos  length;
//   {
//     GB_transaction  dummy(gb_main);
//     char           *selected_species = root->awar(AWAR_SPECIES_NAME)->read_string();
//     GBDATA         *gb_species       = GBT_find_species(gb_main,selected_species);

//     if ( !gb_species ) {
//       error = "You have to select a species!";
//       aw_message(error);
//     }
//     else {
//       const char *alignment = GBT_get_default_alignment(gb_main);
//       GBDATA     *gb_seq    = GBT_read_sequence(gb_species, alignment);

//       sequence = GB_read_string(gb_seq);
//       length = strlen(sequence);

//       // find reasonable parameters
//       SequenceIterator *i = new SequenceIterator( sequence, 0, 50, SequenceIterator::FORWARD );
//       while (i->nextBase() != SequenceIterator::EOS ) ;
//       root->awar(AWAR_PRIMER_DESIGN_POS_LEFT_MIN)->write_int(0);
//       root->awar(AWAR_PRIMER_DESIGN_POS_LEFT_MAX)->write_int(i->pos);
//       i->restart( length, 50, SequenceIterator::BACKWARD );
//       while (i->nextBase() != SequenceIterator::EOS ) ;
//       root->awar(AWAR_PRIMER_DESIGN_POS_RIGHT_MIN)->write_int(i->pos);
//       root->awar(AWAR_PRIMER_DESIGN_POS_RIGHT_MAX)->write_int(length);

//       delete i;
//       free(sequence);
//     }
//   }

  AWUSE(def);
  AW_window_simple *aws = new AW_window_simple;
  aws->init( root, "PRIMER_DESIGN","PRIMER DESIGN", 10, 10 );

  aws->load_xfig("prd_main.fig");

  aws->callback( (AW_CB0)AW_POPDOWN);
  aws->at("close");
  aws->create_button("CLOSE","CLOSE","C");

  aws->at("help");
  aws->callback(AW_POPUP_HELP,(AW_CL)"primer_new.hlp");
  aws->create_button("HELP","HELP","H");

  aws->callback( primer_design_event);
  aws->at("design");
  aws->highlight();
  aws->create_button("GO","GO","G");

  aws->at("minleft"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_LEFT_MIN, 7);
  aws->at("maxleft"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_LEFT_MAX, 7);

  aws->at("minright"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_RIGHT_MIN, 7);
  aws->at("maxright"); aws->create_input_field( AWAR_PRIMER_DESIGN_POS_RIGHT_MAX, 7);

  aws->at("minlen"); aws->create_input_field( AWAR_PRIMER_DESIGN_LENGTH_MIN, 7);
  aws->at("maxlen"); aws->create_input_field( AWAR_PRIMER_DESIGN_LENGTH_MAX, 7);

  aws->at("usedist"); aws->create_toggle(AWAR_PRIMER_DESIGN_DIST_USE);
  aws->at("mindist"); aws->create_input_field( AWAR_PRIMER_DESIGN_DIST_MIN, 7);
  aws->at("maxdist"); aws->create_input_field( AWAR_PRIMER_DESIGN_DIST_MAX, 7);

  aws->at("mingc"); aws->create_input_field( AWAR_PRIMER_DESIGN_GCRATIO_MIN, 7);
  aws->at("maxgc"); aws->create_input_field( AWAR_PRIMER_DESIGN_GCRATIO_MAX, 7);

  aws->at("mintemp"); aws->create_input_field( AWAR_PRIMER_DESIGN_TEMPERATURE_MIN, 7);
  aws->at("maxtemp"); aws->create_input_field( AWAR_PRIMER_DESIGN_TEMPERATURE_MAX, 7);


  aws->at("allowed_match");
  aws->create_input_field( AWAR_PRIMER_DESIGN_ALLOWED_MATCH_MIN_DIST, 7);

  aws->at("expand_IUPAC");
  aws->create_toggle( AWAR_PRIMER_DESIGN_EXPAND_IUPAC);

  aws->at("max_pairs");
  aws->create_input_field( AWAR_PRIMER_DESIGN_MAX_PAIRS, 7);

  aws->at("GC_factor");
  aws->create_input_field( AWAR_PRIMER_DESIGN_GC_FACTOR, 7);

  aws->at("temp_factor");
  aws->create_input_field( AWAR_PRIMER_DESIGN_TEMP_FACTOR, 7);

  return aws;
}



