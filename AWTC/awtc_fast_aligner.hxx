#ifndef awtc_fast_aligner_hxx_included
#define awtc_fast_aligner_hxx_included


#define FAST_ALIGNER_TITLE "Fast Aligner V1.03"

typedef char* 	(*AWTC_get_consensus_func)(const char *species_name, int start_pos, int end_pos);
typedef int 	(*AWTC_get_selected_range)(int *firstColumn, int *lastColumn);
typedef GBDATA* (*AWTC_get_first_selected_species)(int *total_no_of_selected_species);
typedef GBDATA* (*AWTC_get_next_selected_species)(void);

struct AWTC_faligner_cd
{
    int do_refresh;		// if do_refresh==TRUE then AWTC_start_faligning() does a refresh
    void (*refresh_display)();	// via calling refresh_display()
    
    AWTC_get_consensus_func 		get_group_consensus;
    AWTC_get_selected_range 		get_selected_range;
    AWTC_get_first_selected_species	get_first_selected_species;
    AWTC_get_next_selected_species 	get_next_selected_species;
};

// --------------------------------------------------------------------------------

AW_window *AWTC_create_faligner_window(AW_root *awr, AW_CL cd2);    
void AWTC_create_faligner_variables(AW_root *root,AW_default db1);
void AWTC_awar_set_actual_sequence(AW_root *root, AW_default db1);

void AWTC_start_faligning(AW_window *aw, AW_CL cd2);
GB_ERROR AWTC_delete_temp_entries(GBDATA *gb_main, GB_CSTR alignment);
GB_ERROR AWTC_mirrorSequence(char *seq, long length, GB_alignment_type alignment_type);

// --------------------------------------------------------------------------------

char AWTC_complementBase(char c, char T_or_U);

char *AWTC_reverseString(const char *s, int len);
char *AWTC_complementString(const char *s, int len, char T_or_U);

// --------------------------------------------------------------------------------


#endif
