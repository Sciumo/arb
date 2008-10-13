#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include "adlocal.h"
#include "arbdbt.h"
#include "adGene.h"
#include "ad_config.h"
#include "aw_awars.hxx"

#define GBT_GET_SIZE 0
#define GBT_PUT_DATA 1

#define DEFAULT_LENGTH        0.1 /* default length of tree-edge w/o given length */
#define DEFAULT_LENGTH_MARKER -1000.0 /* tree-edges w/o length are marked with this value during read and corrected in GBT_scale_tree */

/********************************************************************************************
                    some alignment header functions
********************************************************************************************/

char **GBT_get_alignment_names(GBDATA *gbd)
{
    /* get all alignment names out of a database
       (array of strings, the last stringptr is zero)
       Note: use GBT_free_names() to free strings+array
    */

    GBDATA *presets;
    GBDATA *ali;
    GBDATA *name;
    long size;

    char **erg;
    presets = GB_search(gbd,"presets",GB_CREATE_CONTAINER);
    size = 0;
    for (ali = GB_entry(presets,"alignment"); ali; ali = GB_nextEntry(ali)) {
        size ++;
    }
    erg = (char **)GB_calloc(sizeof(char *),(size_t)size+1);
    size = 0;
    for (ali = GB_entry(presets,"alignment"); ali; ali = GB_nextEntry(ali)) {
        name = GB_entry(ali,"alignment_name");
        if (!name) {
            erg[size] = (char *)GB_STRDUP("alignment_name ???");
        }else{
            erg[size] = GB_read_string(name);
        }
        size ++;
    }
    return erg;
}

static char *gbt_nonexisting_alignment(GBDATA *gbMain) {
    char  *ali_other = 0;
    int    counter;

    for (counter = 1; !ali_other; ++counter) {
        ali_other = GBS_global_string_copy("ali_x%i", counter);
        if (GBT_get_alignment(gbMain, ali_other) != 0) {
            free(ali_other);
            ali_other = 0;
        }
    }

    return ali_other;
}

GB_ERROR GBT_check_alignment_name(const char *alignment_name)
{
    GB_ERROR error;
    if ( (error = GB_check_key(alignment_name)) ) return error;
    if (strncmp(alignment_name,"ali_",4)){
        return GB_export_error("your alignment_name '%s' must start with 'ali_'",
                               alignment_name);
    }
    return 0;
}

GBDATA *GBT_create_alignment(GBDATA *gbd,
                             const char *name, long len, long aligned,
                             long security, const char *type)
{
    GBDATA *gb_presets;
    GBDATA *gbn;
    char c[2];
    GB_ERROR error;
    c[1] = 0;
    gb_presets = GB_search(gbd,"presets",GB_CREATE_CONTAINER);
    if (!gb_presets) return NULL;

    error = GBT_check_alignment_name(name);
    if (error) {
        GB_export_error(error);
        return NULL;
    }

    if ( aligned<0 ) aligned = 0;
    if ( aligned>1 ) aligned = 1;
    if ( security<0) {
        error = GB_export_error("Negative securities not allowed");
        return NULL;
    }
    if ( security>6) {
        error = GB_export_error("Securities above 6 are not allowed");
        return NULL;
    }
    if (!strstr("dna:rna:ami:usr",type)) {
        error = GB_export_error("Unknown alignment type '%s'",type);
        return NULL;
    }
    gbn = GB_find_string(gb_presets,"alignment_name",name,GB_IGNORE_CASE,down_2_level);
    if (gbn) {
        error = GB_export_error("Alignment '%s' already exists",name);
        return NULL;
    }

    gbd = GB_create_container(gb_presets,"alignment");
    GB_write_security_delete(gbd,6);
    gbn =   GB_create(gbd,"alignment_name",GB_STRING);
    GB_write_string(gbn,name);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,6);

    gbn =   GB_create(gbd,"alignment_len",GB_INT);
    GB_write_int(gbn,len);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);

    gbn =   GB_create(gbd,"aligned",GB_INT);
    GB_write_int(gbn,aligned);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);
    gbn =   GB_create(gbd,"alignment_write_security",GB_INT);
    GB_write_int(gbn,security);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,6);

    gbn =   GB_create(gbd,"alignment_type",GB_STRING);
    GB_write_string(gbn,type);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);
    return gbd;
}

NOT4PERL GB_ERROR GBT_check_alignment(GBDATA *gb_main, GBDATA *preset_alignment, GB_HASH *species_name_hash)
/* check if alignment is of the correct size
   and whether all data is present.
   Sets the security deletes and writes.

   If 'species_name_hash' is not NULL,
   it initially has to contain value == 1 for each existing species.
   Afterwards it contains value == 2 for each species where an alignment has been found.
*/
{
    GBDATA   *gb_species_data  = GBT_find_or_create(gb_main,"species_data",7);
    GBDATA   *gb_extended_data = GBT_find_or_create(gb_main,"extended_data",7);
    GB_ERROR  error         = 0;
    char     *ali_name      = 0;
    {
        GBDATA *gb_ali_name = GB_entry(preset_alignment,"alignment_name");
        if (!gb_ali_name) {
            error = "alignment without 'alignment_name' -- database corrupted";
        }
        else {
            ali_name = GB_read_string(gb_ali_name);
        }
    }

    if (!error) {
        long    security_write = -1;
        long    stored_ali_len = -1;
        long    found_ali_len  = -1;
        long    aligned        = 1;
        GBDATA *gb_ali_len     = 0;

        {
            GBDATA *gb_ali_wsec = GB_entry(preset_alignment,"alignment_write_security");
            if (!gb_ali_wsec) {
                error = "has no 'alignment_write_security' entry";
            }
            else {
                security_write = GB_read_int(gb_ali_wsec);
            }
        }


        if (!error) {
            gb_ali_len = GB_entry(preset_alignment,"alignment_len");
            if (!gb_ali_len) {
                error = "has no 'alignment_len' entry";
            }
            else {
                stored_ali_len = GB_read_int(gb_ali_len);
            }
        }

        if (!error) {
            GBDATA *gb_species;
            for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
                 gb_species && !error;
                 gb_species = GBT_next_species(gb_species))
            {
                GBDATA     *gb_name        = GB_entry(gb_species,"name");
                const char *name           = 0;
                int         alignment_seen = 0;
                GBDATA     *gb_ali         = 0;

                if (gb_name) {
                    name = GB_read_char_pntr(gb_name);
                    if (species_name_hash) {
                        int seen = GBS_read_hash(species_name_hash, name);

                        gb_assert(seen != 0); // species_name_hash not initialized correctly
                        if (seen == 2) alignment_seen = 1; // already seen an alignment
                    }
                }
                else {
                    gb_name = GB_create(gb_species,"name",GB_STRING);
                    GB_write_string(gb_name,"unknown"); // @@@ create a unique name here
                }

                GB_write_security_delete(gb_name,7);
                GB_write_security_write(gb_name,6);

                gb_ali = GB_entry(gb_species, ali_name);
                if (gb_ali) {
                    /* GB_write_security_delete(ali,security_write); */
                    GBDATA *gb_data = GB_entry(gb_ali, "data");
                    if (!gb_data) {
                        gb_data = GB_create(gb_ali,"data",GB_STRING);
                        GB_write_string(gb_data, "Error: entry 'data' was missing and therefore was filled with this text.");

                        GB_warning("No '%s/data' entry for species '%s' (has been filled with dummy data)", ali_name, name);
                    }
                    else {
                        if (GB_read_type(gb_data) != GB_STRING){
                            GB_delete(gb_data);
                            error = GBS_global_string("'%s/data' of species '%s' had wrong DB-type (%s) and has been deleted!",
                                                      ali_name, name, GB_read_key_pntr(gb_data));
                        }
                        else {
                            long data_len = GB_read_string_count(gb_data);
                            if (found_ali_len != data_len) {
                                if (found_ali_len>0)        aligned       = 0;
                                if (found_ali_len<data_len) found_ali_len = data_len;
                            }
                            /* error = GB_write_security_write(data,security_write);
                               if (error) return error;*/
                            GB_write_security_delete(gb_data,7);

                            if (!alignment_seen && species_name_hash) { // mark as seen
                                GBS_write_hash(species_name_hash, name, 2); // 2 means "species has data in at least 1 alignment"
                            }
                        }
                    }
                }

                GB_write_security_delete(gb_species,security_write);
            }
        }

        if (!error) {
            GBDATA *gb_sai;
            for (gb_sai = GBT_first_SAI_rel_exdata(gb_extended_data);
                 gb_sai && !error;
                 gb_sai = GBT_next_SAI(gb_sai) )
            {
                GBDATA *gb_sai_name = GB_entry(gb_sai, "name");
                GBDATA *gb_ali;

                if (!gb_sai_name) continue;

                GB_write_security_delete(gb_sai_name,7);

                gb_ali = GB_entry(gb_sai, ali_name);
                if (gb_ali) {
                    GBDATA *gb_sai_data;
                    for (gb_sai_data = GB_child(gb_ali) ;
                         gb_sai_data;
                         gb_sai_data = GB_nextChild(gb_sai_data))
                    {
                        long type = GB_read_type(gb_sai_data);
                        long data_len;

                        if (type == GB_DB || type < GB_BITS) continue;
                        if (GB_read_key_pntr(gb_sai_data)[0] == '_') continue; // e.g. _STRUCT (of secondary structure)

                        data_len = GB_read_count(gb_sai_data);

                        if (found_ali_len != data_len) {
                            if (found_ali_len>0)        aligned       = 0;
                            if (found_ali_len<data_len) found_ali_len = data_len;
                        }
                    }
                }
            }
        }

        if (stored_ali_len != found_ali_len) {
            error = GB_write_int(gb_ali_len, found_ali_len);
            if (error) return error;
        }
        {
            GBDATA *gb_aligned = GB_search(preset_alignment, "aligned", GB_INT);
            if (GB_read_int(gb_aligned) != aligned) {
                error = GB_write_int(gb_aligned, aligned);
                if (error) return error;
            }
        }

        if (error) {
            error = GBS_global_string("alignment '%s' %s\nDatabase corrupted - try to fix if possible, save with different name and restart application.",
                                      ali_name, error);
        }

        free(ali_name);
    }
    return error;
}

static GB_ERROR gbt_rename_alignment_of_item(GBDATA *gb_item_container, const char *item_name, const char *item_entry_name,
                                             const char *source, const char *dest, int copy, int dele)
{
    GB_ERROR  error = 0;
    GBDATA   *gb_item;

    for (gb_item = GB_entry(gb_item_container, item_entry_name);
         gb_item && !error;
         gb_item = GB_nextEntry(gb_item))
    {
        GBDATA *gb_ali = GB_entry(gb_item, source);
        if (!gb_ali) continue;

        if (copy) {
            GBDATA *gb_new = GB_entry(gb_item, dest);
            if (gb_new) {
                error = GBS_global_string("Entry '%s' already exists", dest);
            }
            else {
                gb_new = GB_create_container(gb_item,dest);
                if (!gb_new) {
                    error = GB_get_error();
                }
                else {
                    error = GB_copy(gb_new,gb_ali);
                }
            }
        }
        if (dele) {
            error = GB_delete(gb_ali);
        }
    }

    if (error && gb_item) {
        char   *name      = GBS_global_string_copy("<unknown%s>", item_name);
        GBDATA *gb_name   = GB_entry(gb_item, "name");
        if (gb_name) name = GB_read_string(gb_name);

        error = GBS_global_string("%s\n(while renaming alignment for %s '%s')", error, item_name, name);
        free(name);
    }

    return error;
}

GB_ERROR GBT_rename_alignment(GBDATA *gbMain, const char *source, const char *dest, int copy, int dele)
{
    /*  if copy     == 1 then create a copy
        if dele     == 1 then delete old */

    GB_ERROR  error            = 0;
    int       is_case_error    = 0;
    GBDATA   *gb_presets       = GB_entry(gbMain, "presets");
    GBDATA   *gb_species_data  = GB_entry(gbMain, "species_data");
    GBDATA   *gb_extended_data = GB_entry(gbMain, "extended_data");

    if (!gb_presets)            error = "presets not found";
    else if (!gb_species_data)  error = "species_data not found";
    else if (!gb_extended_data) error = "extended_data not found";


    /* create copy and/or delete old alignment description */
    if (!error) {
        GBDATA *gb_old_alignment = GBT_get_alignment(gbMain, source);

        if (!gb_old_alignment) {
            error = GBS_global_string("source alignment '%s' not found", source);
        }
        else {
            if (copy) {
                GBDATA *gbh = GBT_get_alignment(gbMain, dest);
                if (gbh) {
                    error         = GBS_global_string("destination alignment '%s' already exists", dest);
                    is_case_error = (strcasecmp(source, dest) == 0); // test for case-only difference
                }
                else {
                    error = GBT_check_alignment_name(dest);
                    if (!error) {
                        GBDATA *gb_new_alignment = GB_create_container(gb_presets,"alignment");
                        error                    = GB_copy(gb_new_alignment, gb_old_alignment);
                        if (!error) {
                            GBDATA *gb_alignment_name = GB_search(gb_new_alignment,"alignment_name",GB_FIND);
                            error                     = GB_write_string(gb_alignment_name,dest);
                        }
                    }
                }
            }

            if (dele && !error) {
                error = GB_delete(gb_old_alignment);
            }
        }
    }

    /* change default alignment */
    if (!error) {
        GBDATA *gb_using = GB_search(gb_presets,"use",GB_STRING);
        if (dele && copy){
            error = GB_write_string(gb_using, dest);
        }
    }

    /* copy and/or delete alignment entries in species */
    if (!error) {
        error = gbt_rename_alignment_of_item(gb_species_data, "Species", "species", source, dest, copy, dele);
    }

    /* copy and/or delete alignment entries in SAIs */
    if (!error) {
        error = gbt_rename_alignment_of_item(gb_extended_data, "SAI", "extended", source, dest, copy, dele);
    }

    if (is_case_error) {
        /* alignments source and dest only differ in case */
        char *ali_other = gbt_nonexisting_alignment(gbMain);
        ad_assert(copy);

        printf("Renaming alignment '%s' -> '%s' -> '%s' (to avoid case-problem)\n", source, ali_other, dest);

        error             = GBT_rename_alignment(gbMain, source, ali_other, 1, dele);
        if (!error) error = GBT_rename_alignment(gbMain, ali_other, dest, 1, 1);

        free(ali_other);
    }

    return error;
}

GBDATA *GBT_find_or_create(GBDATA *Main,const char *key,long delete_level)
{
    GBDATA *gbd;
    gbd = GB_entry(Main,key);
    if (gbd) return gbd;
    gbd = GB_create_container(Main,key);
    GB_write_security_delete(gbd,delete_level);
    return gbd;
}

/********************************************************************************************
                    check the database !!!
********************************************************************************************/

static long check_for_species_without_data(const char *species_name, long value, void *counterPtr) {
    if (value == 1) {
        long cnt = *((long*)counterPtr);
        if (cnt<40) {
            GB_warning("Species '%s' has no data in any alignment", species_name);
        }
        *((long*)counterPtr) = cnt+1;
    }
    return value; // new hash value
}

#if defined(DEVEL_RALF)
#warning GBT_check_data ignores given 'alignment_name' if we have a default alignment. seems wrong!
#endif /* DEVEL_RALF */


