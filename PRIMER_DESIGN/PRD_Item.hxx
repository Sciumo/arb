#ifndef PRD_ITEM_HXX
#define PRD_ITEM_HXX

#include <cstdio>
#ifndef   PRD_GLOBALS_HXX
#include "PRD_Globals.hxx"
#endif

class Item {

public:

  PRD_Sequence_Pos start_pos;
  int              length;

  int              CG_ratio;
  int              temperature;

  Item *next;

  Item ( PRD_Sequence_Pos pos_, int length_, int ratio_, int temperature_, Item *next_ );
  Item ();
  ~Item ();

    void print ( char *prefix, char *suffix );

    const char * get_primer_sequence() { return "Hallo"; }
};

#else
#error PRD_Item.hxx included twice
#endif // PRD_ITEM_HXX

