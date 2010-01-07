// =============================================================== //
//                                                                 //
//   File      : GEN_translations.cxx                              //
//   Purpose   : supports removal of redundant translations of     //
//               gene CDS                                          // 
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "GEN_local.hxx"

#include <Translate.hxx>
#include <AP_codon_table.hxx>
#include <aw_question.hxx>
#include <arbdbt.h>

using namespace std;

// -------------------------------------------------
//      remove redundant translations from genes

#if defined(DEVEL_RALF)
# warning add menu-entry to genome-NTREE ("Remove reproduceable translations")
#endif // DEVEL_RALF

static char *translate_gene_sequence(GBDATA *gb_gene, GB_ERROR& error, int& translated_length, char *startCodon) {
    // return translation of gene sequence
    // the start codon is copied into result buffer 'startCodon' (has to be sized 4 bytes)

    size_t  gene_length;
    char   *gene_seq     = GBT_read_gene_sequence_and_length(gb_gene, true, 0, &gene_length);
    if (!gene_seq) error = GB_await_error();
    else {
        // store start codon in result buffer:
        memcpy(startCodon, gene_seq, 3);
        startCodon[3] = 0;

        int arb_transl_table, codon_start;
        error = AWT_getTranslationInfo(gb_gene, arb_transl_table, codon_start);
        
        if (arb_transl_table == -1) arb_transl_table = AWT_embl_transl_table_2_arb_code_nr(1); // use embl table 1 (standard code)
        if (codon_start == -1) codon_start           = 0; // default codon start

        if (!error) AWT_pro_a_nucs_convert(arb_transl_table, gene_seq, gene_length, codon_start, false, true, true, &translated_length);

        if (error) {
            free(gene_seq);
            gene_seq = 0;
        }
    }

    return gene_seq;
}

enum GEN_remove_state {
    GRS_NO_CHANGE           = 0,  // no translation found
    GRS_FAILED              = 1,  // error is set
    GRS_TRANSLATION_REMOVED = 2,  // translation was present, reproducible and has been removed
    GRS_TRANSLATION_FAILED  = 4,  // translation differed (wrote ARB translation to field  'ARB_translation')
    GRS_START_CODON_WRONG   = 8,  // translation differed only in start codon
    GRS_NOTE_ADDED          = 16, // note has been added
};

static GEN_remove_state remove_redundant_translation(GBDATA *gb_gene, bool ignore_start_codon_error, char *errornousCodon, GB_ERROR &error) {
    // If translation can be re-produced by ARB,
    //          it will be removed
    //          ('ARB_translation' will be removed as well in this case)
    // Otherwise
    //          a field 'ARB_translation' is inserted, which contains the translation generated by ARB.
    // 
    // If result is GRS_START_CODON_WRONG, the questionable codon is copied into errornousCodon.
    // (errornousCodon has to be a buffer with size == 4)
    // 
    // If another code or codonstart translates fine, a hint shall be written to field 'translation_hint'
#if defined(DEVEL_RALF)
#warning TODO: If another code or codonstart translates fine, a hint shall be written to field 'translation_hint'
#endif // DEVEL_RALF

    GEN_remove_state  result   = GRS_NO_CHANGE;
    error                      = 0;
    char             *add_note = 0; // will be added as 'ARB_translation_note' (if set)

    const char *to_remove[] = {
        "translation",
        "ARB_translation",
        "ARB_translation_note",
        0
    };

#define set_result_bit(s) result = GEN_remove_state(result|s)

    GBDATA *gb_translation = GB_entry(gb_gene, "translation");
    if (gb_translation) {
        int   translated_length;
        char *generated = translate_gene_sequence(gb_gene, error, translated_length, errornousCodon);

        if (!generated || translated_length<1) {
            // insert note and continue
            add_note = GBS_global_string_copy("Failed to translate gene-sequence (%s)", error);
            error    = 0;
            set_result_bit(GRS_TRANSLATION_FAILED);
        }
        else {
            if (generated[translated_length-1] == '*') {
                generated[--translated_length] = 0; // cut off stop codon
            }

            const char *original = GB_read_char_pntr(gb_translation);

            bool remove = false;
            if (strcmp(generated+1, original+1) == 0) { // most of translation matches
                if (generated[0] == original[0]) { // start codon matches
                    remove = true;
                }
                else { // start codon differs
                    set_result_bit(GRS_START_CODON_WRONG); // report
                    remove = ignore_start_codon_error; // and delete if requested
                }
            }

            if (remove) {       // remove translation and related entries
                GB_ERROR err          = 0;
                int      failed_field = -1;
                for (int r = 0; to_remove[r] && !err; ++r) {
                    GBDATA *gb_remove = GB_entry(gb_gene, to_remove[r]);
                    if (gb_remove) {
                        err                   = GB_delete(gb_remove);
                        if (err) failed_field = r;
                    }
                }
                if (err) error = GBS_global_string("Failed to delete field '%s' (%s)", to_remove[failed_field], err);
                else {
                    error = GBT_write_byte(gb_gene, "ARB_translation_rm", 1);
                    if (!error) set_result_bit(GRS_TRANSLATION_REMOVED);
                }
            }
            else {
                error = GBT_write_string(gb_gene, "ARB_translation", generated);
                if (!error) set_result_bit(GRS_TRANSLATION_FAILED);
            }
        }
        free(generated);
    }

    if (add_note && !error) {
        error = GBT_write_string(gb_gene, "ARB_translation_note", add_note);
        set_result_bit(GRS_NOTE_ADDED);
    }

    if (error) result = GRS_FAILED;
    free(add_note);

    return result;

#undef set_result_bit
}