GB_ERROR GBT_check_data(GBDATA *Main, const char *alignment_name)
/* if alignment_name == 0 -> check all existing alignments
 * otherwise check only one alignment
 */
{
    GB_ERROR  error             = 0;
    GBDATA   *gb_sd             = GBT_find_or_create(Main,"species_data",7);
    GBDATA   *gb_presets        = GBT_find_or_create(Main,"presets",7);
    GB_HASH  *species_name_hash = 0;

    GBT_find_or_create(Main,"extended_data",7);
    GBT_find_or_create(Main,"tree_data",7);

    if (alignment_name) {
        GBDATA *gb_ali_name = GB_find_string(gb_presets, "alignment_name", alignment_name, GB_IGNORE_CASE, down_2_level);
        if (!gb_ali_name) {
            error = GBS_global_string("Alignment '%s' does not exist - it can't be checked.", alignment_name);
        }
    }

    if (!error) {
        // check whether we have an default alignment
        GBDATA *gb_use = GB_entry(gb_presets, "use");
        if (!gb_use) {
            // if we have no default alignment -> look for any alignment
            GBDATA *gb_ali_name = GB_find_string(gb_presets,"alignment_name",alignment_name,GB_IGNORE_CASE,down_2_level);

            if (gb_ali_name) {
                // use first alignment found
                GBDATA *gb_ali = GB_get_father(gb_ali_name);
                gb_ali_name    = GB_entry(gb_ali,"alignment_name");

                gb_use = GB_create(gb_presets,"use",GB_STRING);
                GB_write_string(gb_use, GB_read_char_pntr(gb_ali_name));
            }
        }
    }

    if (!alignment_name && !error) {
        // if all alignments are checked -> use species_name_hash to detect duplicated species and species w/o data
        GBDATA *gb_species;
        long    duplicates = 0;
        species_name_hash  = GBS_create_hash(GBT_get_species_hash_size(Main), GB_IGNORE_CASE);

        for (gb_species = GBT_first_species_rel_species_data(gb_sd);
             gb_species && !error;
             gb_species = GBT_next_species(gb_species))
        {
            GBDATA *gb_name = GB_entry(gb_species, "name");
            if (gb_name) {
                const char *name  = GB_read_char_pntr(gb_name);
                long        occur = GBS_read_hash(species_name_hash, name) + 1;

                if (occur>1) duplicates++;
                GBS_write_hash(species_name_hash, name, occur);
            }
        }

        if (duplicates) {
            error = GBS_global_string("Database is corrupted:\n"
                                      "Found %li duplicated species with identical names!\n"
                                      "Fix the problem using\n"
                                      "   'Search For Equal Fields and Mark Duplikates'\n"
                                      "in ARB_NTREE search tool, save DB and restart ARB."
                                      , duplicates);
        }
    }

    if (!error) {
        GBDATA *gb_ali;

        for (gb_ali = GB_entry(gb_presets,"alignment");
             gb_ali && !error;
             gb_ali = GB_nextEntry(gb_ali))
        {
            error = GBT_check_alignment(Main, gb_ali, species_name_hash);
        }
    }

    if (species_name_hash) {
        if (!error) {
            long counter = 0;
            GBS_hash_do_loop2(species_name_hash, check_for_species_without_data, &counter);
            if (counter>0) {
                GB_warning("Found %li species without alignment data (only some were listed)", counter);
            }
        }

        GBS_free_hash(species_name_hash);
    }

    return error;
}

char *gbt_insert_delete(const char *source, long len, long destlen, long *newsize, long pos, long nchar, long mod, char insert_what, char insert_tail)
{
    /* removes or inserts bytes in a string
       len ==   len of source
       destlen == if != 0 than cut or append characters to get this len
       newsize      the result
       pos      where to insert/delete
       nchar        and how many items
       mod      size of an item
       insert_what  insert this character
    */

    char *newval;

    pos *=mod;
    nchar *= mod;
    len *= mod;
    destlen *= mod;
    if (!destlen) destlen = len;        /* if no destlen is set than keep len */

    if ( (nchar <0) && (pos-nchar>destlen)) nchar = pos-destlen;

    if (len > destlen) {
        len = destlen;          /* cut tail */
        newval = (char *)GB_calloc(sizeof(char),(size_t)(destlen+nchar+1));
    }else if (len < destlen) {              /* append tail */
        newval = (char *)malloc((size_t)(destlen+nchar+1));
        memset(newval,insert_tail,(int)(destlen+nchar));
        newval[destlen+nchar] = 0;
    }else{
        newval = (char *)GB_calloc(sizeof(char),(size_t)(len+nchar+1));
    }
    *newsize = (destlen+nchar)/mod;
    newval[*newsize] = 0;                   /* only for strings */

    if (pos>len){       /* no place to insert / delete */
        GB_MEMCPY(newval,source,(size_t)len);
        return NULL;
    }

    if (nchar < 0) {
        if (pos-nchar>len) nchar = -(len-pos);      /* clip maximum characters to delete */
    }

    if (nchar > 0)  {                   /* insert */
        GB_MEMCPY(newval,source,(size_t)pos);
        memset(newval+pos,insert_what,(size_t)nchar);
        GB_MEMCPY(newval+pos+nchar,source+pos,(size_t)(len-pos));
    }else{
        GB_MEMCPY(newval,source,(size_t)pos);
        GB_MEMCPY(newval+pos,source+pos-nchar, (size_t)(len - pos + nchar));
    }
    return newval;
}

GB_ERROR gbt_insert_character_gbd(GBDATA *gb_data, long len, long pos, long nchar, const char *delete_chars, const char *species_name){
    GB_TYPES    type;
    long        size  = 0,l;
    long        i;
    long        dlen;
    const char *cchars;
    char       *chars = 0;
    GB_ERROR    error = 0;
    GBDATA     *gb;

    ad_assert(pos>=0);
    type = GB_read_type(gb_data);
    if (type >= GB_BITS && type != GB_DB) {
        size = GB_read_count(gb_data);
        if ((len == size) && (!nchar)) return 0;
    }

    if (GB_read_key_pntr(gb_data)[0] == '_') return 0;

    switch(type) {
        case GB_DB:
            for (gb = GB_child(gb_data); gb; gb = GB_nextChild(gb)) {
                error = gbt_insert_character_gbd(gb,len,pos,nchar,delete_chars, species_name);
                if (error) return error;
            }
            break;

        case GB_STRING:
            cchars = GB_read_char_pntr(gb_data);
            if (!cchars) return GB_get_error();

            if (pos>len) break;

            if (nchar > 0) {            /* insert character */
                if ( (pos >0 && cchars[pos-1] == '.') ||  cchars[pos]  == '.') {
                    chars = gbt_insert_delete(cchars,size,len, &dlen, pos, nchar, sizeof(char), '.' ,'.' );
                }
                else {
                    chars = gbt_insert_delete(cchars,size,len, &dlen, pos, nchar, sizeof(char), '-', '.' );
                }

            }else{
                l = pos+(-nchar);
                if (l>size) l = size;
                for (i = pos; i<l; i++){
                    if (delete_chars[((const unsigned char *)cchars)[i]]) {
                        return GB_export_error(
                                               "You tried to delete '%c' in species %s position %li  -> Operation aborted",
                                               cchars[i],species_name,i);
                    }
                }
                chars = gbt_insert_delete(cchars,size,len, &dlen, pos, nchar, sizeof(char), '-', '.' );
            }
            if (chars) {
                error = GB_write_string(gb_data,chars);
                free(chars);
            }
            break;
        case GB_BITS:
            cchars = GB_read_bits_pntr(gb_data,'-','+');
            if (!cchars) return GB_get_error();
            chars = gbt_insert_delete(cchars,size,len, &dlen, pos, nchar, sizeof(char), '-','-' );
            if (chars) { error = GB_write_bits(gb_data,chars,dlen,"-");
            free(chars);
            }
            break;
        case GB_BYTES:
            cchars = GB_read_bytes_pntr(gb_data);
            if (!cchars) return GB_get_error();
            chars = gbt_insert_delete(cchars,size,len, &dlen, pos, nchar, sizeof(char), 0,0 );
            if (chars) {
                error = GB_write_bytes(gb_data,chars,dlen);
                free(chars);
            }
            break;
        case GB_INTS:{
            GB_UINT4 *longs;
            GB_CUINT4 *clongs;
            clongs = GB_read_ints_pntr(gb_data);
            if (!clongs) return GB_get_error();
            longs = (GB_UINT4 *)gbt_insert_delete( (char *)clongs,size,len, &dlen, pos, nchar, sizeof(GB_UINT4), 0 ,0);
            if (longs) {
                error = GB_write_ints(gb_data,longs,dlen);
                free((char *)longs);
            }
            break;
        }
        case GB_FLOATS:{
            float   *floats;
            GB_CFLOAT   *cfloats;
            cfloats = GB_read_floats_pntr(gb_data);
            if (!cfloats) return GB_get_error();
            floats = (float *)gbt_insert_delete( (char *)cfloats,size,len, &dlen, pos, nchar, sizeof(float), 0 , 0);
            if (floats) {
                error = GB_write_floats(gb_data,floats,dlen);
                free((char *)floats);
            }
            break;
        }
        default:
            break;
    }
    return error;
}


GB_ERROR gbt_insert_character_species(GBDATA *gb_species,const char *ali_name, long len, long pos, long nchar, const char   *delete_chars)
{
    GBDATA *gb_name;
    GBDATA *gb_ali;
    char    *species_name = 0;
    GB_ERROR error = 0;

    gb_ali = GB_entry(gb_species,ali_name);
    if (!gb_ali) return 0;
    gb_name = GB_entry(gb_species,"name");
    if (gb_name) species_name = GB_read_string(gb_name);
    error = gbt_insert_character_gbd(gb_ali,len,pos,nchar,delete_chars, species_name);
    if (error) error = GB_export_error("Species/SAI '%s': %s",species_name,error);
    free(species_name);
    return error;
}

GB_ERROR gbt_insert_character(GBDATA *gb_species_data, const char *species,const char *name, long len, long pos, long nchar, const char *delete_chars)
{
    GBDATA *gb_species;
    GB_ERROR error;

    long species_count = GBT_get_species_count(GB_get_root(gb_species_data));
    long count         = 0;

    for (gb_species = GB_entry(gb_species_data,species);
         gb_species;
         gb_species = GB_nextEntry(gb_species))
    {
        count++;
        GB_status((double)count/species_count);
        error = gbt_insert_character_species(gb_species,name,len,pos,nchar,delete_chars);
        if (error) return error;
    }
    return 0;
}

GB_ERROR GBT_check_lengths(GBDATA *Main,const char *alignment_name)
{
    GBDATA *gb_ali;
    GBDATA *gb_presets;
    GBDATA *gb_species_data;
    GBDATA *gb_extended_data;
    GBDATA *gbd;
    GBDATA *gb_len;
    GB_ERROR error;

    gb_presets       = GBT_find_or_create(Main,"presets",7);
    gb_species_data  = GBT_find_or_create(Main,"species_data",7);
    gb_extended_data = GBT_find_or_create(Main,"extended_data",7);

    for (gb_ali = GB_entry(gb_presets,"alignment");
         gb_ali;
         gb_ali = GB_nextEntry(gb_ali))
    {
        gbd    = GB_find_string(gb_ali,"alignment_name",alignment_name,GB_IGNORE_CASE,down_level);
        gb_len = GB_entry(gb_ali,"alignment_len");
        if (gbd) {
            error = gbt_insert_character(gb_extended_data,"extended",
                                         GB_read_string(gbd),
                                         GB_read_int(gb_len),0,0,0);
            if (error) return error;

            error = gbt_insert_character(gb_species_data,"species",
                                         GB_read_string(gbd),
                                         GB_read_int(gb_len),0,0,0);
            if (error) return error;
        }
    }
    return 0;
}

GB_ERROR GBT_format_alignment(GBDATA *Main, const char *alignment_name) {
    GB_ERROR err = 0;

    if (strcmp(alignment_name, GENOM_ALIGNMENT) != 0) { // NEVER EVER format 'ali_genom'
        err           = GBT_check_data(Main, alignment_name); // detect max. length
        if (!err) err = GBT_check_lengths(Main, alignment_name); // format sequences in alignment
        if (!err) err = GBT_check_data(Main, alignment_name); // sets state to "formatted"
    }
    else {
        err = "It's forbidden to format '" GENOM_ALIGNMENT "'!";
    }
    return err;
}


GB_ERROR GBT_insert_character(GBDATA *Main,char *alignment_name, long pos, long count, char *char_delete)
{
    /*  insert 'count' characters at position pos
        if count < 0    then delete position to position+|count| */
    GBDATA *gb_ali;
    GBDATA *gb_presets;
    GBDATA *gb_species_data;
    GBDATA *gb_extended_data;
    GBDATA *gbd;
    GBDATA *gb_len;
    long len;
    int ch;
    GB_ERROR error;
    char char_delete_list[256];

    if (pos<0) {
        error = GB_export_error("Illegal sequence position");
        return error;
    }

    gb_presets = GBT_find_or_create(Main,"presets",7);
    gb_species_data = GBT_find_or_create(Main,"species_data",7);
    gb_extended_data = GBT_find_or_create(Main,"extended_data",7);

    if (strchr(char_delete,'%') ) {
        memset(char_delete_list,0,256);
    }else{
        for (ch = 0;ch<256; ch++) {
            if (char_delete) {
                if (strchr(char_delete,ch)) char_delete_list[ch] = 0;
                else            char_delete_list[ch] = 1;
            }else{
                char_delete_list[ch] = 0;
            }
        }
    }

    for (gb_ali = GB_entry(gb_presets, "alignment"); gb_ali; gb_ali = GB_nextEntry(gb_ali)) {
        char *use;
        gbd = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, down_level);
        if (gbd) {
            gb_len = GB_entry(gb_ali, "alignment_len");
            len = GB_read_int(gb_len);
            if (pos > len)
                return
                    GB_export_error("GBT_insert_character: insert position %li exceeds length %li", pos, len);
            if (count < 0) {
                if (pos - count > len)
                    count = pos - len;
            }
            error = GB_write_int(gb_len, len + count);

            if (error) return error;
            use = GB_read_string(gbd);
            error = gbt_insert_character(gb_species_data, "species", use, len, pos, count, char_delete_list);
            if (error) {
                free(use);
                return error;
            }
            error = gbt_insert_character(gb_extended_data, "extended", use, len, pos, count, char_delete_list);
            free(use);
            if (error) return error;
        }
    }

    GB_disable_quicksave(Main,"a lot of sequences changed");
    return 0;
}


/********************************************************************************************
                    some tree write functions
********************************************************************************************/

GB_ERROR GBT_delete_tree(GBT_TREE *tree)
     /* frees a tree only in memory (not in the database)
        to delete the tree in Database
        just call GB_delete((GBDATA *)gb_tree);
    */
{
    GB_ERROR error;
    if (tree->name)  free(tree->name);
    if (tree->remark_branch) free(tree->remark_branch);
    if (tree->is_leaf == 0) {
        gb_assert(tree->leftson);
        gb_assert(tree->rightson);
        if ( (error=GBT_delete_tree( tree->leftson) ) ) return error;
        if ( (error=GBT_delete_tree( tree->rightson)) ) return error;
    }
    if (!tree->father || !tree->tree_is_one_piece_of_memory){
        free((char *)tree);
    }
    return 0;
}

