//  =============================================================== //
//                                                                  //
//    File      : ps_node.cxx                                       //
//    Purpose   :                                                   //
//    Time-stamp: <Thu Jun/13/2002 12:36 MET Coder@ReallySoft.de>   //
//                                                                  //
//    Coded by Ralf Westram (coder@reallysoft.de) in June 2002      //
//    Institute of Microbiology (Technical University Munich)       //
//    http://www.arb-home.de/                                       //
//                                                                  //
//  =============================================================== //

#include "ps_node.hxx"
#include <unistd.h>

using namespace std;


//
// *** disk output ASCII **
//
bool PS_Node::saveASCII( PS_FileBuffer* _fb, char *buffer ) { // buffer MUST be at least 100 chars long
    unsigned int size;
    int          count;
    //
    // write num
    //
    count = sprintf( buffer, "N[%i", num );
    _fb->put( buffer,count );
    //
    // write probes
    //
    size = (probes == 0) ? 0 : probes->size();
    if (size) {
        count = sprintf( buffer, " P{%i", size );
        _fb->put( buffer, count );
	for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
            count = sprintf( buffer, " (%i_%i_%i)", (*i)->quality, (*i)->length, (*i)->GC_content );
	    _fb->put( buffer, count );
	}
        _fb->put_char( '}' );
    }
    //
    // write children
    //
    size  = children.size();
    if (size) {
        count = sprintf( buffer, " C<%i", size );
        _fb->put( buffer, count );
        for (PS_NodeMapIterator i=children.begin(); i!=children.end(); ++i) {
            _fb->put_char( '\n' );
            if (num == -1) _fb->put_char( '+' );
            i->second->saveASCII( _fb,buffer );
        }
        _fb->put_char( '>' );
    }
    //
    // return true to signal success
    //
    _fb->put_char( ']' );
    return true;
}


//
// *** disk output **
//
bool PS_Node::save( PS_FileBuffer* _fb ) {
    unsigned int size;
    //
    // write num
    //
    _fb->put_int( num );
    //
    // write probes
    //
    size = (probes == 0) ? 0 : probes->size();
    _fb->put_uint( size );
    if (size) {
        PS_Probe p;
	for (PS_ProbeSetIterator i=probes->begin(); i!=probes->end(); ++i) {
            p = **i;
	    _fb->put( &p, sizeof(PS_Probe) );
	}
    }
    //
    // write children
    //
    size = children.size();
    _fb->put_uint( size );
    for (PS_NodeMapIterator i=children.begin(); i!=children.end(); ++i) {
        i->second->save( _fb );
    }
    //
    // return true to signal success
    //
    return true;
}


//
// *** disk input **
//
bool PS_Node::load( PS_FileBuffer* _fb ) {
    unsigned int size;
    //
    // read num
    //
    _fb->get_int( num );
    //
    // read probes
    //
    _fb->get_uint( size );
    if (size) {               // does node have probes ?
	probes = new PS_ProbeSet;                                 // make new probeset
	for (unsigned int i=0; i<size; ++i) {
            PS_Probe *p = new PS_Probe;
	    _fb->get( p, sizeof(PS_Probe) );                      // read new probe
	    PS_ProbePtr new_probe(p);                             // make new probe-smartpointer
	    probes->insert( new_probe );                          // append new probe to probeset
	}
    } else {
	probes = 0;                                               // unset probeset
    }
    //
    // read children
    //
    _fb->get_uint( size );
    if (num == -1) {
        for (unsigned int i=0; i<size; ++i) {
            PS_NodePtr new_child(new PS_Node(-1));                // make new child
            new_child->load( _fb );                               // read new child
            children[new_child->getNum()] = new_child;            // insert new child to childmap
            if (i % 200 == 0) printf( "loaded 1st level #%i\n",new_child->getNum() );
        }
    } else {
        for (unsigned int i=0; i<size; ++i) {
            PS_NodePtr new_child(new PS_Node(-1));                // make new child
            new_child->load( _fb );                               // read new child
            children[new_child->getNum()] = new_child;            // insert new child to childmap
        }
    }
    // return true to signal success
    return true;
}


//
// *** disk input appending **
//
bool PS_Node::append( PS_FileBuffer* _fb ) {
    unsigned int size;
    //
    // read num if root
    //
    if (num == -1) {
        _fb->get_int( num );
    }
    //
    // read probes
    //
    _fb->get_uint( size );
    if (size) {               // does node have probes ?
	if (!probes) probes = new PS_ProbeSet;                    // make new probeset
	for (unsigned int i=0; i<size; ++i) {
            PS_Probe *p = new PS_Probe;
	    _fb->get( p, sizeof(PS_Probe) );                      // read new probe
	    PS_ProbePtr new_probe(p);                             // make new probe-smartpointer
	    probes->insert( new_probe );                          // append new probe to probeset
	}
    }
    //
    // read children
    //
    _fb->get_uint( size );
    for (unsigned int i=0; i<size; ++i) {
        //
        // read num of child
        //
        SpeciesID childNum;
        _fb->get_int( childNum );
        if ((num == -1) && (i % 200 == 0)) printf( "appended 1st level #%i\n", childNum );
        //
        // test if child already exists
        //
        PS_NodeMapIterator it = children.find( childNum );
        if (it != children.end()) {
            it->second->append( _fb );                           // 'update' child
        } else {
            PS_NodePtr newChild(new PS_Node(childNum));          // make new child
            newChild->append( _fb );                             // read new child
            children[childNum] = newChild;                       // insert new child to childmap
        }
    }
    // return true to signal success
    return true;
}
