# MAFFT for ARB
# by Arne Boeckmann (2013)

PROG  = MAFFT

# force gcc (hack to fix cent5 build)
export CC = gcc

# dont spam
ANY_EXISTING_FILE=Makefile
POST_COMPILE_LOCAL := 2>&1 | $(ARBHOME)/SOURCE_TOOLS/postcompile.pl --no-warnings $(ANY_EXISTING_FILE)

#disable makeflags
MAKEFLAGS=
MAFFTDIR=mafft-7.055-with-extensions
SCRIPTDIR=$(MAFFTDIR)/scripts
BINDIR=$(MAFFTDIR)/binaries

# ARBs standard target
$(MAIN) : $(SCRIPTDIR) $(BINDIR)
	$(MAKE) mafft.stamp

$(SCRIPTDIR):
	mkdir -p $@

$(BINDIR):
	mkdir -p $@

clean_core:
	+cd $(MAFFTDIR)/core && make clean

clean_extensions:
	+cd $(MAFFTDIR)/extensions && make clean

clean_here:
	rm -rf $(ARBHOME)/lib/mafft
	rm -f mafft.stamp
	rm -f $(ARBHOME)/bin/mafft*

clean_dirs:
	rm -rf $(SCRIPTDIR)
	rm -rf $(BINDIR)

clean: clean_core clean_extensions clean_here clean_dirs

make_core:
	+( cd $(MAFFTDIR)/core && make && make install ) $(POST_COMPILE_LOCAL)

make_extensions:
	+( cd $(MAFFTDIR)/extensions && make && make install ) $(POST_COMPILE_LOCAL)

mafft.stamp: make_core make_extensions
	touch mafft.stamp

.PHONY : clean clean_core clean_extensions clean_here make_core make_extensions