GB_ERROR GBT_write_group_name(GBDATA *gb_group_name, const char *new_group_name) {
    GB_ERROR error = 0;
    size_t   len   = strlen(new_group_name);

    if (len >= GB_GROUP_NAME_MAX) {
        char *shortened = (char*)GB_calloc(10+len+1, 1);
        strcpy(shortened, "Too long: ");
        strcpy(shortened+10, new_group_name);
        strcpy(shortened+GB_GROUP_NAME_MAX-4, "..."); // cut end
        error = GB_write_string(gb_group_name, shortened);
    }
    else {
        error = GB_write_string(gb_group_name, new_group_name);
    }
    return error;
}

long gbt_write_tree_nodes(GBDATA *gb_tree,GBT_TREE *node,long startid)
{
    long me;
    GB_ERROR error;
    const char *key;
    GBDATA *gb_id,*gb_name,*gb_any;
    if (node->is_leaf) return 0;
    me = startid;
    if (node->name && (strlen(node->name)) ) {
        if (!node->gb_node) {
            node->gb_node = GB_create_container(gb_tree,"node");
        }
        gb_name = GB_search(node->gb_node,"group_name",GB_STRING);
        error = GBT_write_group_name(gb_name, node->name);
/*         error = GB_write_string(gb_name,node->name); */
        if (error) return -1;
    }
    if (node->gb_node){         /* delete not used nodes else write id */
        gb_any = GB_child(node->gb_node);
        if (gb_any) {
            key = GB_read_key_pntr(gb_any);
            if (!strcmp(key,"id")){
                gb_any = GB_nextChild(gb_any);
            }
        }

        if (gb_any){
            gb_id = GB_search(node->gb_node,"id",GB_INT);
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                int old = GB_read_int(gb_id);
                if (old != me) {
                    printf("id changed in gbt_write_tree_nodes(): old=%i new=%li (tree-node=%p; gb_node=%p)\n",
                           old, me, node, node->gb_node);
                }
            }
#endif /* DEBUG */
            error = GB_write_int(gb_id,me);
            GB_write_usr_private(node->gb_node,0);
            if (error) return -1;
        }else{
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                GBDATA *gb_id2 = GB_entry(node->gb_node, "id");
                int     id     = 0;
                if (gb_id2) id = GB_read_int(gb_id2);

                printf("deleting node w/o info: tree-node=%p; gb_node=%p prev.id=%i\n", node, node->gb_node, id);
            }
#endif /* DEBUG */
            GB_delete(node->gb_node);
            node->gb_node = 0;
        }
    }
    startid++;
    if (!node->leftson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->leftson,startid);
        if (startid<0) return startid;
    }

    if (!node->rightson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->rightson,startid);
        if (startid<0) return startid;
    }
    return startid;
}

char *gbt_write_tree_rek_new(GBDATA *gb_tree, GBT_TREE *node, char *dest, long mode) {
    char buffer[40];        /* just real numbers */
    char    *c1;

    if ( (c1 = node->remark_branch) ) {
        int c;
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'R';
            while ( (c= *(c1++))  ) {
                if (c == 1) continue;
                *(dest++) = c;
            }
            *(dest++) = 1;
        }else{
            dest += strlen(c1) + 2;
        }
    }

    if (node->is_leaf){
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'L';
            if (node->name) strcpy(dest,node->name);
            while ( (c1= (char *)strchr(dest,1)) ) *c1 = 2;
            dest += strlen(dest);
            *(dest++) = 1;
            return dest;
        }else{
            if (node->name) return dest+1+strlen(node->name)+1; /* N name term */
            return dest+1+1;
        }
    }else{
        sprintf(buffer,"%g,%g;",node->leftlen,node->rightlen);
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'N';
            strcpy(dest,buffer);
            dest += strlen(buffer);
        }else{
            dest += strlen(buffer)+1;
        }
        dest = gbt_write_tree_rek_new(gb_tree,node->leftson,dest,mode);
        dest = gbt_write_tree_rek_new(gb_tree,node->rightson,dest,mode);
        return dest;
    }
}

GB_ERROR gbt_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree, int plain_only)
{
    /* writes a tree to the database.

    If tree is loaded by function GBT_read_tree(..) then 'tree_name' should be zero !!!!!!
    else 'gb_tree' should be set to zero.

    to copy a tree call GB_copy((GBDATA *)dest,(GBDATA *)source);
    or set recursively all tree->gb_node variables to zero (that unlinks the tree),

    if 'plain_only' == 1 only the plain tree string is written

    */
    GBDATA   *gb_node,    *gb_tree_data;
    GBDATA   *gb_node_next;
    GBDATA   *gb_nnodes, *gbd;
    GB_ERROR  error;
    char     *ctree,*t_size;
    GBDATA   *gb_ctree;

    gb_assert(!plain_only || (tree_name == 0)); // if plain_only == 1 -> set tree_name to 0

    if (!tree) return 0;
    if (gb_tree && tree_name) return GB_export_error("you cannot change tree name to %s",tree_name);
    if ((!gb_tree) && (!tree_name)) return GB_export_error("please specify a tree name");

    if (tree_name) {
        error = GBT_check_tree_name(tree_name);
        if (error) return error;
        gb_tree_data = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
        gb_tree = GB_search(gb_tree_data,tree_name,GB_CREATE_CONTAINER);
    }

    if (!plain_only) {
        /* now delete all old style tree data */
        for (gb_node = GB_entry(gb_tree,"node"); gb_node; gb_node = GB_nextEntry(gb_node)) {
            GB_write_usr_private(gb_node,1);
        }
    }

    gb_ctree  = GB_search(gb_tree,"tree",GB_STRING);
    t_size    = gbt_write_tree_rek_new(gb_tree, tree, 0, GBT_GET_SIZE);
    ctree     = (char *)GB_calloc(sizeof(char),(size_t)(t_size+1));
    t_size    = gbt_write_tree_rek_new(gb_tree, tree, ctree, GBT_PUT_DATA);
    *(t_size) = 0;
    error     = GB_set_compression(gb_main,0); /* no more compressions */
    error     = GB_write_string(gb_ctree,ctree);
    error     = GB_set_compression(gb_main,-1); /* allow all types of compression */

    free(ctree);
    if (!plain_only && !error) {
        long size = gbt_write_tree_nodes(gb_tree,tree,0);
            
        if (size<0) {
            error = GB_get_error();
        }
        else {
            for (gb_node = GB_entry(gb_tree,"node"); /* delete all ghost nodes */
                 gb_node && !error;
                 gb_node = gb_node_next)
            {
                gb_node_next = GB_nextEntry(gb_node);
                gbd = GB_entry(gb_node,"id");
                if (!gbd || GB_read_usr_private(gb_node)) {
                    error = GB_delete(gb_node);
                }
            }

            if (!error) {
                gb_nnodes = GB_search(gb_tree,"nnodes",GB_INT);
                error = GB_write_int(gb_nnodes,size);
            }
        }
    }

    if (error) return error;
    return 0;
}

GB_ERROR GBT_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 0);
}
GB_ERROR GBT_write_plain_tree(GBDATA *gb_main, GBDATA *gb_tree, char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 1);
}


GB_ERROR GBT_write_tree_rem(GBDATA *gb_main,const char *tree_name, const char *remark) {
    GBDATA *ali_cont = GBT_get_tree(gb_main,tree_name);
    GBDATA *tree_rem =  GB_search(ali_cont,"remark",    GB_STRING);
    return GB_write_string(tree_rem,remark);
}
/********************************************************************************************
                    some tree read functions
********************************************************************************************/

GBT_TREE *gbt_read_tree_rek(char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree, GB_ERROR *error)
{
    GBT_TREE    *node;
    GBDATA      *gb_group_name;
    char         c;
    char        *p1;
    static char *membase;

    gb_assert(error);
    if (*error) return NULL;

    if (structure_size>0){
        node = (GBT_TREE *)GB_calloc(1,(size_t)structure_size);
    }
    else {
        if (!startid[0]){
            membase =(char *)GB_calloc(size_of_tree+1,(size_t)(-2*structure_size)); /* because of inner nodes */
        }
        node = (GBT_TREE *)membase;
        node->tree_is_one_piece_of_memory = 1;
        membase -= structure_size;
    }

    c = *((*data)++);

    if (c=='R') {
        p1 = strchr(*data,1);
        *(p1++) = 0;
        node->remark_branch = GB_STRDUP(*data);
        c = *(p1++);
        *data = p1;
    }


    if (c=='N') {
        p1 = (char *)strchr(*data,',');
        *(p1++) = 0;
        node->leftlen = GB_atof(*data);
        *data = p1;
        p1 = (char *)strchr(*data,';');
        *(p1++) = 0;
        node->rightlen = GB_atof(*data);
        *data = p1;
        if ((*startid < size_of_tree) && (node->gb_node = gb_tree_nodes[*startid])){
            gb_group_name = GB_entry(node->gb_node,"group_name");
            if (gb_group_name) {
                node->name = GB_read_string(gb_group_name);
            }
        }
        (*startid)++;
        node->leftson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree, error);
        if (!node->leftson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->rightson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree, error);
        if (!node->rightson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->leftson->father = node;
        node->rightson->father = node;
    }
    else if (c=='L') {
        node->is_leaf = GB_TRUE;
        p1            = (char *)strchr(*data,1);

        gb_assert(p1);
        gb_assert(p1[0] == 1);

        *p1        = 0;
        node->name = (char *)GB_STRDUP(*data);
        *data      = p1+1;
    }
    else {
        if (!c) {
            *error = "Unexpected end of tree definition.";
        }
        else {
            *error = GBS_global_string("Can't interpret tree definition (expected 'N' or 'L' - not '%c')", c);
        }
        /* GB_internal_error("Error reading tree 362436"); */
        return NULL;
    }
    return node;
}


/** Loads a tree from the database into any user defined structure.
    make sure that the first eight members members of your
    structure looks exectly like GBT_TREE, You should send the size
    of your structure ( minimum sizeof GBT_TREE) to this
    function.

    If size < 0 then the tree is allocated as just one big piece of memery,
    which can be freed by free((char *)root_of_tree) + deleting names or
    by GBT_delete_tree.

    tree_name is the name of the tree in the db
    return NULL if any error occur
*/

static GBT_TREE *read_tree_and_size_internal(GBDATA *gb_tree, GBDATA *gb_ctree, int structure_size, int size, GB_ERROR *error) {
    GBDATA   **gb_tree_nodes;
    GBT_TREE  *node = 0;

    gb_tree_nodes = (GBDATA **)GB_calloc(sizeof(GBDATA *),(size_t)size);
    if (gb_tree) {
        GBDATA *gb_node;

        for (gb_node = GB_entry(gb_tree,"node"); gb_node && !*error; gb_node = GB_nextEntry(gb_node)) {
            long    i;
            GBDATA *gbd = GB_entry(gb_node,"id");
            if (!gbd) continue;

            /*{ GB_export_error("ERROR while reading tree '%s' 4634",tree_name);return NULL;}*/
            i = GB_read_int(gbd);
            if ( i<0 || i>= size ) {
                *error = "An inner node of the tree is corrupt";
            }
            else {
                gb_tree_nodes[i] = gb_node;
            }
        }
    }
    if (!*error) {
        char *cptr[1];
        long  startid[1];
        char *fbuf;

        startid[0] = 0;
        fbuf       = cptr[0] = GB_read_string(gb_ctree);
        node       = gbt_read_tree_rek(cptr, startid, gb_tree_nodes, structure_size,(int)size, error);
        free (fbuf);
    }

    free((char *)gb_tree_nodes);

    return node;
}

GBT_TREE *GBT_read_tree_and_size(GBDATA *gb_main,const char *tree_name, long structure_size, int *tree_size) {
    /* read a tree from DB */
    GB_ERROR error = 0;

    if (!tree_name) {
        error = "no treename given";
    }
    else {
        error = GBT_check_tree_name(tree_name);
        if (!error) {
            GBDATA *gb_tree_data = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
            GBDATA *gb_tree      = GB_entry(gb_tree_data, tree_name);

            if (!gb_tree) {
                error = GBS_global_string("Could not find tree '%s'", tree_name);
            }
            else {
                GBDATA *gb_nnodes = GB_entry(gb_tree, "nnodes");
                if (!gb_nnodes) {
                    error = GBS_global_string("Tree '%s' is empty", tree_name);
                }
                else {
                    long size = GB_read_int(gb_nnodes);
                    if (!size) {
                        error = GBS_global_string("Tree '%s' has zero size", tree_name);
                    }
                    else {
                        GBDATA *gb_ctree = GB_search(gb_tree, "tree", GB_FIND);
                        if (!gb_ctree) {
                            error = "Sorry - old tree format no longer supported";
                        }
                        else { /* "new" style tree */
                            GBT_TREE *tree = read_tree_and_size_internal(gb_tree, gb_ctree, structure_size, size, &error);
                            if (!error) {
                                gb_assert(tree);
                                if (tree_size) *tree_size = size; /* return size of tree */
                                return tree;
                            }

                            gb_assert(!tree);
                        }
                    }
                }
            }
        }
    }

    gb_assert(error);
    GB_export_error("Couldn't read tree '%s' (Reason: %s)", tree_name, error);
    return NULL;
}

GBT_TREE *GBT_read_tree(GBDATA *gb_main,const char *tree_name, long structure_size) {
    return GBT_read_tree_and_size(gb_main, tree_name, structure_size, 0);
}

GBT_TREE *GBT_read_plain_tree(GBDATA *gb_main, GBDATA *gb_ctree, long structure_size, GB_ERROR *error) {
    GBT_TREE *t;

    gb_assert(error && !*error); /* expect cleared error*/

    GB_push_transaction(gb_main);
    t = read_tree_and_size_internal(0, gb_ctree, structure_size, 0, error);
    GB_pop_transaction(gb_main);

    return t;
}

/********************************************************************************************
                    link the tree tips to the database
********************************************************************************************/
long GBT_count_nodes(GBT_TREE *tree){
    if ( tree->is_leaf ) {
        return 1;
    }
    return GBT_count_nodes(tree->leftson) + GBT_count_nodes(tree->rightson);
}

struct link_tree_data {
    GB_HASH *species_hash;
    GB_HASH *seen_species;      /* used to count duplicates */
    int      nodes; /* if != 0 -> update status */;
    int      counter;
    int      zombies;           /* counts zombies */
    int      duplicates;        /* counts duplicates */
};

static GB_ERROR gbt_link_tree_to_hash_rek(GBT_TREE *tree, struct link_tree_data *ltd) {
    GB_ERROR error = 0;
    if (tree->is_leaf) {
        if (ltd->nodes) { /* update status? */
            GB_status(ltd->counter/(double)ltd->nodes);
            ltd->counter++;
        }

        tree->gb_node = 0;
        if (tree->name) {
            GBDATA *gbd = (GBDATA*)GBS_read_hash(ltd->species_hash, tree->name);
            if (gbd) tree->gb_node = gbd;
            else ltd->zombies++;

            if (ltd->seen_species) {
                if (GBS_read_hash(ltd->seen_species, tree->name)) ltd->duplicates++;
                else GBS_write_hash(ltd->seen_species, tree->name, 1);
            }
        }
    }
    else {
        error = gbt_link_tree_to_hash_rek(tree->leftson, ltd);
        if (!error) error = gbt_link_tree_to_hash_rek(tree->rightson, ltd);
    }
    return error;
}

