#ifndef PS_BITMAP_HXX
#define PS_BITMAP_HXX

#ifndef PS_BITSET_HXX
#include "ps_bitset.hxx"
#endif

//  ############################################################
//  # PS_BitMap
//  ############################################################
class PS_BitMap {
protected:

    PS_BitSet **data;                   // array of pointers to PS_BitSet
    bool bias;                          // preset for bitmap
    long max_row;                       // max used row index
    long capacity;                      // number of allocated rows (and columns)

    PS_BitMap( const PS_BitMap& );
    PS_BitMap();
    PS_BitMap( const bool _bias, const long _max_row, const long _capacity ) {
        bias     = _bias;
        max_row  = _max_row;
        capacity = _capacity;
    }

public:

    virtual bool get( const long _row, const long _col );
    virtual bool set( const long _row, const long _col, const bool _value );
    // triangle_ - functions reverse _row and _col if _row is greater than _col
    // the resulting map is only triangular
    bool triangle_get( const long _row, const long _col );
    bool triangle_set( const long _row, const long _col, const bool _value );

    virtual bool reserve( const long _capacity );
    virtual void invert();
    virtual void xor( const PS_BitMap *_other_map );

    virtual void print();
    virtual bool save( PS_FileBuffer *_file );
    virtual bool load( PS_FileBuffer *_file );

    explicit PS_BitMap( const bool _bias, const long _capacity ) : bias(_bias), max_row(0), capacity(_capacity) {
        data = (PS_BitSet **)malloc(capacity * sizeof(PS_BitSet*));
        //fprintf( stderr, "PS_BitMap(%p) malloc(%u) = %p\n", this, sizeof(PS_BitSet*), data );
        for (long i = 0; i < capacity; ++i) {                        // init requested bitsets
            data[i] = new PS_BitSet( bias, capacity );               // init field
            if (data[i] == 0) {
                printf( "PS_BitMap( %s,%li ) : error while init. data[%li]\n", bias ? "true" : "false", capacity, i );
                exit( 1 );
            }
        }
    }

    explicit PS_BitMap( PS_FileBuffer *_file ) : bias(false), max_row(0), capacity(0) {
        data = 0;
        load( _file );
    }

    virtual ~PS_BitMap() {
        for (long i = 0; i < capacity; ++i) {
            delete data[i];
        }
        if (data) free( data );
    }
};


//  ############################################################
//  # PS_BitMap_Fast
//  ############################################################
//   No checks are made to assure that given row/col indices
//   point to an allocated byte. Make sure you reserve the needed
//   space either at creation time or before accessing the bits.
//
class PS_BitMap_Fast : public PS_BitMap {
private:

    PS_BitMap_Fast( const PS_BitMap_Fast& );
    PS_BitMap_Fast();

public:

    bool get( const long _row, const long _col );
    bool set( const long _row, const long _col, const bool _value );
    void setTrue(  const long _row, const long _col );
    void setFalse( const long _row, const long _col );

    bool reserve( const long _capacity );

    explicit PS_BitMap_Fast( const bool _bias, const long _capacity ) : PS_BitMap(_bias,0,_capacity) {
        data = (PS_BitSet **)malloc(capacity * sizeof(PS_BitSet*));
        //fprintf( stderr, "PS_BitMap(%p) malloc(%u) = %p\n", this, sizeof(PS_BitSet*), data );
        for (long i = 0; i < capacity; ++i) {                        // init requested bitsets
            data[i] = new PS_BitSet_Fast( bias, capacity );          // init field
            if (data[i] == 0) {
                printf( "PS_BitMap_Fast( %s,%li ) : error while init. data[%li]\n", bias ? "true" : "false", capacity, i );
                exit( 1 );
            }
        }
    }
};


