
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PS_DATABASE_HXX
#include "ps_database.hxx"
#endif
#ifndef PS_BITMAP_HXX
#include "ps_bitmap.hxx"
#endif

// common globals
SpeciesID         __MAX_ID;
PS_BitMap_Fast   *__MAP;
// globals for PS_detect_weak_differences
IDVector         *__PATH;
IDVector         *__INVERSE_PATH;
unsigned long int __COUNT_SET_OPS  = 0;
unsigned long int __COUNT_SET_OPS2 = 0;
// globals for PS_print_and_evaluate_map
IDSet            *__PATHSET;
IDID2IDSetMap    *__PAIR2PATH;
SpeciesID         __ONEMATCH_MIN_ID;
SpeciesID         __ONEMATCH_MAX_ID;

//  ----------------------------------------------------
//      void PS_print_path()
//  ----------------------------------------------------
void PS_print_path() {
    printf( "__PATH %3i :",__PATH->size() );
    int c = 1;
    for (IDVectorCIter i = __PATH->begin(); i != __PATH->end(); ++i,++c) {
        if (c % 20 == 0) printf( "\n" );
        printf( " %3i", *i );
    }
    printf( "\n" );
}


//  ----------------------------------------------------
//      void PS_inverse_print_path()
//  ----------------------------------------------------
void PS_print_inverse_path() {
    printf( "__INVERSE_PATH %3i :",__INVERSE_PATH->size() );
    int c = 1;
    for (IDVectorCIter i = __INVERSE_PATH->begin(); i != __INVERSE_PATH->end(); ++i,++c) {
        if (c % 20 == 0) printf( "\n" );
        printf( " %3i", *i );
    }
    printf( "\n" );
}