GB_ERROR GBT_link_tree_using_species_hash(GBT_TREE *tree, GB_BOOL show_status, GB_HASH *species_hash, int *zombies, int *duplicates) {
    GB_ERROR              error;
    struct link_tree_data ltd;
    long                  leafs = 0;

    if (duplicates || show_status) {
        leafs = GBT_count_nodes(tree);
    }
    
    ltd.species_hash = species_hash;
    ltd.seen_species = leafs ? GBS_create_hash(2*leafs, GB_IGNORE_CASE) : 0;
    ltd.zombies      = 0;
    ltd.duplicates   = 0;
    ltd.counter      = 0;

    if (show_status) {
        GB_status2("Relinking tree to database");
        ltd.nodes = leafs;
    }
    else {
        ltd.nodes = 0;
    }

    error = gbt_link_tree_to_hash_rek(tree, &ltd);
    if (ltd.seen_species) GBS_free_hash(ltd.seen_species);

    if (zombies) *zombies = ltd.zombies;
    if (duplicates) *duplicates = ltd.duplicates;

    return error;
}

/** Link a given tree to the database. That means that for all tips the member
    gb_node is set to the database container holding the species data.
    returns the number of zombies and duplicates in 'zombies' and 'duplicates'
*/
GB_ERROR GBT_link_tree(GBT_TREE *tree,GBDATA *gb_main,GB_BOOL show_status, int *zombies, int *duplicates)
{
    GB_HASH  *species_hash = GBT_create_species_hash(gb_main);
    GB_ERROR  error        = GBT_link_tree_using_species_hash(tree, show_status, species_hash, zombies, duplicates);

    GBS_free_hash(species_hash);

    return error;
}

/** Unlink a given tree from the database.
 */
void GBT_unlink_tree(GBT_TREE *tree)
{
    tree->gb_node = 0;
    if (!tree->is_leaf) {
        GBT_unlink_tree(tree->leftson);
        GBT_unlink_tree(tree->rightson);
    }
}



/********************************************************************************************
                    load a tree from file system
********************************************************************************************/

#define MAX_COMMENT_SIZE 1024

#define GBT_READ_CHAR(io,c)                                                     \
for (c=' '; (c==' ') || (c=='\n') || (c=='\t')|| (c=='[') ;){                   \
    c=getc(io);                                                                 \
    if (c == '\n' ) gbt_line_cnt++;                                             \
    if (c == '[') {                                                             \
        if (gbt_tree_comment_size && gbt_tree_comment_size<MAX_COMMENT_SIZE){   \
            gbt_tree_comment[gbt_tree_comment_size++] = '\n';                   \
        }                                                                       \
        c = getc(io);                                                           \
        for (; (c!=']') && (c!=EOF); c = getc(io)) {                            \
            if (gbt_tree_comment_size<MAX_COMMENT_SIZE) {                       \
                gbt_tree_comment[gbt_tree_comment_size++] = c;                  \
            }                                                                   \
        }                                                                       \
        c                                     = ' ';                            \
    }                                                                           \
}                                                                               \
gbt_last_character = c;

#define GBT_GET_CHAR(io,c)                      \
c = getc(io);                                   \
if (c == '\n' ) gbt_line_cnt++;                 \
gbt_last_character = c;

static int  gbt_last_character    = 0;
static int  gbt_line_cnt          = 0;
static char gbt_tree_comment[MAX_COMMENT_SIZE+1]; // all comments from a tree file are collected here
static int  gbt_tree_comment_size = 0; // all comments from a tree file are collected here

/* ----------------------------------------------- */
/*      static void clear_tree_comment(void)       */
/* ----------------------------------------------- */
static void clear_tree_comment(void) {
    gbt_tree_comment_size = 0;
}

/* ---------------------------------------------- */
/*      double gbt_read_number(FILE * input)      */
/* ---------------------------------------------- */
double gbt_read_number(FILE * input)
{
    char            strng[256];
    char           *s;
    int             c;
    double          fl;
    s = &(strng[0]);
    c = gbt_last_character;
    while (((c <= '9') && (c >= '0')) || (c == '.') || (c == '-') || (c=='e') || (c=='E') ) {
        *(s++) = c;
        c = getc(input);
    }
    while ((c == ' ') || (c == '\n') || (c == '\t')){
        if (c == '\n' ) gbt_line_cnt++;
        c = getc(input);
    }
    gbt_last_character = c;
    *s = 0;
    fl = GB_atof(strng);
    return fl;
}

/** Read in a quoted or unquoted string. in quoted strings double quotes ('') are replaced by (') */
char *gbt_read_quoted_string(FILE *input){
    char buffer[1024];
    int c;
    char *s;
    s = buffer;
    c = gbt_last_character;
    if ( c == '\'' ) {
        GBT_GET_CHAR(input,c);
        while ( (c!= EOF) && (c!='\'') ) {
        gbt_lt_double_quot:
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                GB_export_error("Error while reading tree: Name '%s' longer than 1000 bytes",buffer);
                return NULL;
            }
            GBT_GET_CHAR(input,c);
        }
        if (c == '\'') {
            GBT_READ_CHAR(input,c);
            if (c == '\'') goto gbt_lt_double_quot;
        }
    }else{
        while ( c== '_') GBT_READ_CHAR(input,c);
        while ( c== ' ') GBT_READ_CHAR(input,c);
        while ( (c != ':') && (c!= EOF) && (c!=',') &&
                (c!=';') && (c!= ')') )
        {
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                GB_export_error("Error while reading tree: Name '%s' longer than 1000 bytes",buffer);
                return NULL;
            }
            GBT_READ_CHAR(input,c);
        }
    }
    *s = 0;
    return GB_STRDUP(buffer);
}

/* ---------------------------------------------------------------- */
/*      static void setBranchName(GBT_TREE *node, char *name)       */
/* ---------------------------------------------------------------- */
/* detect bootstrap values */
/* name has to be stored in node or must be free'ed */

static double max_found_bootstrap = -1;
static double max_found_branchlen = -1;

static void setBranchName(GBT_TREE *node, char *name) {
    char   *end       = 0;
    double  bootstrap = strtod(name, &end);

    if (end == name) {          // no digits -> no bootstrap
        node->name = name;
    }
    else {
        bootstrap = bootstrap*100.0 + 0.5; // needed if bootstrap values are between 0.0 and 1.0 */
        // downscaling in done later!

        if (bootstrap>max_found_bootstrap) {
            max_found_bootstrap = bootstrap;
        }

        assert(node->remark_branch == 0);
        node->remark_branch  = GB_strdup(GBS_global_string("%i%%", (int)bootstrap));

        if (end[0] != 0) {      // sth behind bootstrap value
            if (end[0] == ':') ++end; // ARB format for nodes with bootstraps AND node name is 'bootstrap:nodename'
            node->name = GB_strdup(end);
        }
        free(name);
    }
}

static GBT_TREE *gbt_load_tree_rek(FILE *input,int structuresize, const char *tree_file_name)
{
    int             c;
    GB_BOOL     loop_flag;
    GBT_TREE *nod,*left,*right;

    if ( gbt_last_character == '(' ) {  /* make node */

        nod = (GBT_TREE *)GB_calloc(structuresize,1);
        GBT_READ_CHAR(input,c);
        left = gbt_load_tree_rek(input,structuresize, tree_file_name);
        if (!left ) return NULL;
        nod->leftson = left;
        left->father = nod;
        if (    gbt_last_character != ':' &&
                gbt_last_character != ',' &&
                gbt_last_character != ';' &&
                gbt_last_character != ')'
                ) {
            char *str = gbt_read_quoted_string(input);
            if (!str) return NULL;

            setBranchName(left, str);
            /* left->name = str; */
        }
        if (gbt_last_character !=  ':') {
            nod->leftlen = DEFAULT_LENGTH_MARKER;
        }else{
            GBT_READ_CHAR(input,c);
            nod->leftlen = gbt_read_number(input);
            if (nod->leftlen>max_found_branchlen) {
                max_found_branchlen = nod->leftlen;
            }
        }
        if ( gbt_last_character == ')' ) {  /* only a single node !!!!, skip this node */
            GB_FREE(nod);           /* delete superflous father node */
            left->father = NULL;
            return left;
        }
        if ( gbt_last_character != ',' ) {
            GB_export_error ( "error in '%s':  ',' expected '%c' found; line %i",
                              tree_file_name,
                              gbt_last_character,
                              gbt_line_cnt);
            return NULL;
        }
        loop_flag = GB_FALSE;
        while (gbt_last_character == ',') {
            if (loop_flag==GB_TRUE) {       /* multi branch tree */
                left = nod;
                nod = (GBT_TREE *)GB_calloc(structuresize,1);
                nod->leftson = left;
                nod->leftson->father = nod;
            }else{
                loop_flag = GB_TRUE;
            }
            GBT_READ_CHAR(input, c);
            right = gbt_load_tree_rek(input,structuresize, tree_file_name);
            if (right == NULL) return NULL;
            nod->rightson = right;
            right->father = nod;
            if (    gbt_last_character != ':' &&
                    gbt_last_character != ',' &&
                    gbt_last_character != ';' &&
                    gbt_last_character != ')'
                    ) {
                char *str = gbt_read_quoted_string(input);
                if (!str) return NULL;
                setBranchName(right, str);
                /* right->name = str; */
            }
            if (gbt_last_character != ':') {
                nod->rightlen = DEFAULT_LENGTH_MARKER;
            }else{
                GBT_READ_CHAR(input, c);
                nod->rightlen = gbt_read_number(input);
                if (nod->rightlen>max_found_branchlen) {
                    max_found_branchlen = nod->rightlen;
                }
            }
        }
        if ( gbt_last_character != ')' ) {
            GB_export_error ( "error in '%s':  ')' expected '%c' found: line %i ",
                              tree_file_name,
                              gbt_last_character,gbt_line_cnt);
            return NULL;
        }
        GBT_READ_CHAR(input,c);     /* remove the ')' */

    }else{
        char *str = gbt_read_quoted_string(input);
        if (!str) return NULL;
        nod = (GBT_TREE *)GB_calloc(structuresize,1);
        nod->is_leaf = GB_TRUE;
        nod->name = str;
        return nod;
    }
    return nod;
}

void GBT_scale_tree(GBT_TREE *tree, double length_scale, double bootstrap_scale) {
    if (tree->leftson) {
        if (tree->leftlen <= DEFAULT_LENGTH_MARKER) tree->leftlen  = DEFAULT_LENGTH;
        else                                        tree->leftlen *= length_scale;
        
        GBT_scale_tree(tree->leftson, length_scale, bootstrap_scale);
    }
    if (tree->rightson) {
        if (tree->rightlen <= DEFAULT_LENGTH_MARKER) tree->rightlen  = DEFAULT_LENGTH; 
        else                                         tree->rightlen *= length_scale;
        
        GBT_scale_tree(tree->rightson, length_scale, bootstrap_scale);
    }

    if (tree->remark_branch) {
        const char *end          = 0;
        double      bootstrap    = strtod(tree->remark_branch, (char**)&end);
        GB_BOOL     is_bootstrap = end[0] == '%' && end[1] == 0;

        free(tree->remark_branch);
        tree->remark_branch = 0;

        if (is_bootstrap) {
            bootstrap = bootstrap*bootstrap_scale+0.5;
            tree->remark_branch  = GB_strdup(GBS_global_string("%i%%", (int)bootstrap));
        }
    }
}

/* Load a newick compatible tree from file 'path',
   structure size should be >0, see GBT_read_tree for more information
   if commentPtr != NULL -> set it to a malloc copy of all concatenated comments found in tree file
   if warningPtr != NULL -> set it to a malloc copy auto-scale-warning (if autoscaling happens) 
*/
GBT_TREE *GBT_load_tree(const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr)
{
    FILE        *input;
    GBT_TREE    *tree;
    int     c;
    if ((input = fopen(path, "r")) == NULL) {
        GB_export_error("Import tree: file '%s' not found", path);
        return NULL;
    }

    clear_tree_comment();

    GBT_READ_CHAR(input,c);
    gbt_line_cnt = 1;
    {
        const char *name_only = strrchr(path, '/');
        if (name_only) ++name_only;
        else name_only = path;

        max_found_bootstrap = -1;
        max_found_branchlen = -1;
        tree                = gbt_load_tree_rek(input,structuresize, name_only);

        {
            double bootstrap_scale = 1.0;
            double branchlen_scale = 1.0;

            if (max_found_bootstrap >= 101.0) { // bootstrap values were given in percent
                bootstrap_scale = 0.01;
                if (warningPtr) {
                    *warningPtr = GBS_global_string_copy("Auto-scaling bootstrap values by factor %.2f (max. found bootstrap was %5.2f)",
                                                         bootstrap_scale, max_found_bootstrap);
                }
            }
            if (max_found_branchlen >= 1.01) { // assume branchlengths have range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                    if (warningPtr) {
                        char *w = GBS_global_string_copy("Auto-scaling branchlengths by factor %.2f (max. found branchlength = %5.2f)",
                                                         branchlen_scale, max_found_branchlen);
                        if (*warningPtr) {
                            char *w2 = GBS_global_string_copy("%s\n%s", *warningPtr, w);

                            free(*warningPtr);
                            free(w);
                            *warningPtr = w2;
                        }
                        else {
                            *warningPtr = w;
                        }
                    }
                }
            }

            GBT_scale_tree(tree, branchlen_scale, bootstrap_scale); // scale bootstraps and branchlengths
        }
    }
    fclose(input);

    if (commentPtr) {
        assert(*commentPtr == 0);
        if (gbt_tree_comment_size) *commentPtr = GB_STRDUP(gbt_tree_comment);
    }

    return tree;
}

GBDATA *GBT_get_tree(GBDATA *gb_main, const char *tree_name) {
    /* returns the datapntr to the database structure, which is the container for the tree */
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    gb_tree = GB_entry(gb_treedata, tree_name) ;
    return gb_tree;
}

long GBT_size_of_tree(GBDATA *gb_main, const char *tree_name) {
    GBDATA *gb_tree = GBT_get_tree(gb_main,tree_name);
    GBDATA *gb_nnodes;
    if (!gb_tree) return -1;
    gb_nnodes = GB_entry(gb_tree,"nnodes");
    if (!gb_nnodes) return -1;
    return GB_read_int(gb_nnodes);
}

char *GBT_find_largest_tree(GBDATA *gb_main){
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    GBDATA *gb_nnodes;
    long nnodes;
    char *largest = 0;
    long maxnodes = 0;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    for (gb_tree = GB_child(gb_treedata);
         gb_tree;
         gb_tree = GB_nextChild(gb_tree))
    {
        gb_nnodes = GB_entry(gb_tree,"nnodes");
        if (!gb_nnodes) continue;
        nnodes = GB_read_int(gb_nnodes);
        if (nnodes> maxnodes) {
            if (largest) free(largest);
            largest = GB_read_key(gb_tree);
            maxnodes = nnodes;
        }
    }
    return largest;
}

char *GBT_find_latest_tree(GBDATA *gb_main){
    char **names = GBT_get_tree_names(gb_main);
    char *name = 0;
    char **pname;
    if (!names) return NULL;
    for (pname = names;*pname;pname++) name = *pname;
    if (name) name = strdup(name);
    GBT_free_names(names);
    return name;
}