//  ############################################################
//  # PS_BitMap_Counted
//  ############################################################
//   PS_BitMap_Counted is ALWAYS triangular.
//   This means that the max column index in a row is the row number.
//   The resulting bitmap is the lower left half
//     012
//   0 .
//   1 ..
//   2 ...
//   Therefore the row index must be higher or equal than the column index
//   when you call set/get-functions, which do not check given row,col.
//   If you cannot assure this use the triangleSet/Get-functions.
//
//   count_true_per_index counts TRUEs in a row + the TRUEs in the
//   column of the same index.
//   Only set() automatically updates count_true_per_index.
//
class PS_BitMap_Counted : public PS_BitMap {
private:

    long *count_true_per_index;

    PS_BitMap_Counted( const PS_BitMap_Counted& );
    PS_BitMap_Counted();

public:

    bool get( const long _row, const long _col );
    bool set( const long _row, const long _col, const bool _value );
    void setTrue(  const long _row, const long _col );
    void setFalse( const long _row, const long _col );

    bool reserve( const long _capacity );
    void recalcCounters();
    long getCountFor( long _index ) { return count_true_per_index[ _index ]; }

    void print();
    bool load( PS_FileBuffer *_file );

    explicit PS_BitMap_Counted( PS_FileBuffer *_file ) : PS_BitMap(false,0,0) {
        data                 = 0;
        count_true_per_index = 0;
        load( _file );
    }

    explicit PS_BitMap_Counted( const bool _bias, const long _capacity ) : PS_BitMap(_bias,_capacity) {
        // alloc memory for counters
        count_true_per_index = (long *)malloc( capacity * sizeof(long) );
        // preset memory of counters
        memset( count_true_per_index, (bias) ? capacity : 0, capacity * sizeof(long) );
    }

    ~PS_BitMap_Counted() {
        if (count_true_per_index) free( count_true_per_index );
    }
};


// ************************************************************
// * PS_BitMap::set( _row,_col,_value )
// ************************************************************
bool PS_BitMap::set( const long _row, const long _col, const bool _value ) {
    reserve( _row+1 );
    if (_row > max_row) max_row = _row;
    return data[_row]->set( _col, _value );
}


// ************************************************************
// * PS_BitMap::get( _row,_col )
// ************************************************************
bool PS_BitMap::get( const long _row, const long _col ) {
    reserve( _row+1 );
    return data[_row]->get( _col );
}


// ************************************************************
// * PS_BitMap::triangle_set( _row,_col,_value )
// ************************************************************
bool PS_BitMap::triangle_set( const long _row, const long _col, const bool _value ) {
    if (_row > _col) {
        return set( _col,_row,_value );
    } else {
        return set( _row,_col,_value );
    }
}


// ************************************************************
// * PS_BitMap::triangle_get( _row,_col )
// ************************************************************
bool PS_BitMap::triangle_get( const long _row, const long _col ) {
    if (_row > _col) {
        return get( _col,_row );
    } else {
        return get( _row,_col );
    }
}


// ************************************************************
// * PS_BitMap::reserve( _capacity )
// ************************************************************
bool PS_BitMap::reserve( const long _capacity ) {
    PS_BitSet **new_data;
    long new_capacity_bytes = _capacity * sizeof(PS_BitSet*);
    long old_capacity_bytes =  capacity * sizeof(PS_BitSet*);
    if (_capacity <= capacity) return true;                      // smaller or same size requested ?
    new_data = (PS_BitSet **)malloc( new_capacity_bytes );       // get new memory for pointer array
    if (new_data == 0) return false;
    if (capacity > 0) memcpy( new_data,data,old_capacity_bytes );// copy old pointers
    if (data) free( data );                                      // free old memory
    data = new_data;
    for (long i = capacity; i < _capacity; ++i) {                // init new requested bitsets
        data[i] = new PS_BitSet( bias,1 );                       // init field
        if (data[i] == 0) return false;                          // check success
    }
    capacity = _capacity;                                        // store new capacity
    return true;
}


// ************************************************************
// * PS_BitMap::invert()
// ************************************************************
void PS_BitMap::invert() {
    for (long i = 0; i <= max_row; ++i) {
        data[i]->invert();
    }
}