GB_ERROR GEN_testAndRemoveTranslations(GBDATA *gb_gene_data, void (*warn)(AW_CL cd, const char *msg), AW_CL cd, AW_repeated_question *ok_to_ignore_wrong_start_codon) {
    int      ok                = 0; // identical translations
    int      failed            = 0; // non-identical translations
    int      wrong_start_codon = 0; // translations where start_codon differed
    int      no_entry          = 0; // genes w/o 'translation' entry
    int      note_added        = 0; // count gene for which a note has been added
    GB_ERROR error             = 0;

    GB_HASH  *wrongStartCodons = GBS_create_hash(50, GB_IGNORE_CASE);

    for (GBDATA *gb_gene = GB_entry(gb_gene_data, "gene"); gb_gene && !error; gb_gene = GB_nextEntry(gb_gene)) {
        int retry = 0;
        for (int Try = 0; Try <= retry && !error; Try++) {
            error = 0;
            
            char             startCodon[4];
            GEN_remove_state state = remove_redundant_translation(gb_gene, Try, startCodon, error);

            switch (state) {
                case GRS_NO_CHANGE:
                    no_entry++;
                    break;

                case GRS_FAILED:
                    gen_assert(error);
                    break;

                default:
                    if (state&GRS_TRANSLATION_REMOVED) {
                        ok++;
                    }
                    else {
                        gen_assert(state&GRS_TRANSLATION_FAILED);
                        if (Try == 0) {
                            if (state&GRS_START_CODON_WRONG) {
                                wrong_start_codon++;
                                AW_repeated_question* q = ok_to_ignore_wrong_start_codon;

                                if (q->get_answer("Translation differs only in start codon",
                                                  "Ignore and remove,Keep translation", "all", false) == 0) {
                                    retry++;
                                }
                                else {
                                    failed++;
                                }

                                GBS_incr_hash(wrongStartCodons, startCodon);
                            }
                            else if (state&GRS_NOTE_ADDED) {
                                failed++;
                                note_added++;
                            }
                        }
                        else {
                            failed++;
                        }
                    }
                    break;
            }
        }
    }

    if (!error && failed>0) {
        warn(cd, GBS_global_string("%i translations could not be reproduced by ARB", failed));
        static bool first_warning = true;
        if (first_warning) { // show details once 
            warn(cd,
                 "Note: Reproducible translations were removed from database.\n"
                 "      Failed translations were left in database and an additional\n"
                 "      field 'ARB_translation' was added.");
            warn(cd, GBS_global_string("- %i genes had no translation entry", no_entry));
            warn(cd, GBS_global_string("- %i translations were reproducible", ok));
            first_warning = false;
        }
        if (wrong_start_codon>0) {
            char *codonInfo = GBS_hashtab_2_string(wrongStartCodons);
            warn(cd, GBS_global_string("- %i translations had wrong start codon (%s)", wrong_start_codon, codonInfo));
            free(codonInfo);
        }
        if (note_added>0) {
            warn(cd, GBS_global_string("- %i ARB_translation_note entries were generated. Please examine!", note_added));
        }
    }

    GBS_free_hash(wrongStartCodons);

    return error;
}