const char *GBT_tree_info_string(GBDATA *gb_main, const char *tree_name, int maxTreeNameLen) {
    /* maxTreeNameLen shall be the max len of the longest tree name (or -1 -> do not format) */

    const char *result  = 0;
    GBDATA     *gb_tree = GBT_get_tree(gb_main,tree_name);
    
    if (!gb_tree) {
        GB_export_error("tree '%s' not found",tree_name);
    }
    else {
        GBDATA *gb_nnodes = GB_entry(gb_tree,"nnodes");
        if (!gb_nnodes) {
            GB_export_error("nnodes not found in tree '%s'",tree_name);
        }
        else {
            const char *sizeInfo = GBS_global_string("(%li:%i)", GB_read_int(gb_nnodes), GB_read_security_write(gb_tree));
            GBDATA     *gb_rem   = GB_entry(gb_tree,"remark");
            int         len;

            if (maxTreeNameLen == -1) {
                result = GBS_global_string("%s %11s", tree_name, sizeInfo);
                len    = strlen(tree_name);
            }
            else {
                result = GBS_global_string("%-*s %11s", maxTreeNameLen, tree_name, sizeInfo);
                len    = maxTreeNameLen;
            }
            if (gb_rem) {
                const char *remark    = GB_read_char_pntr(gb_rem);
                const int   remarkLen = 800;
                char       *res2      = GB_give_other_buffer(remark, len+1+11+2+remarkLen+1);

                strcpy(res2, result);
                strcat(res2, "  ");
                strncat(res2, remark, remarkLen);

                result = res2;
            }
        }
    }
    return result;
}

GB_ERROR GBT_check_tree_name(const char *tree_name)
{
    GB_ERROR error;
    if ( (error = GB_check_key(tree_name)) ) return error;
    if (strncmp(tree_name,"tree_",5)){
        return GB_export_error("your treename '%s' does not begin with 'tree_'",tree_name);
    }
    return 0;
}

char **GBT_get_tree_names_and_count(GBDATA *Main, int *countPtr){
    /* returns an null terminated array of string pointers */

    int      count       = 0;
    GBDATA  *gb_treedata = GB_entry(Main,"tree_data");
    char   **erg         = 0;

    if (gb_treedata) {
        GBDATA *gb_tree;
        count = 0;

        for (gb_tree = GB_child(gb_treedata);
             gb_tree;
             gb_tree = GB_nextChild(gb_tree))
        {
            count ++;
        }

        if (count) {
            erg   = (char **)GB_calloc(sizeof(char *),(size_t)count+1);
            count = 0;

            for (gb_tree = GB_child(gb_treedata);
                 gb_tree;
                 gb_tree = GB_nextChild(gb_tree) )
            {
                erg[count] = GB_read_key(gb_tree);
                count ++;
            }
        }
    }

    *countPtr = count;
    return erg;
}

char **GBT_get_tree_names(GBDATA *Main){
    int dummy;
    return GBT_get_tree_names_and_count(Main, &dummy);
}

char *GBT_get_next_tree_name(GBDATA *gb_main, const char *tree){
    GBDATA *gb_treedata;
    GBDATA *gb_tree = 0;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    if (tree){
        gb_tree = GB_entry(gb_treedata,tree);
    }
    if (gb_tree){
        gb_tree = GB_nextEntry(gb_tree);
    }else{
        gb_tree = GB_child(gb_treedata);
    }
    if (gb_tree) return GB_read_key(gb_tree);
    return NULL;
}

GB_ERROR GBT_free_names(char **names)
{
    char **pn;
    for (pn = names; *pn;pn++) free(*pn);
    free((char *)names);
    return 0;
}

int gbt_sum_leafs(GBT_TREE *tree){
    if (tree->is_leaf){
        return 1;
    }
    return gbt_sum_leafs(tree->leftson) + gbt_sum_leafs(tree->rightson);
}

GB_CSTR *gbt_fill_species_names(GB_CSTR *des,GBT_TREE *tree) {
    if (tree->is_leaf){
        des[0] = tree->name;
        return des+1;
    }
    des = gbt_fill_species_names(des,tree->leftson);
    des = gbt_fill_species_names(des,tree->rightson);
    return des;
}

/* creates an array of all species names in a tree,
   the names is not strdupped !!! */

GB_CSTR *GBT_get_species_names_of_tree(GBT_TREE *tree){
    int size = gbt_sum_leafs(tree);
    GB_CSTR *result = (GB_CSTR *)GB_calloc(sizeof(char *),size +1);
#if defined(DEBUG)
    GB_CSTR *check =
#endif // DEBUG
        gbt_fill_species_names(result,tree);
    assert(check - size == result);
    return result;
}

/* search for an existing or an alternate tree */
char *GBT_existing_tree(GBDATA *Main, const char *tree) {
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    gb_treedata = GB_entry(Main,"tree_data");
    if (!gb_treedata) return NULL;
    gb_tree = GB_entry(gb_treedata,tree);
    if (gb_tree) return GB_STRDUP(tree);
    gb_tree = GB_child(gb_treedata);
    if (!gb_tree) return NULL;
    return GB_read_key(gb_tree);
}

void gbt_export_tree_node_print_remove(char *str){
    int i,len;
    len = strlen (str);
    for(i=0;i<len;i++) {
        if (str[i] =='\'') str[i] ='.';
        if (str[i] =='\"') str[i] ='.';
    }
}

void gbt_export_tree_rek(GBT_TREE *tree,FILE *out){
    if (tree->is_leaf) {
        gbt_export_tree_node_print_remove(tree->name);
        fprintf(out," '%s' ",tree->name);
    }else{
        fprintf(out, "(");
        gbt_export_tree_rek(tree->leftson,out);
        fprintf(out, ":%.5f,", tree->leftlen);
        gbt_export_tree_rek(tree->rightson,out);
        fprintf(out, ":%.5f", tree->rightlen);
        fprintf(out, ")");
        if (tree->name){
            fprintf(out, "'%s'",tree->name);
        }
    }
}


GB_ERROR GBT_export_tree(GBDATA *gb_main,FILE *out,GBT_TREE *tree, GB_BOOL triple_root){
    GBUSE(gb_main);
    if(triple_root){
        GBT_TREE *one,*two,*three;
        if (tree->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf && tree->rightson->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf){
            one = tree->leftson;
            two = tree->rightson->leftson;
            three = tree->rightson->rightson;
        }else{
            one = tree->leftson->leftson;
            two = tree->leftson->rightson;
            three = tree->rightson;
        }
        fprintf(out, "(");
        gbt_export_tree_rek(one,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(two,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(three,out);
        fprintf(out, ":%.5f)", 1.0);
    }else{
        gbt_export_tree_rek(tree,out);
    }
    return 0;
}

/********************************************************************************************
                    species functions
********************************************************************************************/


GBDATA *GBT_create_species(GBDATA *gb_main,const char *name)
{
    /* Search for a species, when species do not exist create it */
    GBDATA *species;
    GBDATA *gb_name;
    GBDATA *gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    species = GB_find_string(gb_species_data,"name",name,GB_IGNORE_CASE,down_2_level);
    if (species) return GB_get_father(species);
    if ((int)strlen(name) <2) {
        GB_export_error("create species failed: too short name '%s'",name);
        return NULL;
    }
    species = GB_create_container(gb_species_data,"species");
    gb_name = GB_create(species,"name",GB_STRING);
    GB_write_string(gb_name,name);
    GB_write_flag(species,1);
    return species;
}
GBDATA *GBT_create_species_rel_species_data(GBDATA *gb_species_data,const char *name)
{
    /* Search for a species, when species do not exist create it */
    GBDATA *species;
    GBDATA *gb_name;
    species = GB_find_string(gb_species_data,"name",name,GB_IGNORE_CASE,down_2_level);
    if (species) return GB_get_father(species);
    if ((int)strlen(name) <2) {
        GB_export_error("create species failed: too short name '%s'",name);
        return NULL;
    }
    species = GB_create_container(gb_species_data,"species");
    gb_name = GB_create(species,"name",GB_STRING);
    GB_write_string(gb_name,name);
    GB_write_flag(species,1);
    return species;
}

GBDATA *GBT_create_SAI(GBDATA *gb_main,const char *name)
{
    /* Search for an extended, when extended do not exist create it */
    GBDATA *extended;
    GBDATA *gb_name;
    GBDATA *gb_extended_data = GB_search(gb_main, "extended_data", GB_CREATE_CONTAINER);
    extended = GB_find_string(gb_extended_data,"name",name,GB_IGNORE_CASE,down_2_level);
    if (extended) return GB_get_father(extended);
    if ((int)strlen(name) <2) {
        GB_export_error("create SAI failed: too short name '%s'",name);
        return NULL;
    }
    extended = GB_create_container(gb_extended_data,"extended");
    gb_name = GB_create(extended,"name",GB_STRING);
    GB_write_string(gb_name,name);
    GB_write_flag(extended,1);
    return extended;
}

GBDATA *GBT_add_data(GBDATA *species,const char *ali_name, const char *key, GB_TYPES type)
{
    /* the same as GB_search(species, 'ali_name/key', GB_CREATE) */
    GBDATA *gb_gb;
    GBDATA *gb_data;
    if (GB_check_key(ali_name)) {
        return NULL;
    }
    if (GB_check_hkey(key)) {
        return NULL;
    }
    gb_gb = GB_entry(species,ali_name);
    if (!gb_gb) gb_gb = GB_create_container(species,ali_name);

    if (type == GB_STRING) {
        gb_data = GB_search(gb_gb, key, GB_FIND);
        if (!gb_data){
            gb_data = GB_search(gb_gb, key, GB_STRING);
            GB_write_string(gb_data,"...");
        }
    }else{
        gb_data = GB_search(gb_gb, key, type);
    }
    return gb_data;
}


GB_ERROR GBT_write_sequence(GBDATA *gb_data, const char *ali_name, long ali_len, const char *sequence) {
    /* writes a sequence which is generated by GBT_add_data,
     * cuts sequence after alignment len only if bases e ".-nN" */
    int slen = strlen(sequence);
    int old_char = 0;
    GB_ERROR error = 0;
    if (slen > ali_len) {
        int i;
        for (i= slen -1; i>=ali_len; i--) {
            if (!strchr("-.nN",sequence[i])) break;     /* real base after end of alignment */
        }
        i++;                            /* points to first 0 after alignment */
        if (i > ali_len){
            GBDATA *gb_main = GB_get_root(gb_data);
            ali_len = GBT_get_alignment_len(gb_main,ali_name);
            if (slen > ali_len){                /* check for modified alignment len */
                GBT_set_alignment_len(gb_main,ali_name,i);
                ali_len = i;
            }
        }
        if (slen > ali_len){
            old_char = sequence[ali_len];
            ((char*)sequence)[ali_len] = 0;
        }
    }
    error = GB_write_string(gb_data,sequence);
    if (slen> ali_len) ((char*)sequence)[ali_len] = old_char;
    return error;
}


GBDATA *GBT_gen_accession_number(GBDATA *gb_species,const char *ali_name){
    GBDATA *gb_acc;
    GBDATA *gb_data;
    GB_CSTR sequence;
    char buf[100];
    long id;

    gb_acc = GB_entry(gb_species,"acc");
    if (gb_acc) return gb_acc;
    /* Search a valid alignment */
    gb_data = GBT_read_sequence(gb_species,ali_name);
    if (!gb_data) return NULL;
    sequence = GB_read_char_pntr(gb_data);
    id = GBS_checksum(sequence,1,".-");
    sprintf(buf,"ARB_%lX",id);
    gb_acc = GB_search(gb_species,"acc",GB_STRING);
    GB_write_string(gb_acc,buf);
    return gb_acc;
}


int GBT_is_partial(GBDATA *gb_species, int default_value, int define_if_undef) {
    // checks whether a species has a partial or full sequence
    //
    // Note: partial sequences should not be used for tree calculations
    //
    // returns: 0 if sequence is full
    //          1 if sequence is partial
    //          -1 in case of error
    //
    // if the sequence has no 'ARB_partial' entry it returns 'default_value'
    // if 'define_if_undef' is true then create an 'ARB_partial'-entry with the default value

    int       result     = -1;
    GB_ERROR  error      = 0;
    GBDATA   *gb_partial = GB_entry(gb_species, "ARB_partial");

    if (gb_partial) {
        result = GB_read_int(gb_partial);
        if (result != 0 && result != 1) {
            error = "Illegal value for 'ARB_partial' (only 1 or 0 allowed)";
        }
    }
    else {
        if (define_if_undef) {
            gb_partial = GB_create(gb_species, "ARB_partial", GB_INT);
            if (gb_partial) error = GB_write_int(gb_partial, default_value);
            else error = GB_get_error();
        }

        result = default_value;
    }

    if (error) {
        GB_export_error(error);
        return -1;
    }
    return result;
}

/********************************************************************************************
                    some simple find procedures
********************************************************************************************/
GBDATA *GBT_get_species_data(GBDATA *gb_main) {
    return GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
}

GBDATA *GBT_first_marked_species_rel_species_data(GBDATA *gb_species_data) {
    return GB_first_marked(gb_species_data,"species");
}

GBDATA *GBT_first_marked_species(GBDATA *gb_main) {
    return GB_first_marked(GBT_get_species_data(gb_main), "species");
}
GBDATA *GBT_next_marked_species(GBDATA *gb_species) {
    return GB_next_marked(gb_species,"species");
}

GBDATA *GBT_first_species_rel_species_data(GBDATA *gb_species_data) {
    return GB_entry(gb_species_data,"species");
}
GBDATA *GBT_first_species(GBDATA *gb_main) {
    return GB_entry(GBT_get_species_data(gb_main),"species");
}

GBDATA *GBT_next_species(GBDATA *gb_species) {
    gb_assert(GB_has_key(gb_species, "species"));
    return GB_nextEntry(gb_species);
}

GBDATA *GBT_find_species_rel_species_data(GBDATA *gb_species_data,const char *name) {
    GBDATA *gb_species_name = GB_find_string(gb_species_data,"name",name,GB_IGNORE_CASE,down_2_level);
    return gb_species_name ? GB_get_father(gb_species_name) : 0;
}
GBDATA *GBT_find_species(GBDATA *gb_main,const char *name) {
    return GBT_find_species_rel_species_data(GBT_get_species_data(gb_main), name);
}

/* --------------------- */
/*      count items      */
/* --------------------- */

long GBT_get_item_count(GBDATA *gb_main, const char *item_container_name) {
    GBDATA *gb_item_data;
    long    count = 0;

    GB_push_transaction(gb_main);
    gb_item_data = GB_entry(gb_main, item_container_name);
    if (gb_item_data) count = GB_number_of_subentries(gb_item_data);
    GB_pop_transaction(gb_main);

    return count;
}

long GBT_get_species_count(GBDATA *gb_main) {
    return GBT_get_item_count(gb_main, "species_data");
}

long GBT_get_SAI_count(GBDATA *gb_main) {
    return GBT_get_item_count(gb_main, "extended_data");
}

/* -------------------------------------------------------------------------------- */

GBDATA *GBT_first_marked_extended(GBDATA *gb_extended_data)
{
    GBDATA *gb_extended;
    for (gb_extended = GB_entry(gb_extended_data,"extended");
         gb_extended;
         gb_extended = GB_nextEntry(gb_extended))
    {
        if (GB_read_flag(gb_extended)) return gb_extended;
    }
    return NULL;
}

GBDATA *GBT_next_marked_extended(GBDATA *gb_extended)
{
    while ((gb_extended = GB_nextEntry(gb_extended))) {
        if (GB_read_flag(gb_extended)) return gb_extended;
    }
    return NULL;
}
/* Search SAIs */
GBDATA *GBT_first_SAI(GBDATA *gb_main) {
    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    GBDATA *gb_extended      = GB_entry(gb_extended_data,"extended");
    return gb_extended;
}

GBDATA *GBT_next_SAI(GBDATA *gb_extended) {
    gb_assert(GB_has_key(gb_extended, "extended"));
    return GB_nextEntry(gb_extended);
}

GBDATA *GBT_find_SAI(GBDATA *gb_main,const char *name)
{
    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    return GBT_find_species_rel_species_data(gb_extended_data,name);
}
/* Search SAIs rel extended_data */

GBDATA *GBT_first_SAI_rel_exdata(GBDATA *gb_extended_data) {
    return GB_entry(gb_extended_data,"extended");
}

GBDATA *GBT_find_SAI_rel_exdata(GBDATA *gb_extended_data, const char *name) {
    return GBT_find_species_rel_species_data(gb_extended_data,name);
}

char *GBT_create_unique_species_name(GBDATA *gb_main,const char *default_name){
    GBDATA *gb_species;
    int c = 1;
    char *buffer;
    gb_species = GBT_find_species(gb_main,default_name);
    if (!gb_species) return strdup(default_name);

    buffer = (char *)GB_calloc(sizeof(char),strlen(default_name)+10);
    while (gb_species){
        sprintf(buffer,"%s_%i",default_name,c++);
        gb_species = GBT_find_species(gb_main,buffer);
    }
    return buffer;
}

/********************************************************************************************
                    mark and unmark species
********************************************************************************************/
void GBT_mark_all(GBDATA *gb_main, int flag)
{
    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    if (flag == 2) {
        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            GB_write_flag(gb_species,!GB_read_flag(gb_species));
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            GB_write_flag(gb_species,flag);
        }
    }
    GB_pop_transaction(gb_main);
}
void GBT_mark_all_that(GBDATA *gb_main, int flag, int (*condition)(GBDATA*, void*), void *cd)
{
    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    if (flag == 2) {
        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            if (condition(gb_species, cd)) {
                GB_write_flag(gb_species,!GB_read_flag(gb_species));
            }
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            int curr_flag = GB_read_flag(gb_species);
            if (curr_flag != flag && condition(gb_species, cd)) {
                GB_write_flag(gb_species,flag);
            }
        }
    }
    GB_pop_transaction(gb_main);
}

