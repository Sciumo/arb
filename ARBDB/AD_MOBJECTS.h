# objects with autogenerated headers (ad_lpro.h + ad_prot.h)
GB_O =  adsort.o adlang1.o adstring.o arbdb.o      \
        ad_core.o admath.o adoptimize.o adsystem.o \
        adindex.o adperl.o adlink.o \
        adsocket.o adcomm.o adhash.o  \
        adquery.o ad_save_load.o  \
        adcompr.o admalloc.o ad_load.o \
        admap.o adTest.o adtune.o \
        adGene.o adtcp.o
    
# objects with autogenerated headers (ad_t_lpro.h + ad_t_prot.h)
GB_T =  adtools.o adseqcompr.o adtables.o adRevCompl.o

# c++-only objects    
GB_PP = arbdbpp.o

# objects with own header files
GB_X =  ad_config.o