// ************************************************************
// * PS_BitMap::xor( _other_map )
// ************************************************************
void PS_BitMap::xor( const PS_BitMap *_other_map ) {
    for (long i = 0; i <= max_row; ++i) {
        data[i]->xor( _other_map->data[i] );
    }
}


// ************************************************************
// * PS_BitMap::print()
// ************************************************************
void PS_BitMap::print() {
    printf( "PS_BitMap : bias(%1i) max_row(%6li) capacity(%6li)\n", bias, max_row, capacity );
    for (long i = 0; i <= max_row; ++i) {
        printf( "[%5li] ",i);
        data[i]->print( true, i );
    }
}


// ************************************************************
// * PS_BitMap::save( PS_FileBuffer *_file )
// ************************************************************
bool PS_BitMap::save( PS_FileBuffer *_file ) {
    if (_file->isReadonly()) return false;
    // save max_row
    _file->put_long( max_row );
    // save bias
    _file->put_char( (bias) ? '1' : '0' );
    // save bitsets
    for (long i = 0; i <= max_row; ++i) {
        data[i]->save( _file );
    }
    return true;
}


// ************************************************************
// * PS_BitMap::load( PS_FileBuffer *_file )
// ************************************************************
bool PS_BitMap::load( PS_FileBuffer *_file ) {
    if (!_file->isReadonly()) return false;
    // load max_row
    _file->get_long( max_row );
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitmap
    if (!reserve( max_row+1 )) return false;
    for (long i = 0; i <= max_row; ++i) {
        data[i]->load( _file );
    }
    return true;
}


// ************************************************************
// * PS_BitMap_Fast::set( _row, _col, _value )
// ************************************************************
bool PS_BitMap_Fast::set( const long _row, const long _col, const bool _value ) {
    if (_row > max_row) max_row = _row;
    return data[_row]->set( _col, _value );
}


// ************************************************************
// * PS_BitMap_Fast::get( _row, _col )
// ************************************************************
bool PS_BitMap_Fast::get( const long _row, const long _col ) {
    if (_row > max_row) max_row = _row;
    return data[_row]->get( _col );
}


// ************************************************************
// * PS_BitMap_Fast::setTrue( _row, _col )
// ************************************************************
void PS_BitMap_Fast::setTrue( const long _row, const long _col ) {
    if (_row > max_row) max_row = _row;
    data[_row]->setTrue( _col );
}


// ************************************************************
// * PS_BitMap_Fast::setFalse( _row, _col )
// ************************************************************
void PS_BitMap_Fast::setFalse( const long _row, const long _col ) {
    if (_row > max_row) max_row = _row;
    data[_row]->setFalse( _col );
}


// ************************************************************
// * PS_BitMap_Fast::reserve( _capacity )
// ************************************************************
bool PS_BitMap_Fast::reserve( const long _capacity ) {
    if (_capacity <= capacity) return true;                      // smaller or same size requested ?
    PS_BitSet **new_data;
    long new_capacity_bytes = _capacity * sizeof(PS_BitSet*);
    long old_capacity_bytes =  capacity * sizeof(PS_BitSet*);
    new_data = (PS_BitSet **)malloc( new_capacity_bytes );       // get new memory for pointer array
    if (new_data == 0) return false;
    if (capacity > 0) memcpy( new_data,data,old_capacity_bytes );// copy old pointers
    if (data) free( data );                                      // free old memory
    data = new_data;
    for (long i = capacity; i < _capacity; ++i) {                // init new requested bitsets
        data[i] = new PS_BitSet_Fast( bias,1 );                  // init field
        if (data[i] == 0) return false;                          // check success
    }
    capacity = _capacity;                                        // store new capacity
    return true;
}


// ************************************************************
// * PS_BitMap_Counted::set( _row,_col,_value )
// ************************************************************
bool PS_BitMap_Counted::set( const long _row, const long _col, const bool _value ) {
    if (_row > max_row) max_row = _row;
    bool previous_value = data[_row]->set( _col, _value );
    if (_value && !previous_value) {
        ++count_true_per_index[ _row ];
        ++count_true_per_index[ _col ];
    } else if (!_value && previous_value) {
        --count_true_per_index[ _row ];
        --count_true_per_index[ _col ];
    }
    return previous_value;
}