long GBT_count_marked_species(GBDATA *gb_main)
{
    long    cnt = 0;
    GBDATA *gb_species_data;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GB_pop_transaction(gb_main);

    cnt = GB_number_of_marked_subentries(gb_species_data);
    return cnt;
}

char *GBT_store_marked_species(GBDATA *gb_main, int unmark_all)
{
    /* stores the currently marked species in a string
       if (unmark_all != 0) then unmark them too
    */

    void   *out   = GBS_stropen(10000);
    GBDATA *gb_species;
    int     first = 1;

    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        GBDATA  *gb_name = GB_entry(gb_species, "name");
        GB_CSTR  name    = GB_read_char_pntr(gb_name);

        if (first) {
            first = 0;
        }
        else {
            GBS_chrcat(out, ';');
        }
        GBS_strcat(out, name);
        if (unmark_all) GB_write_flag(gb_species, 0);
    }

    return GBS_strclose(out);
}

NOT4PERL GB_ERROR GBT_with_stored_species(GBDATA *gb_main, const char *stored, species_callback doit, int *clientdata) {
    /* call function 'doit' with all species stored in 'stored' */

#define MAX_NAME_LEN 20
    char     name[MAX_NAME_LEN+1];
    GB_ERROR error = 0;

    while (!error) {
        char   *p   = strchr(stored, ';');
        int     len = p ? (p-stored) : (int)strlen(stored);
        GBDATA *gb_species;

        gb_assert(len <= MAX_NAME_LEN);
        memcpy(name, stored, len);
        name[len] = 0;

        gb_species = GBT_find_species(gb_main, name);
        if (gb_species) {
            error = doit(gb_species, clientdata);
        }
        else {
            error = "Some stored species where not found.";
        }

        if (!p) break;
        stored = p+1;
    }
#undef MAX_NAME_LEN
    return error;
}

static GB_ERROR restore_mark(GBDATA *gb_species, int *clientdata) {
    GBUSE(clientdata);
    GB_write_flag(gb_species, 1);
    return 0;
}

GB_ERROR GBT_restore_marked_species(GBDATA *gb_main, const char *stored_marked) {
    /* restores the species-marks to a state currently saved
       into 'stored_marked' by GBT_store_marked_species
    */

    GBT_mark_all(gb_main, 0);   /* unmark all species */
    return GBT_with_stored_species(gb_main, stored_marked, restore_mark, 0);
}

/********************************************************************************************
                    read species information
********************************************************************************************/

GBDATA *GBT_read_sequence(GBDATA *gb_species, const char *aliname)
{
    GBDATA *gb_ali = GB_entry(gb_species, aliname);
    return gb_ali ? GB_entry(gb_ali, "data") : 0;
}

char *GBT_read_name(GBDATA *gb_species)
{
    GBDATA *gb_name = GB_entry(gb_species,"name");
    if (!gb_name) return NULL;
    return GB_read_string(gb_name);
}

/********************************************************************************************
                    alignment procedures
********************************************************************************************/

char *GBT_get_default_alignment(GBDATA *gb_main)
{
    GBDATA *gb_use;
    gb_use = GB_search(gb_main,"presets/use",GB_FIND);
    if (!gb_use) return NULL;
    return GB_read_string(gb_use);
}

GB_ERROR GBT_set_default_alignment(GBDATA *gb_main,const char *alignment_name)
{
    GBDATA *gb_use;
    gb_use = GB_search(gb_main,"presets/use",GB_STRING);
    if (!gb_use) return 0;
    return GB_write_string(gb_use,alignment_name);
}

char *GBT_get_default_helix(GBDATA *gb_main)
{
    gb_main = gb_main;
    return GB_STRDUP("HELIX");
}

char *GBT_get_default_helix_nr(GBDATA *gb_main)
{
    gb_main = gb_main;
    return GB_STRDUP("HELIX_NR");
}

char *GBT_get_default_ref(GBDATA *gb_main)
{
    gb_main = gb_main;
    return GB_STRDUP("ECOLI");
}


GBDATA *GBT_get_alignment(GBDATA *gb_main, const char *aliname) {
    GBDATA *gb_presets        = GB_search(gb_main, "presets", GB_CREATE_CONTAINER);
    GBDATA *gb_alignment_name = GB_find_string(gb_presets,"alignment_name",aliname,GB_IGNORE_CASE,down_2_level);
    
    if (!gb_alignment_name) {
        GB_export_error("alignment '%s' not found", aliname);
        return NULL;
    }
    return GB_get_father(gb_alignment_name);
}

long GBT_get_alignment_len(GBDATA *gb_main, const char *aliname) {
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    if (gb_alignment) {
        GBDATA *gb_alignment_len = GB_search(gb_alignment, "alignment_len", GB_FIND);
        gb_assert(gb_alignment_len);
        return GB_read_int(gb_alignment_len);
    }
    return -1;
}

GB_ERROR GBT_set_alignment_len(GBDATA *gb_main, const char *aliname, long new_len) {
    GB_ERROR  error        = 0;
    GBDATA   *gb_alignment = GBT_get_alignment(gb_main, aliname);

    if (gb_alignment) {
        GBDATA *gb_alignment_len     = GB_search(gb_alignment,"alignment_len",GB_FIND);
        GBDATA *gb_alignment_aligned = GB_search(gb_alignment,"aligned",GB_FIND);
        
        GB_push_my_security(gb_main);
        error             = GB_write_int(gb_alignment_len,new_len); /* write new len */
        if (!error) error = GB_write_int(gb_alignment_aligned,0); /* sequences will be unaligned */
        GB_pop_my_security(gb_main);

    }
    else error = GB_export_error("Alignment '%s' not found", aliname);
    return error;
}

int GBT_get_alignment_aligned(GBDATA *gb_main, const char *aliname) {
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    if (gb_alignment) {
        GBDATA *gb_alignment_aligned = GB_search(gb_alignment, "aligned", GB_FIND);
        gb_assert(gb_alignment_aligned);
        return GB_read_int(gb_alignment_aligned);
    }
    return -1;
}

char *GBT_get_alignment_type_string(GBDATA *gb_main, const char *aliname) {
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    if (gb_alignment) {
        GBDATA *gb_alignment_type = GB_search(gb_alignment,"alignment_type",GB_FIND);
        gb_assert(gb_alignment_type);
        return GB_read_string(gb_alignment_type);
    }
    return NULL;
}

GB_alignment_type GBT_get_alignment_type(GBDATA *gb_main, const char *aliname) {
    char              *ali_type = GBT_get_alignment_type_string(gb_main, aliname);
    GB_alignment_type  at       = GB_AT_UNKNOWN;

    if (ali_type) {
        switch(ali_type[0]) {
            case 'r': if (strcmp(ali_type, "rna")==0) at = GB_AT_RNA; break;
            case 'd': if (strcmp(ali_type, "dna")==0) at = GB_AT_DNA; break;
            case 'a': if (strcmp(ali_type, "ami")==0) at = GB_AT_AA; break;
            case 'p': if (strcmp(ali_type, "pro")==0) at = GB_AT_AA; break;
            default: ad_assert(0); break;
        }
        free(ali_type);
    }
    return at;
}

GB_BOOL GBT_is_alignment_protein(GBDATA *gb_main,const char *alignment_name) {
    return GBT_get_alignment_type(gb_main,alignment_name) == GB_AT_AA;
}

/********************************************************************************************
                    check routines
********************************************************************************************/
GB_ERROR GBT_check_arb_file(const char *name)
     /* Checks whether the name of a file seemed to be an arb file */
     /* if == 0 it was an arb file */
{
    FILE *in;
    long i;
    char buffer[100];
    if (strchr(name,':')) return 0;
    in = fopen(name,"r");
    if (!in) return GB_export_error("Cannot find file '%s'",name);
    i = gb_read_in_long(in, 0);
    if ( (i== 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED)) {
        fclose(in);
        return 0;
    }
    rewind(in);
    fgets(buffer,50,in);
    fclose(in);
    if (!strncmp(buffer,"/*ARBDB AS",10)) {
        return 0;
    };


    return GB_export_error("'%s' is not an arb file",name);
}

/********************************************************************************************
                    analyse the database
********************************************************************************************/
#define GBT_SUM_LEN 4096
/* maximum length of path */

struct {
    GB_HASH *hash_table;
    int count;
    char **result;
    GB_TYPES type;
    char *buffer;
} gbs_scan_db_data;

void gbt_scan_db_rek(GBDATA *gbd,char *prefix, int deep)
{
    GB_TYPES type = GB_read_type(gbd);
    GBDATA *gb2;
    const char *key;
    int len_of_prefix;
    if (type == GB_DB) {
        len_of_prefix = strlen(prefix);
        for (gb2 = GB_child(gbd); gb2; gb2 = GB_nextChild(gb2)) {  /* find everything */
            if (deep){
                key = GB_read_key_pntr(gb2);
                sprintf(&prefix[len_of_prefix],"/%s",key);
                gbt_scan_db_rek(gb2,prefix,1);
            }
            else {
                prefix[len_of_prefix] = 0;
                gbt_scan_db_rek(gb2,prefix,1);
            }
        }
        prefix[len_of_prefix] = 0;
    }
    else {
        if (GB_check_hkey(prefix+1)) {
            prefix = prefix;        /* for debugging purpose */
        }
        else {
            prefix[0] = (char)type;
            GBS_incr_hash( gbs_scan_db_data.hash_table, prefix);
        }
    }
}

long gbs_scan_db_count(const char *key,long val)
{
    gbs_scan_db_data.count++;
    key = key;
    return val;
}

long gbs_scan_db_insert(const char *key,long val, void *v_datapath)
{
    if (!v_datapath) {
        gbs_scan_db_data.result[gbs_scan_db_data.count++]  = GB_STRDUP(key);
    }
    else {
        char *datapath = (char*)v_datapath;
        if (GBS_strscmp(datapath, key+1) == 0) { // datapath matches
            char *subkey = GB_STRDUP(key+strlen(datapath)); // cut off prefix
            subkey[0]    = key[0]; // copy type

            gbs_scan_db_data.result[gbs_scan_db_data.count++] = subkey;
        }
    }
    return val;
}

long gbs_scan_db_compare(const char *left,const char *right){
    return strcmp(&left[1],&right[1]);
}


char **GBT_scan_db(GBDATA *gbd, const char *datapath) {
    /* returns a NULL terminated array of 'strings'
       each string is the path to a node beyond gbd;
       every string exists only once
       the first character of a string is the type of the entry
       the strings are sorted alphabetically !!!

       if datapath              != 0, only keys with prefix datapath are scanned and
       the prefix is removed from the resulting key_names
    */
    gbs_scan_db_data.hash_table  = GBS_create_hash(1024, GB_MIND_CASE);
    gbs_scan_db_data.buffer      = (char *)malloc(GBT_SUM_LEN);
    strcpy(gbs_scan_db_data.buffer,"");
    gbt_scan_db_rek(gbd, gbs_scan_db_data.buffer,0);

    gbs_scan_db_data.count = 0;
    GBS_hash_do_loop(gbs_scan_db_data.hash_table,gbs_scan_db_count);

    gbs_scan_db_data.result = (char **)GB_calloc(sizeof(char *),gbs_scan_db_data.count+1);
    /* null terminated result */

    gbs_scan_db_data.count = 0;
    GBS_hash_do_loop2(gbs_scan_db_data.hash_table,gbs_scan_db_insert, (void*)datapath);

    GBS_free_hash(gbs_scan_db_data.hash_table);

    GB_mergesort((void **)gbs_scan_db_data.result,0,gbs_scan_db_data.count, (gb_compare_two_items_type)gbs_scan_db_compare,0);

    free(gbs_scan_db_data.buffer);
    return gbs_scan_db_data.result;
}

/********************************************************************************************
                send a message to the db server to AWAR_ERROR_MESSAGES
********************************************************************************************/

static void new_gbt_message_created_cb(GBDATA *gb_pending_messages, int *cd, GB_CB_TYPE cbt) {
    static int avoid_deadlock = 0;

    GBUSE(cd);
    GBUSE(cbt);

    if (!avoid_deadlock) {
        GBDATA *gb_msg;

        avoid_deadlock++;
        GB_push_transaction(gb_pending_messages);

        for (gb_msg = GB_entry(gb_pending_messages, "msg"); gb_msg;) {
            {
                const char *msg = GB_read_char_pntr(gb_msg);
                GB_warning("%s", msg);
            }
            {
                GBDATA *gb_next_msg = GB_nextEntry(gb_msg);
                GB_delete(gb_msg);
                gb_msg              = gb_next_msg;
            }
        }

        GB_pop_transaction(gb_pending_messages);
        avoid_deadlock--;
    }
}