//  ----------------------------------------------------
//      void PS_detect_weak_differences( const PS_NodePtr _root_node )
//  ----------------------------------------------------
//  Recursively walk through tree and make a bool-matrix of SpeciesID's
//  where true means that the 2 species can be distinguished by a probe.
//
//  The first occurence of a pair of distinguishable IDs is stored as (smaller_ID,bigger_ID).
//  The following ocurences of this pair are stored as (bigger_ID,smaller_ID).
//  (this allows us to find pairs of SpeciesIDs that can be distinguished by exactly one probe)
//
void PS_detect_weak_differences_stepdown( const PS_NodePtr _ps_node,
                                          const SpeciesID  _parent_ID ) {
    
    SpeciesID id = _ps_node->getNum();

<<<<<<< ps_detect_weak_differences.cxx
=======
void PS_detect_weak_differences_stepdown( const PS_NodePtr _ps_node, PS_BitMap *_map, IDvector &_upper_nodes ) {
    
>>>>>>> 1.5
    //
<<<<<<< ps_detect_weak_differences.cxx
    // append IDs to paths
=======
    // get SpeciesID
>>>>>>> 1.5
    //
<<<<<<< ps_detect_weak_differences.cxx
    __PATH->push_back( id );
    for (SpeciesID i = _parent_ID+1; ((i < id) && (i >= 0)); ++i) {
        //printf( "%i ",i );
        __INVERSE_PATH->push_back( i );
    }
=======
    SpeciesID nodeNum = _ps_node->getNum();
>>>>>>> 1.5

    //
    // set values in the maps if node has probes
    //
<<<<<<< ps_detect_weak_differences.cxx
    if (_ps_node->hasProbes()) {
        if (_ps_node->hasPositiveProbes() && _ps_node->hasInverseProbes()) {
//            PS_print_path();
//            PS_print_inverse_path();
            unsigned long int set_ops = 2*__PATH->size()*(__MAX_ID-id-1+__INVERSE_PATH->size());
            if (ULONG_MAX - __COUNT_SET_OPS < set_ops) {
                set_ops = set_ops - (ULONG_MAX - __COUNT_SET_OPS);
                __COUNT_SET_OPS = 0;
                ++__COUNT_SET_OPS2;
            }
            __COUNT_SET_OPS = __COUNT_SET_OPS + set_ops;

            SpeciesID inverse_path_ID;
            // path loop
            for (IDVectorCIter it_path = __PATH->begin();
                 it_path != __PATH->end();
                 ++it_path) {
                SpeciesID path_ID = *it_path;
                // inverse path loop (explicit)
                for (IDVectorCIter it_inverse_path = __INVERSE_PATH->begin();
                     it_inverse_path != __INVERSE_PATH->end();
                     ++it_inverse_path ) {
                    inverse_path_ID = *it_inverse_path;

                    __MAP->setTrue( path_ID, inverse_path_ID );
                    __MAP->setTrue( inverse_path_ID, path_ID );
                }

                // inverse path loop (implicit)
                for (inverse_path_ID = id+1; inverse_path_ID < __MAX_ID; ++inverse_path_ID) { // skip to id ABOVE current node id
                    __MAP->setTrue( path_ID, inverse_path_ID );
                    __MAP->setTrue( inverse_path_ID, path_ID );
                }
            }
        } else {
//            PS_print_path();
//            PS_print_inverse_path();
            unsigned long int set_ops = __PATH->size()*(__MAX_ID-id-1+__INVERSE_PATH->size());
            if (ULONG_MAX - __COUNT_SET_OPS < set_ops) {
                set_ops = set_ops - (ULONG_MAX - __COUNT_SET_OPS);
                __COUNT_SET_OPS = 0;
                ++__COUNT_SET_OPS2;
            }
            __COUNT_SET_OPS = __COUNT_SET_OPS + set_ops;

            SpeciesID inverse_path_ID;
            SpeciesID smaller_ID;
            SpeciesID bigger_ID;
            // path loop
            for (IDVectorCIter it_path = __PATH->begin();
                 it_path != __PATH->end();
                 ++it_path) {
                SpeciesID path_ID = *it_path;
                // inverse path loop (explicit)
                for (IDVectorCIter it_inverse_path = __INVERSE_PATH->begin();
                     it_inverse_path != __INVERSE_PATH->end();
                     ++it_inverse_path ) {
                    inverse_path_ID = *it_inverse_path;
                    smaller_ID = (path_ID < inverse_path_ID) ? path_ID : inverse_path_ID;
                    bigger_ID  = (path_ID > inverse_path_ID) ? path_ID : inverse_path_ID;

                    if (__MAP->get( smaller_ID, bigger_ID )) {
                        __MAP->setTrue( bigger_ID, smaller_ID );
                    } else {
                        __MAP->setTrue( smaller_ID, bigger_ID );
                    }
                }

                // inverse path loop (implicit)
                for (inverse_path_ID = id+1; inverse_path_ID < __MAX_ID; ++inverse_path_ID) { // skip to id ABOVE current node id
                    smaller_ID = (path_ID < inverse_path_ID) ? path_ID : inverse_path_ID;
                    bigger_ID  = (path_ID > inverse_path_ID) ? path_ID : inverse_path_ID;

                    if (__MAP->get( smaller_ID, bigger_ID )) {
                        __MAP->setTrue( bigger_ID, smaller_ID );
                    } else {
                        __MAP->setTrue( smaller_ID, bigger_ID );
                    }
                }
            }
        }
    }
=======
    if (!_ps_node->hasProbes()) nodeNum = -nodeNum;               // negative nodeNums indicate an empty probes-list in a node
    _upper_nodes.push_back( nodeNum );

>>>>>>> 1.5
    //
    // step down the children
    //
<<<<<<< ps_detect_weak_differences.cxx
    for (PS_NodeMapConstIterator i = _ps_node->getChildrenBegin();
         i != _ps_node->getChildrenEnd();
         ++i) {
        PS_detect_weak_differences_stepdown( i->second, id );
=======
    for (PS_NodeMapConstIterator i = _ps_node->getChildrenBegin(); i != _ps_node->getChildrenEnd(); ++i) {
        PS_detect_weak_differences_stepdown( i->second, _map, _upper_nodes );
>>>>>>> 1.5
    }

    //
    // remove IDs from paths
    //
<<<<<<< ps_detect_weak_differences.cxx
    __PATH->pop_back();
    while ((__INVERSE_PATH->back() > _parent_ID) && (!__INVERSE_PATH->empty())) {
        __INVERSE_PATH->pop_back();
    }
}
=======
    _upper_nodes.pop_back();
>>>>>>> 1.5

void PS_detect_weak_differences( const PS_NodePtr _root_node ) {
    //
    // make bitmap
    //
<<<<<<< ps_detect_weak_differences.cxx
    __PATH         = new IDVector;
    __INVERSE_PATH = new IDVector;

    int c = 0;
    for (PS_NodeMapConstIterator i = _root_node->getChildrenBegin(); i != _root_node->getChildrenEnd(); ++i,++c ) {
        if (c % 50 ==0) fprintf( stderr, "PS_detect_weak_differences_stepdown( %i ) : %i of %i\n", i->first, c+1, _root_node->countChildren() );
        PS_detect_weak_differences_stepdown( i->second, -1 );
=======
    IDvector::reverse_iterator upperIndex = _upper_nodes.rbegin();    // rbegin() <-> walk from end to start
    // skip empty nodes at the end of the upper_nodes - list
    // because they have no probe to distinguish them from us
    for ( ; upperIndex != _upper_nodes.rend(); ++upperIndex ) {
        if (*upperIndex >= 0) break;
>>>>>>> 1.5
    }
<<<<<<< ps_detect_weak_differences.cxx
    printf( "%lu * %lu + %lu set operations performed\n", __COUNT_SET_OPS2, ULONG_MAX, __COUNT_SET_OPS );

    delete __PATH;
    delete __INVERSE_PATH;
}

//  ----------------------------------------------------
//      void PS_print_and_evaluate_map( const PS_NodePtr _root_node )
//  ----------------------------------------------------
typedef map<ID2IDPair,PS_NodePtr>    IDID2NodeMap;
typedef IDID2NodeMap::iterator       IDID2NodeMapIter;
typedef IDID2NodeMap::const_iterator IDID2NodeMapCIter;
void PS_find_probes_for_pairs( const PS_NodePtr  _ps_node, 
                                       ID2IDSet &_pairs ) {
    SpeciesID id         = _ps_node->getNum();
    bool      has_probes = _ps_node->hasProbes();

    //
    // append ID to path
    //
    __PATHSET->insert( id );

    //
    // dont look at path until ID is greater than lowest ID in the set of ID-pairs
    //
    if ((id >= __ONEMATCH_MIN_ID) && has_probes) {
        for (ID2IDSetCIter pair=_pairs.begin(); pair != _pairs.end(); ++pair) {
            bool found_first  = __PATHSET->find( pair->first  ) != __PATHSET->end();
            bool found_second = __PATHSET->find( pair->second ) != __PATHSET->end();
            if (found_first ^ found_second) { // ^ = XOR
                printf( "found path for (%i,%i) at %p ", pair->first, pair->second,&(*_ps_node) );
                _ps_node->printOnlyMe();
                (*__PAIR2PATH)[ *pair ] = *__PATHSET;   // store path
                _pairs.erase( pair );                   // remove found pair
                // scan pairs for new min,max IDs
                if ((pair->first == __ONEMATCH_MIN_ID) || (pair->second == __ONEMATCH_MAX_ID)) {
                    __ONEMATCH_MIN_ID = __MAX_ID;
                    __ONEMATCH_MAX_ID = -1;
                    for (ID2IDSetCIter p=_pairs.begin(); p != _pairs.end(); ++p) {
                        if (p->first  < __ONEMATCH_MIN_ID) __ONEMATCH_MIN_ID = p->first;
                        if (p->second > __ONEMATCH_MAX_ID) __ONEMATCH_MAX_ID = p->second;
                    }
                    printf( " new MIN,MAX (%d,%d)", __ONEMATCH_MIN_ID, __ONEMATCH_MAX_ID );
                }
                printf( "\n" );
            }
        }
    }
        
    //
    // step down the children unless all paths are found
    // if either ID is lower than highest ID in the set of ID-pairs
    // or the node has no probes
    //
    if ((id < __ONEMATCH_MAX_ID) || (! has_probes)) {
        for (PS_NodeMapConstIterator i = _ps_node->getChildrenBegin();
             (i != _ps_node->getChildrenEnd()) && (!_pairs.empty());
             ++i) {
            PS_find_probes_for_pairs( i->second, _pairs );
        }
=======
    // continue with the rest of the list
    if (nodeNum < 0) nodeNum = -nodeNum;
    for ( ; upperIndex != _upper_nodes.rend(); ++upperIndex) {
        _map->triangle_set( nodeNum, abs(*upperIndex), true );
        //printf( "(%i|%i) ", nodeNum, *upperIndex );
>>>>>>> 1.5
    }
<<<<<<< ps_detect_weak_differences.cxx

    //
    // remove ID from path
    //
    __PATHSET->erase( id );
=======
>>>>>>> 1.5
}

<<<<<<< ps_detect_weak_differences.cxx
void PS_print_and_evaluate_map( const PS_NodePtr _root_node ) {
    //
    // print and evaluate bitmap
    //
    printf( "\n\n----------------- bitmap ---------------\n\n" );
    SpeciesID smaller_id;
    SpeciesID bigger_id;
    ID2IDSet  noMatch;
    ID2IDSet  oneMatch;
    bool      bit1;
    bool      bit2;
    __ONEMATCH_MIN_ID = __MAX_ID;
    __ONEMATCH_MAX_ID = -1;
    for (SpeciesID id1 = 0; id1 <= __MAX_ID; ++id1) {
//         printf( "[%6i] ",id1 );
        for (SpeciesID id2 = 0; id2 <= id1; ++id2) {
            smaller_id = (id1 < id2) ? id1 : id2;
            bigger_id  = (id1 < id2) ? id2 : id1;
            bit1       = __MAP->get( smaller_id, bigger_id );
            bit2       = __MAP->get( bigger_id, smaller_id );
            if (bit1 && bit2) {
//                 printf( "2" );
            } else if (bit1) {
//                 printf( "1" );
                oneMatch.insert( ID2IDPair(smaller_id,bigger_id) );
                if (smaller_id < __ONEMATCH_MIN_ID) __ONEMATCH_MIN_ID = smaller_id;
                if (bigger_id  > __ONEMATCH_MAX_ID) __ONEMATCH_MAX_ID = bigger_id;
            } else {
//                 printf( "0" );
                if (id1 != id2) noMatch.insert( ID2IDPair(smaller_id,bigger_id) ); // there are no probes to distinguish a species from itself .. obviously
            }
        }
//         printf( "\n" );
=======
void PS_detect_weak_differences( const PS_NodePtr _ps_root_node, SpeciesID _max_ID ) {
    PS_BitMap *theMap = new PS_BitMap( false,_max_ID,PS_BitMap::TRIANGULAR );
    IDvector   upperNodes;
    
    printf( "PS_detect_weak_differences_stepdown :      1 of %i (%6.2f%%)\n",_max_ID,(1/(float)_max_ID)*100 );
    for (PS_NodeMapConstIterator i = _ps_root_node->getChildrenBegin(); i != _ps_root_node->getChildrenEnd(); ++i ) {
        //        if (i->first % 10 == 0)
        printf( "PS_detect_weak_differences_stepdown : %6i of %i (%6.2f%%)\n",i->first+1,_max_ID,((i->first+1)/(float)_max_ID)*100 );
        PS_detect_weak_differences_stepdown( i->second, theMap, upperNodes );
        if (!upperNodes.empty()) upperNodes.clear();
>>>>>>> 1.5
    }
<<<<<<< ps_detect_weak_differences.cxx
    printf( "(enter to continue)\n" );
//    getchar();
=======
    printf( "PS_detect_weak_differences_stepdown : %6i of %i (%6.2f%%)\n",_max_ID,_max_ID,100.0 );
>>>>>>> 1.5

<<<<<<< ps_detect_weak_differences.cxx
    printf( "\n\n----------------- no matches ---------------\n\n" );
    for (ID2IDSetCIter i = noMatch.begin(); i != noMatch.end(); ++i) {
        printf( "%6i %6i\n", i->first, i->second );
    }
    printf( "%u no matches\n(enter to continue)\n", noMatch.size() );
//    getchar();
=======
    getchar();
    theMap->print();
    printf( "(enter to continue)\n" );
//     getchar();
>>>>>>> 1.5

    printf( "\n\n----------------- one match ---------------\n\n" );
    for (ID2IDSetCIter i = oneMatch.begin(); i != oneMatch.end(); ++i) {
        printf( "%6i %6i\n", i->first, i->second );
    }
    printf( "%u one matches\n(enter to continue)\n", oneMatch.size() );
//    getchar();
    //
    // find paths for pairs
    //
    __PATHSET   = new IDSet;
    __PAIR2PATH = new IDID2IDSetMap;
    int c = 0;
    for (PS_NodeMapConstIterator i = _root_node->getChildrenBegin();
         (i != _root_node->getChildrenEnd()) && (!oneMatch.empty());
         ++i,++c ) {
//         if (c % 100 == 0) printf( "PS_find_probes_for_pairs( %i ) : %i of %i\n", i->first, c+1, _root_node->countChildren() );
        PS_find_probes_for_pairs( i->second, oneMatch );
    }
    //
    // print paths
    //
    for (IDID2IDSetMapCIter i = __PAIR2PATH->begin();
         i != __PAIR2PATH->end();
         ++i) {
        printf( "Pair (%i,%i) Setsize (%d)", i->first.first, i->first.second, i->second.size() );
        PS_NodePtr current_node = _root_node;
        long c = 0;
        for (IDSetCIter path_id=i->second.begin();
             path_id !=i->second.end();
             ++path_id,++c) {
            current_node = current_node->getChild( *path_id ).second;
            if (c % 10 == 0) printf( "\n" );
            printf( "%6i%s ", *path_id, (current_node->hasProbes()) ? ((current_node->hasInverseProbes()) ? "*" : "+") : "-" );
        }
        printf( "\nFinal Node : %p ", &(*current_node) );
        current_node->printOnlyMe();
        printf( "\n\n" );
    }
    printf( "%u paths\n", __PAIR2PATH->size() );
    //
    // oups
    //
    if (!oneMatch.empty()) {
        printf( "did not find a path for these :\n" );
        for (ID2IDSetCIter i = oneMatch.begin(); i != oneMatch.end(); ++i) {
            printf( "%6i %6i\n", i->first, i->second );
        }
    }
    //
    // make preset map
    //
    PS_BitMap_Counted *preset = new PS_BitMap_Counted( false, __MAX_ID+1 );
    // iterate over paths
    for (IDID2IDSetMapCIter i = __PAIR2PATH->begin();
         i != __PAIR2PATH->end();
         ++i) {
        // iterate over all IDs except path
        IDSetCIter next_path_id = i->second.begin();
        for (SpeciesID id = 0; id <= __MAX_ID; ++id) {
            if (id == *next_path_id) {  // if i run into a ID in path
                ++next_path_id;         // advance to next path ID
                continue;               // skip this ID
            }
            // iterate over path IDs
            for (IDSetCIter path_id = i->second.begin(); path_id != i->second.end(); ++path_id) {
                if (id == *path_id) continue;   // obviously an probe cant differ a species from itself
                if (id > *path_id) {
                    preset->set( id,*path_id,true );
                } else {
                    preset->set( *path_id,id,true );
                }
            }
        }
    }
    preset->print();
    delete preset;
    delete __PATHSET;
    delete __PAIR2PATH;
}

//  ====================================================
//  ====================================================

int main( int argc,  char *argv[] ) {

<<<<<<< ps_detect_weak_differences.cxx
   // open probe-set-database
    if (argc < 2) {
        printf("Missing argument\n Usage %s <database name> [[-]bitmap filename]\n",argv[0]);
=======
    // open probe-set-database
    if (argc < 3) {
        printf("Missing arguments\n Usage %s <database name> <map name>\n",argv[0]);
>>>>>>> 1.5
        exit(1);
    }

<<<<<<< ps_detect_weak_differences.cxx
    const char *input_DB_name = argv[1];
=======
    const char  *input_DB_name = argv[1];
    const char  *map_file_name = argv[2];
    unsigned int ID_count = 0;
    
    printf( "Opening probe-set-database '%s'..\n", map_file_name );
    PS_FileBuffer *fb = new PS_FileBuffer( map_file_name, PS_FileBuffer::READONLY );
    fb->get_uint( ID_count );
    printf( "there should be %i species in the database you specified ;)\n", ID_count );

>>>>>>> 1.5
    printf( "Opening probe-set-database '%s'..\n", input_DB_name );
<<<<<<< ps_detect_weak_differences.cxx
    PS_Database *db = new PS_Database( input_DB_name, PS_Database::READONLY );
    db->load();
    __MAX_ID = db->getMaxID();
=======
    PS_NodePtr root(new PS_Node(-1));
    fb->reinit( input_DB_name, PS_FileBuffer::READONLY );
    root->load( fb );
>>>>>>> 1.5
    printf( "loaded database (enter to continue)\n" );
//    getchar();


<<<<<<< ps_detect_weak_differences.cxx
    __MAP = new PS_BitMap_Fast( false, __MAX_ID+1 );
    if ((argc < 3) || (argv[2][0] != '-')) {
        printf( "PS_detect_weak_differences\n" );
        PS_detect_weak_differences( db->getRootNode() );
        if (argc > 2) {
            printf( "saving bitmap to file %s\n", argv[2] );
            PS_FileBuffer *mapfile = new PS_FileBuffer( argv[2], PS_FileBuffer::WRITEONLY );
            __MAP->save( mapfile );
            delete mapfile;
        }
    } else {
        printf( "loading bitmap from file %s\n",argv[2]+1 );
        PS_FileBuffer *mapfile = new PS_FileBuffer( argv[2]+1, PS_FileBuffer::READONLY );
        __MAP->load( mapfile );
        delete mapfile;
    }
    printf( "(enter to continue)\n" );
//    getchar();
=======
    PS_detect_weak_differences( root, ID_count );
>>>>>>> 1.5

    printf( "PS_print_and_evaluate_map\n" );
    PS_print_and_evaluate_map( db->getRootNode() );
    printf( "(enter to continue)\n" );
//    getchar();
    delete __MAP;
    
    printf( "removing database from memory\n" );
    delete db;
    printf( "(enter to continue)\n" );
//    getchar();

    return 0;
}