// ************************************************************
// * PS_BitMap_Counted::get( _row,_col )
// ************************************************************
bool PS_BitMap_Counted::get( const long _row, const long _col ) {
    return data[_row]->get( _col );
}


// ************************************************************
// * PS_BitMap_Counted::setTrue( _row, _col )
// ************************************************************
void PS_BitMap_Counted::setTrue( const long _row, const long _col ) {
    if (_row > max_row) max_row = _row;
    data[_row]->setTrue( _col );
}


// ************************************************************
// * PS_BitMap_Counted::setFalse( _row, _col )
// ************************************************************
void PS_BitMap_Counted::setFalse( const long _row, const long _col ) {
    if (_row > max_row) max_row = _row;
    data[_row]->setFalse( _col );
}


// ************************************************************
// * PS_BitMap_Counted::reserve( _capacity )
// ************************************************************
bool PS_BitMap_Counted::reserve( const long _capacity ) {
    PS_BitSet **new_data;
    long *new_counts;
    if (_capacity <= capacity) return true;                      // smaller or same size requested ?
    long new_data_bytes   = _capacity * sizeof(PS_BitSet*);
    long old_data_bytes   =  capacity * sizeof(PS_BitSet*);
    long new_counts_bytes = _capacity * sizeof(long);
    long old_counts_bytes =  capacity * sizeof(long);
    //
    // allocate new memory
    //
    new_data   = (PS_BitSet **)malloc( new_data_bytes );
    new_counts = (long *)malloc( new_counts_bytes );
    //
    // test is we got the memory we wanted
    //
    if (!new_data || !new_counts) {
        // failed to allocate all memory so give up the parts we got
        if (new_data)   free( new_data );
        if (new_counts) free( new_counts );
        return false;
    }
    //
    // initialize new data-array
    //
    if (capacity > 0) memcpy( new_data,data,old_data_bytes );    // copy old pointers
    if (data) free( data );                                      // free old memory
    data = new_data;
    for (long i = capacity; i < _capacity; ++i) {                // init new requested bitsets
        data[i] = new PS_BitSet_Fast( bias,i+1 );                // init field
        if (data[i] == 0) return false;                          // check success
    }
    //
    // initialize new counts-arrays
    //
    if (capacity > 0) memcpy( new_counts, count_true_per_index, old_counts_bytes );
    memset( new_counts+old_counts_bytes, 0, new_counts_bytes-old_counts_bytes );
    if (count_true_per_index) free( count_true_per_index );
    count_true_per_index = new_counts;

    capacity = _capacity;                                        // store new capacity
    return true;
}


// ************************************************************
// * PS_BitMap_Counted::print()
// ************************************************************
void PS_BitMap_Counted::print() {
    printf( "PS_BitMap_Counted : bias(%1i) max_row(%6li) capacity(%6li)\n", bias, max_row, capacity );
    for (long i = 0; i <= max_row; ++i) {
        printf( "[%5li] %6li ", i, count_true_per_index[i] );
        data[i]->print( false );
    }
}


// ************************************************************
// * PS_BitMap_Counted::load( PS_FileBuffer *_file )
// ************************************************************
bool PS_BitMap_Counted::load( PS_FileBuffer *_file ) {
    if (!PS_BitMap::load( _file )) return false;
    recalcCounters();
    return true;
}


// ************************************************************
// * PS_BitMap_Counted::recalcCounters()
// ************************************************************
void PS_BitMap_Counted::recalcCounters() {
    memset( count_true_per_index, 0, capacity * sizeof(long) );
    for (long row = 0; row <= max_row; ++row) {
        PS_BitSet *row_data = data[row];
        for (long col = 0; col <= row_data->getMaxUsedIndex(); ++col) {
            if (row_data->get(col)) {
                ++count_true_per_index[ col ];
                ++count_true_per_index[ row ];
            }
        }
    }
}

#else
#error ps_bitmap.hxx included twice
#endif