void GBT_install_message_handler(GBDATA *gb_main) {
    GBDATA *gb_pending_messages;

    GB_push_transaction(gb_main);
    gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
    GB_add_callback(gb_pending_messages, GB_CB_SON_CREATED, new_gbt_message_created_cb, 0);
    GB_pop_transaction(gb_main);

#if defined(DEBUG) && 0
    GBT_message(GB_get_root(gb_pending_messages), GBS_global_string("GBT_install_message_handler installed for gb_main=%p", gb_main));
#endif /* DEBUG */
}


void GBT_message(GBDATA *gb_main, const char *msg) {
    // When called in client(or server) this causes the DB server to show the message.
    // Message is showed via GB_warning (which uses aw_message in GUIs)
    //
    // Note: The message is not shown before the transaction ends.
    // If the transaction is aborted, the message is never shown!
    // 
    // see also : GB_warning

    GBDATA *gb_pending_messages;
    GBDATA *gb_msg;

    gb_assert(msg);

    GB_push_transaction(gb_main);

    gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
    gb_msg              = GB_create(gb_pending_messages, "msg", GB_STRING);

    GB_write_string(gb_msg, msg);

    GB_pop_transaction(gb_main);
}

/********************************************************************************************
                Rename one or many species (only one session at a time/ uses
                commit abort transaction)
********************************************************************************************/
struct gbt_renamed_struct {
    int     used_by;
    char    data[1];

};

struct gbt_rename_struct {
    GB_HASH *renamed_hash;
    GB_HASH *old_species_hash;
    GBDATA *gb_main;
    GBDATA *gb_species_data;
    int all_flag;
} gbtrst;

/* starts the rename session, if allflag = 1 then the whole database is to be
        renamed !!!!!!! */
GB_ERROR GBT_begin_rename_session(GBDATA *gb_main, int all_flag)
{
    GB_ERROR error = GB_push_transaction(gb_main);
    if (!error) {
        gbtrst.gb_main         = gb_main;
        gbtrst.gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

        if (!all_flag) { // this is meant to be used for single or few species
            int hash_size = 256;

            gbtrst.renamed_hash     = GBS_create_hash(hash_size, GB_MIND_CASE);
            gbtrst.old_species_hash = 0;
        }
        else {
            int hash_size = GBT_get_species_hash_size(gb_main);

            gbtrst.renamed_hash     = GBS_create_hash(hash_size, GB_MIND_CASE);
            gbtrst.old_species_hash = GBT_create_species_hash(gb_main);
        }
        gbtrst.all_flag = all_flag;
    }
    return error;
}
/* returns errors */
GB_ERROR GBT_rename_species(const char *oldname, const  char *newname, GB_BOOL ignore_protection)
{
    GBDATA   *gb_species;
    GBDATA   *gb_name;
    GB_ERROR  error;

    if (strcmp(oldname, newname) == 0)
        return 0;

#if defined(DEBUG) && 1
    if (isdigit(oldname[0])) {
        printf("oldname='%s' newname='%s'\n", oldname, newname);
    }
#endif

    if (gbtrst.all_flag) {
        gb_assert(gbtrst.old_species_hash);
        gb_species = (GBDATA *)GBS_read_hash(gbtrst.old_species_hash,oldname);
    }
    else {
        GBDATA *gb_found_species;

        gb_assert(gbtrst.old_species_hash == 0);
        gb_found_species = GBT_find_species_rel_species_data(gbtrst.gb_species_data, newname);
        gb_species       = GBT_find_species_rel_species_data(gbtrst.gb_species_data, oldname);

        if (gb_found_species && gb_species != gb_found_species) {
            return GB_export_error("A species named '%s' already exists.",newname);
        }
    }

    if (!gb_species) {
        return GB_export_error("Expected that a species named '%s' exists (maybe there are duplicate species, database might be corrupt)",oldname);
    }

    gb_name = GB_entry(gb_species,"name");
    if (ignore_protection) GB_push_my_security(gbtrst.gb_main);
    error   = GB_write_string(gb_name,newname);
    if (ignore_protection) GB_pop_my_security(gbtrst.gb_main);

    if (!error){
        struct gbt_renamed_struct *rns;
        if (gbtrst.old_species_hash) {
            GBS_write_hash(gbtrst.old_species_hash, oldname, 0);
        }
        rns = (struct gbt_renamed_struct *)GB_calloc(strlen(newname) + sizeof (struct gbt_renamed_struct),sizeof(char));
        strcpy(&rns->data[0],newname);
        GBS_write_hash(gbtrst.renamed_hash,oldname,(long)rns);
    }
    return error;
}

static void gbt_free_rename_session_data(void) {
    if (gbtrst.renamed_hash) {
        GBS_free_hash_free_pointer(gbtrst.renamed_hash);
        gbtrst.renamed_hash = 0;
    }
    if (gbtrst.old_species_hash) {
        GBS_free_hash(gbtrst.old_species_hash);
        gbtrst.old_species_hash = 0;
    }
}

GB_ERROR GBT_abort_rename_session(void)
{
    GB_ERROR error;
    gbt_free_rename_session_data();
    error = GB_abort_transaction(gbtrst.gb_main);
    return error;
}

static const char *currentTreeName = 0;

GB_ERROR gbt_rename_tree_rek(GBT_TREE *tree,int tree_index){
    char buffer[256];
    static int counter = 0;
    if (!tree) return 0;
    if (tree->is_leaf){
        if (tree->name){
            struct gbt_renamed_struct *rns = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash,tree->name);
            if (rns){
                char *newname;
                if (rns->used_by == tree_index){ /* species more than once in the tree */
                    sprintf(buffer,"%s_%i", rns->data, counter++);
                    GB_warning("Species '%s' more than once in '%s', creating zombie '%s'",
                               tree->name, currentTreeName, buffer);
                    newname = buffer;
                }
                else {
                    newname = &rns->data[0];
                }
                free(tree->name);
                tree->name = GB_STRDUP(newname);
                rns->used_by = tree_index;
            }
        }
    }else{
        gbt_rename_tree_rek(tree->leftson,tree_index);
        gbt_rename_tree_rek(tree->rightson,tree_index);
    }
    return 0;
}

#ifdef __cplusplus
extern "C"
#endif
GB_ERROR GBT_commit_rename_session(int (*show_status)(double gauge), int (*show_status_text)(const char *)){
    GB_ERROR error = 0;

    // rename species in trees
    {
        int tree_count;
        char **tree_names = GBT_get_tree_names_and_count(gbtrst.gb_main, &tree_count);

        if (tree_names) {
            int count;
            gb_assert(tree_count); // otherwise tree_names should be zero

            if (show_status_text) show_status_text(GBS_global_string("Renaming species in %i tree%c", tree_count, "s"[tree_count<2]));
            if (show_status) show_status(0.0);

            for (count = 0; count<tree_count; ++count) {
                char     *tname = tree_names[count];
                GBT_TREE *tree  = GBT_read_tree(gbtrst.gb_main,tname,-sizeof(GBT_TREE));

                if (tree) {
                    currentTreeName = tname; // provide tree name (used for error message)
                    gbt_rename_tree_rek(tree, count+1);
                    currentTreeName = 0;

                    GBT_write_tree(gbtrst.gb_main, 0, tname, tree);
                    GBT_delete_tree(tree);
                }
                if (show_status) show_status((double)(count+1)/tree_count);
            }
            GBS_free_names(tree_names);
        }
    }
    // rename configurations
    if (!error) {
        int config_count;
        char **config_names = GBT_get_configuration_names_and_count(gbtrst.gb_main, &config_count);

        if (config_names) {
            int count;
            gb_assert(config_count); // otherwise config_names should be zero

            if (show_status_text) show_status_text(GBS_global_string("Renaming species in %i config%c", config_count, "s"[config_count<2]));
            if (show_status) show_status(0.0);

            for (count = 0; !error && count<config_count; ++count) {
                GBT_config *config = GBT_load_configuration_data(gbtrst.gb_main, config_names[count], &error);
                if (!error) {
                    int need_save = 0;
                    int mode;

                    for (mode = 0; !error && mode<2; ++mode) {
                        char              **configStrPtr = (mode == 0 ? &config->top_area : &config->middle_area);
                        GBT_config_parser  *parser       = GBT_start_config_parser(*configStrPtr);
                        GBT_config_item    *item         = GBT_create_config_item();
                        void               *strstruct    = GBS_stropen(1000);

                        error = GBT_parse_next_config_item(parser, item);
                        while (!error && item->type != CI_END_OF_CONFIG) {
                            if (item->type == CI_SPECIES) {
                                struct gbt_renamed_struct *rns = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash, item->name);
                                if (rns) { // species was renamed
                                    const char *newname = &rns->data[0];

                                    free(item->name);
                                    item->name = GB_STRDUP(newname);
                                    need_save  = 1;
                                }
                            }
                            GBT_append_to_config_string(item, strstruct);
                            error = GBT_parse_next_config_item(parser, item);
                        }

                        if (!error) {
                            free(*configStrPtr);
                            *configStrPtr = GBS_strclose(strstruct);
                        }

                        GBT_free_config_item(item);
                        GBT_free_config_parser(parser);
                    }

                    if (!error && need_save) error = GBT_save_configuration_data(config, gbtrst.gb_main, config_names[count]);
                }
                if (show_status) show_status((double)(count+1)/config_count);
            }
            GBS_free_names(config_names);
        }
    }

    // rename links in pseudo-species
    if (!error && GEN_is_genome_db(gbtrst.gb_main, -1)) {
        GBDATA *gb_pseudo;
        for (gb_pseudo = GEN_first_pseudo_species(gbtrst.gb_main);
             gb_pseudo && !error;
             gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
        {
            GBDATA *gb_origin_organism = GB_entry(gb_pseudo, "ARB_origin_species");
            if (gb_origin_organism) {
                const char                *origin = GB_read_char_pntr(gb_origin_organism);
                struct gbt_renamed_struct *rns    = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash, origin);
                if (rns) {          // species was renamed
                    const char *newname = &rns->data[0];
                    error               = GB_write_string(gb_origin_organism, newname);
                }
            }
        }
    }

    gbt_free_rename_session_data();

    error = GB_pop_transaction(gbtrst.gb_main);
    return error;
}

GBDATA **GBT_gen_species_array(GBDATA *gb_main, long *pspeccnt)
{
    GBDATA *gb_species;
    GBDATA *gb_species_data = GBT_find_or_create(gb_main,"species_data",7);
    GBDATA **result;
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)){
        (*pspeccnt) ++;
    }
    result = (GBDATA **)malloc((size_t)(sizeof(GBDATA *)* (*pspeccnt)));
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)){
        result[(*pspeccnt)++]=gb_species;
    }
    return result;
}


char *GBT_read_string(GBDATA *gb_main, const char *awar_name){
    /* Search and read an database field */
    GBDATA *gbd;
    char *result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        fprintf(stderr,"GBT_read_string error: Cannot find %s\n",awar_name);
        GB_pop_transaction(gb_main);
        return GB_STRDUP("");
    }
    result = GB_read_string(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

long GBT_read_int(GBDATA *gb_main, const char *awar_name)
{
    GBDATA *gbd;
    long result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        fprintf(stderr,"GBT_read_int error: Cannot find %s\n",awar_name);
        GB_pop_transaction(gb_main);
        return 0;
    }
    result = GB_read_int(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

double GBT_read_float(GBDATA *gb_main, const char *awar_name)
{
    GBDATA *gbd;
    float result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        fprintf(stderr,"GBT_read_float error: Cannot find %s\n",awar_name);
        GB_pop_transaction(gb_main);
        return 0.0;
    }
    result = GB_read_float(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

GBDATA *GBT_search_string(GBDATA *gb_main, const char *awar_name, const char *def){
    GBDATA *gbd;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_STRING);
        GB_write_string(gbd,def);
    }
    GB_pop_transaction(gb_main);
    return gbd;
}

GBDATA *GBT_search_int(GBDATA *gb_main, const char *awar_name, int def){
    GBDATA *gbd;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_INT);
        GB_write_int(gbd,def);
    }
    GB_pop_transaction(gb_main);
    return gbd;
}

GBDATA *GBT_search_float(GBDATA *gb_main, const char *awar_name, double def){
    GBDATA *gbd;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_FLOAT);
        GB_write_float(gbd,def);
    }
    GB_pop_transaction(gb_main);
    return gbd;
}

#if defined(DEVEL_RALF)
#warning give the following functions meaningful names!
/* e.g. GBT_readOrCreate_string() etc. */
#endif /* DEVEL_RALF */

char *GBT_read_string2(GBDATA *gb_main, const char *awar_name, const char *def)
{
    GBDATA *gbd;
    char *result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_STRING);
        GB_write_string(gbd,def);
    }
    result = GB_read_string(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

long GBT_read_int2(GBDATA *gb_main, const char *awar_name, long def)
{
    GBDATA *gbd;
    long result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_INT);
        GB_write_int(gbd,def);
    }
    result = GB_read_int(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

double GBT_read_float2(GBDATA *gb_main, const char *awar_name, double def)
{
    GBDATA *gbd;
    double result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_FLOAT);
        GB_write_float(gbd,def);
    }
    result = GB_read_float(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

GB_ERROR GBT_write_string(GBDATA *gb_main, const char *awar_name, const char *def)
{
    GBDATA *gbd;
    GB_ERROR error;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_STRING);
    error = GB_write_string(gbd,def);
    GB_pop_transaction(gb_main);
    return error;
}

GB_ERROR GBT_write_int(GBDATA *gb_main, const char *awar_name, long def)
{
    GBDATA *gbd;
    GB_ERROR error;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_INT);
    error = GB_write_int(gbd,def);
    GB_pop_transaction(gb_main);
    return error;
}

GB_ERROR GBT_write_float(GBDATA *gb_main, const char *awar_name, double def)
{
    GBDATA *gbd;
    GB_ERROR error;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FLOAT);
    error = GB_write_float(gbd,def);
    GB_pop_transaction(gb_main);
    return error;
}

GBDATA *GB_test_link_follower(GBDATA *gb_main,GBDATA *gb_link,const char *link){
    GBDATA *linktarget = GB_search(gb_main,"tmp/link/string",GB_STRING);
    GBUSE(gb_link);
    GB_write_string(linktarget,GBS_global_string("Link is '%s'",link));
    return GB_get_father(linktarget);
}

/********************************************************************************************
                    SAVE & LOAD
********************************************************************************************/

/** Open a database, create an index for species and extended names,
    disable saving the database in the PT_SERVER directory */

GBDATA *GBT_open(const char *path,const char *opent,const char *disabled_path)
{
    GBDATA *gbd = GB_open(path,opent);
    GBDATA *species_data;
    GBDATA *extended_data;
    GBDATA *gb_tmp;
    long    hash_size;

    if (!gbd) return gbd;
    if (!disabled_path) disabled_path = "$(ARBHOME)/lib/pts/*";
    GB_disable_path(gbd,disabled_path);
    GB_begin_transaction(gbd);

    if (!strchr(path,':')){
        species_data = GB_search(gbd, "species_data", GB_FIND);
        if (species_data){
            hash_size = GB_number_of_subentries(species_data);
            if (hash_size < GBT_SPECIES_INDEX_SIZE) hash_size = GBT_SPECIES_INDEX_SIZE;
            GB_create_index(species_data,"name",GB_IGNORE_CASE,hash_size);

            extended_data = GB_search(gbd, "extended_data", GB_CREATE_CONTAINER);
            hash_size = GB_number_of_subentries(extended_data);
            if (hash_size < GBT_SAI_INDEX_SIZE) hash_size = GBT_SAI_INDEX_SIZE;
            GB_create_index(extended_data,"name",GB_IGNORE_CASE,hash_size);
        }
    }
    gb_tmp = GB_search(gbd,"tmp",GB_CREATE_CONTAINER);
    GB_set_temporary(gb_tmp);
    {               /* install link followers */
        GB_MAIN_TYPE *Main = GB_MAIN(gbd);
        Main->table_hash = GBS_create_hash(256, GB_MIND_CASE);
        GB_install_link_follower(gbd,"REF",GB_test_link_follower);
    }
    GBT_install_table_link_follower(gbd);
    GB_commit_transaction(gbd);
    return gbd;
}

/********************************************************************************************
                    REMOTE COMMANDS
********************************************************************************************/

#define AWAR_REMOTE_BASE_TPL            "tmp/remote/%s/"
#define MAX_REMOTE_APPLICATION_NAME_LEN 30
#define MAX_REMOTE_AWAR_STRING_LEN      (11+MAX_REMOTE_APPLICATION_NAME_LEN+1+6+1)

struct gbt_remote_awars {
    char awar_action[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_result[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_awar[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_value[MAX_REMOTE_AWAR_STRING_LEN];
};

static void gbt_build_remote_awars(struct gbt_remote_awars *awars, const char *application) {
    int length;

    gb_assert(strlen(application) <= MAX_REMOTE_APPLICATION_NAME_LEN);

    length = sprintf(awars->awar_action, AWAR_REMOTE_BASE_TPL, application);
    gb_assert(length < (MAX_REMOTE_AWAR_STRING_LEN-6)); // Note :  6 is length of longest name appended below !

    strcpy(awars->awar_result, awars->awar_action);
    strcpy(awars->awar_awar, awars->awar_action);
    strcpy(awars->awar_value, awars->awar_action);

    strcpy(awars->awar_action+length, "action");
    strcpy(awars->awar_result+length, "result");
    strcpy(awars->awar_awar+length,   "awar");
    strcpy(awars->awar_value+length,  "value");
}

static GBDATA *gbt_remote_search_awar(GBDATA *gb_main, const char *awar_name) {
    GBDATA *gb_action;
    while (1) {
        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awar_name, GB_FIND);
        GB_commit_transaction(gb_main);
        if (gb_action) break;
        GB_usleep(2000);
    }
    return gb_action;
}

static GB_ERROR gbt_wait_for_remote_action(GBDATA *gb_main, GBDATA *gb_action, const char *awar_read) {
    GB_ERROR error = 0;

    /* wait to end of action */
    while(1) {
        char *ac;
        GB_usleep(2000);
        GB_begin_transaction(gb_main);
        ac = GB_read_string(gb_action);
        if (ac[0] == 0) { // action has been cleared from remote side
            GBDATA *gb_result = GB_search(gb_main, awar_read, GB_STRING);
            error             = GB_read_char_pntr(gb_result); // check for errors
            free(ac);
            GB_commit_transaction(gb_main);
            break;
        }
        free(ac);
        GB_commit_transaction(gb_main);
    }

    return error; // may be error or result
}

GB_ERROR GBT_remote_action(GBDATA *gb_main, const char *application, const char *action_name){
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_action;

    gbt_build_remote_awars(&awars, application);
    gb_action = gbt_remote_search_awar(gb_main, awars.awar_action);

    GB_begin_transaction(gb_main);
    GB_write_string(gb_action, action_name); /* write command */
    GB_commit_transaction(gb_main);

    return gbt_wait_for_remote_action(gb_main, gb_action, awars.awar_result);
}

GB_ERROR GBT_remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    {
        GBDATA *gb_value;

        GB_begin_transaction(gb_main);
        gb_value = GB_search(gb_main, awars.awar_value, GB_STRING);
        GB_write_string(gb_awar, awar_name);
        GB_write_string(gb_value, value);
        GB_commit_transaction(gb_main);
    }

    return gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
}

const char *GBT_remote_read_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    const char              *result = 0;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    {
        GBDATA *gb_action;

        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awars.awar_action, GB_STRING);
        GB_write_string(gb_awar, awar_name);
        GB_write_string(gb_action, "AWAR_REMOTE_READ");
        GB_commit_transaction(gb_main);
    }

    result = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_value);
    return result;
}

const char *GBT_remote_touch_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    {
        GBDATA *gb_action;

        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awars.awar_action, GB_STRING);
        GB_write_string(gb_awar, awar_name);
        GB_write_string(gb_action, "AWAR_REMOTE_TOUCH");
        GB_commit_transaction(gb_main);
    }

    return gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
}

static GB_ERROR expect_gene_position(GBDATA *gb_gene, const char *prefix, int whichPos, long *pos) {
    const char *pos_entry = whichPos>1 ? GBS_global_string("%s%i", prefix, whichPos) : prefix;
    GBDATA     *gb_pos;
    GB_ERROR    error     = 0;

    *pos = 0;

    gb_assert(whichPos>0);
    gb_pos = GB_entry(gb_gene, pos_entry);
    if (gb_pos) *pos = GB_read_int(gb_pos);
    else error = GB_export_error("Expected entry '%s'", pos_entry);
    return error;
}

NOT4PERL GB_ERROR GBT_get_gene_positions(GBDATA *gb_gene, int whichPos, long *pos_begin, long *pos_end) {
    GB_ERROR error    = expect_gene_position(gb_gene, "pos_begin", whichPos, pos_begin);
    if (!error) {
        error = expect_gene_position(gb_gene, "pos_end", whichPos, pos_end);
        if (!error) {
            if (*pos_begin>*pos_end) error = "Illegal gene positions";
        }
    }
    return error;
}

/*  ---------------------------------------------------------------------------------  */
/*      char *GBT_read_gene_sequence(GBDATA *gb_gene, GB_BOOL use_revComplement)       */
/*  ---------------------------------------------------------------------------------  */
/* GBT_read_gene_sequence is intentionally located here (otherwise we get serious linkage problems) */

static const char *gb_cache_genome(GBDATA *gb_genome) {
    static GBDATA     *gb_last_genome = 0;
    static const char *last_genome    = 0;

    if (gb_genome != gb_last_genome) {
        last_genome    = GB_read_char_pntr(gb_genome);
        gb_last_genome = gb_genome;
    }

    return last_genome;
}

char *GBT_read_gene_sequence(GBDATA *gb_gene, GB_BOOL use_revComplement) {
    /* read the sequence for the specified gene */

    GB_ERROR  error         = 0;
    char     *result        = 0;
    GBDATA   *gb_complement = GB_entry(gb_gene, "complement");
    int       complement    = gb_complement ? GB_read_byte(gb_complement)!=0 : 0;
    GBDATA   *gb_joined     = GB_entry(gb_gene, "pos_joined");
    int       parts         = gb_joined ? GB_read_int(gb_joined) : 1;
    int       p;
    GBDATA   *gb_species    = GB_get_father(GB_get_father(gb_gene));
    GBDATA   *gb_seq        = GBT_read_sequence(gb_species, "ali_genom");
    long      seq_length    = GB_read_count(gb_seq);
    int       resultlen     = 0;

    for (p = 1; p <= parts; p++) {
        long pos_begin, pos_end;
        error = GBT_get_gene_positions(gb_gene, p, &pos_begin, &pos_end);
        if (!error) {
            if (pos_end>seq_length) {
                error = GBS_global_string("Illegal gene position(s): endpos = %li, seq.length=%li", pos_end, seq_length);
            }
            else  {
                resultlen += pos_end-pos_begin+1;
            }
        }
    }

    if (!error) {
        char       *resultpos;
        long        pos_begin, pos_end;
        int         len;
        /* const char *seq_data = GB_read_char_pntr(gb_seq); */
        const char *seq_data = gb_cache_genome(gb_seq);

        result    = malloc(resultlen+1);
        resultpos = result;

#warning GBT_read_gene_sequence has to be changed later when 'complement' 'complement2' 'complement3' entries exits

        if (complement == 0) {
            for (p = 1; p <= parts; p++) {
                error      = GBT_get_gene_positions(gb_gene, p, &pos_begin, &pos_end);
                gb_assert(!error); // would have occurred above
                len        = pos_end-pos_begin+1;
                memcpy(resultpos, seq_data+pos_begin-1, len);
                resultpos += len;
            }
            result[resultlen] = 0;
        }
        else {
            for (p = parts; p >= 1; p--) {
                error      = GBT_get_gene_positions(gb_gene, p, &pos_begin, &pos_end);
                gb_assert(!error); // would have occurred above
                len        = pos_end-pos_begin+1;
                memcpy(resultpos, seq_data+pos_begin-1, len);
                resultpos += len;
            }
            result[resultlen] = 0;

            if (use_revComplement) {
                char  T_or_U;
                error = GBT_determine_T_or_U(GB_AT_DNA, &T_or_U, "reverse-complement");
                if (!error) GBT_reverseComplementNucSequence(result, resultlen, T_or_U);
            }
        }
        if (error)  {
            free(result);
            result = 0;
        }
    }

    if (error) {
        GBDATA *gb_name   = GB_entry(gb_gene, "name");
        char   *gene_name = GB_strdup(gb_name ? GB_read_char_pntr(gb_name) : "<unnamed gene>");
        char   *species_name;

        gb_name      = GB_entry(gb_species, "name");
        species_name = GB_strdup(gb_name ? GB_read_char_pntr(gb_name) : "<unnamed species>");

        error = GB_export_error("%s (in %s/%s)", error, species_name, gene_name);
    }

    return result;
}

/* --------------------------- */
/*      self-notification      */
/* --------------------------- */
/* provides a mechanism to notify ARB after some external tool finishes */

#define ARB_NOTIFICATIONS "tmp/notify"

/* DB structure for notifications : 
 *
 * ARB_NOTIFICATIONS/counter        GB_INT      counts used ids
 * ARB_NOTIFICATIONS/notify/id      GB_INT      id of notification
 * ARB_NOTIFICATIONS/notify/message GB_STRING   message of notification (set by callback)
 */

typedef void (*notify_cb_type)(const char *message, void *client_data);

struct NCB {
    notify_cb_type  cb;
    void           *client_data;
};

static void notify_cb(GBDATA *gb_message, int *cb_info, GB_CB_TYPE cb_type) {
    GB_ERROR error   = GB_remove_callback(gb_message, GB_CB_CHANGED|GB_CB_DELETE, notify_cb, cb_info);
    int      cb_done = 0;

    struct NCB *pending = (struct NCB*)cb_info;

    if (cb_type == GB_CB_CHANGED) {
        if (!error) {
            const char *message = GB_read_char_pntr(gb_message);
            if (message) {
                pending->cb(message, pending->client_data);
                cb_done = 1;
            }
        }

        if (!cb_done) {
            if (!error) error = GB_get_error();
            fprintf(stderr, "Notification failed (Reason: %s)\n", error);
            gb_assert(0);
        }
    }
    else { /* called from GB_remove_last_notification */
        gb_assert(cb_type == GB_CB_DELETE);
    }

    free(pending);
}

static int allocateNotificationID(GBDATA *gb_main, int *cb_info) {
    int      id    = 0;
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_CREATE_CONTAINER);
        if (gb_notify) {
            GBDATA *gb_counter = GB_entry(gb_notify, "counter");

            if (!gb_counter) {          /* first call */
                gb_counter = GB_create(gb_notify, "counter", GB_INT);
                if (gb_counter) {
                    error = GB_write_int(gb_counter, 0);
                }
            }

            if (gb_counter) {
                int newid = GB_read_int(gb_counter) + 1; /* increment counter */
                error     = GB_write_int(gb_counter, newid);

                if (!error) {
                    /* change transaction (to never use id twice!) */
                    error             = GB_pop_transaction(gb_main);
                    if (!error) error = GB_push_transaction(gb_main);

                    if (!error) {
                        GBDATA *gb_notification = GB_create_container(gb_notify, "notify");
                        if (gb_notification) {
                            GBDATA *gb_id = GB_create(gb_notification, "id", GB_INT);
                            if (gb_id) {
                                error = GB_write_int(gb_id, newid);
                                if (!error) {
                                    GBDATA *gb_message = GB_create(gb_notification, "message", GB_STRING);
                                    if (gb_message) {
                                        error = GB_write_string(gb_message, "");
                                        if (!error) {
                                            error = GB_add_callback(gb_message, GB_CB_CHANGED|GB_CB_DELETE, notify_cb, cb_info);
                                            if (!error) {
                                                id = newid; /* success */
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (error || !id) {
        GB_abort_transaction(gb_main);
        if (error) GB_export_error(error);
    }
    else GB_pop_transaction(gb_main);

    return id;
}


char *GB_generate_notification(GBDATA *gb_main,
                               void (*cb)(const char *message, void *client_data),
                               const char *message, void *client_data)
{
    /* generates a call to 'arb_notify', meant to be inserted into some external system call.
     * When that call is executed, the callback instanciated here will be called.
     *
     * Tip : To return variable results from the shell skript, use the name of an environment
     *       variable in 'message' (e.g. "$RESULT")
     */

    int         id;
    char       *arb_notify_call = 0;
    struct NCB *pending         = malloc(sizeof(*pending));

    pending->cb          = cb;
    pending->client_data = client_data;

    id = allocateNotificationID(gb_main, (int*)pending);
    if (id) {
        arb_notify_call = GBS_global_string_copy("arb_notify %i \"%s\"", id, message);
    }
    else {
        free(pending);
    }

    return arb_notify_call;
}

GB_ERROR GB_remove_last_notification(GBDATA *gb_main) {
    /* aborts the last notification */
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_CREATE_CONTAINER);
        if (gb_notify) {
            GBDATA *gb_counter = GB_entry(gb_notify, "counter");
            if (gb_counter) {
                int     id    = GB_read_int(gb_counter);
                GBDATA *gb_id = GB_find_int(gb_notify, "id", id, down_2_level);

                if (!gb_id) {
                    error = GBS_global_string("No notification for ID %i", id);
                    gb_assert(0);           // GB_generate_notification() has not been called for 'id'!
                }
                else {
                    GBDATA *gb_message = GB_brother(gb_id, "message");

                    if (!gb_message) {
                        error = "Missing 'message' entry";
                    }
                    else {
                        error = GB_delete(gb_message); /* calls notify_cb */
                    }
                }
            }
            else {
                error = "No notification generated yet";
            }
        }
    }

    GB_pop_transaction(gb_main);

    return error;
}

GB_ERROR GB_notify(GBDATA *gb_main, int id, const char *message) {
    /* called via 'arb_notify'
     * 'id' has to be generated by GB_generate_notification()
     * 'message' is passed to notification callback belonging to id
     */

    GB_ERROR  error     = 0;
    GBDATA   *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_FIND);

    if (!gb_notify) {
        error = "Missing notification data";
        gb_assert(0);           // GB_generate_notification() has not been called!
    }
    else {
        GBDATA *gb_id = GB_find_int(gb_notify, "id", id, down_2_level);

        if (!gb_id) {
            error = GBS_global_string("No notification for ID %i", id);
        }
        else {
            GBDATA *gb_message = GB_brother(gb_id, "message");

            if (!gb_message) {
                error = "Missing 'message' entry";
            }
            else {
                /* callback the instanciating DB client */
                error = GB_write_string(gb_message, message);
            }
        }
    }

    return error;
}


