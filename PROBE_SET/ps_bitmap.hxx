#ifndef PS_BITMAP_HXX
#define PS_BITMAP_HXX

#ifndef __MAP__
#include <map>
#endif
#ifndef PS_BITSET_HXX
#include "ps_bitset.hxx"
#endif

class PS_BitMap : virtual Noncopyable {
protected:

    PS_BitSet **data; // array of pointers to PS_BitSet
    
    bool bias;     // preset for bitmap
    long max_row;  // max used row index
    long capacity; // number of allocated rows (and columns)

    virtual bool do_reserve(const long _capacity, const bool _init_sets);

    PS_BitMap(const bool _bias, const long _max_row, const long _capacity)
        : data(NULL),
          bias(_bias),
          max_row(_max_row),
          capacity(_capacity)
    {}

public:

    virtual long getTrueIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set);
    virtual long getFalseIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set);
    virtual long getCountOfTrues();

    virtual bool get(const long _row, const long _col);
    virtual bool set(const long _row, const long _col, const bool _value);

    virtual void setTrue(const long _row, const long _col)  = 0;
    virtual void setFalse(const long _row, const long _col) = 0;

    // triangle_ - functions reverse _row and _col if _row is greater than _col
    // the resulting map is only triangular
            bool triangle_get(const long _row, const long _col);
            bool triangle_set(const long _row, const long _col, const bool _value);

            void invert();
            void x_or(const PS_BitMap *_other_map);

    virtual void print(FILE *out);
    virtual void printGNUplot(const char *_title, char *_buffer, PS_FileBuffer *_file);
            bool save(PS_FileBuffer *_file);
    virtual bool load(PS_FileBuffer *_file);

            bool copy(const PS_BitMap *_other_bitmap);
    virtual bool reserve(const long _capacity);

    explicit PS_BitMap(const bool _bias, const long _capacity) : bias(_bias), max_row(0), capacity(_capacity) {
        data = (PS_BitSet **)malloc(capacity * sizeof(PS_BitSet*));
        for (long i = 0; i < capacity; ++i) {                        // init requested bitsets
            data[i] = new PS_BitSet(bias, capacity);                 // init field
            if (data[i] == 0) {
                printf("PS_BitMap( %s,%li ) : error while init. data[%li]\n", bias ? "true" : "false", capacity, i);
                exit(1);
            }
        }
    }

    explicit PS_BitMap(PS_FileBuffer *_file) : bias(false), max_row(0), capacity(0) {
        data = 0;
        load(_file);
    }

    virtual ~PS_BitMap() {
        for (long i = 0; i < capacity; ++i) {
            if (data[i]) delete data[i];
        }
        if (data) free(data);
    }
};


//  ############################################################
//  # PS_BitMap_Fast
//  ############################################################
//   No checks are made to assure that given row/col indices
//   point to an allocated byte. Make sure you reserve the needed
//   space either at creation time or before accessing the bits.
//
class PS_BitMap_Fast FINAL_TYPE : public PS_BitMap {
private:

    bool copy(const PS_BitSet *_other_bitset);   // declared but not implemented
    bool do_reserve(const long _capacity, const bool _init_sets) OVERRIDE;

    PS_BitMap_Fast(const PS_BitMap_Fast&);
    PS_BitMap_Fast();

public:

    bool get(const long _row, const long _col) OVERRIDE;
    bool set(const long _row, const long _col, const bool _value) OVERRIDE;
    void setTrue(const long _row, const long _col) OVERRIDE;
    void setFalse(const long _row, const long _col) OVERRIDE;

    bool load(PS_FileBuffer *_file) OVERRIDE;

    bool copy(const PS_BitMap_Fast *_other_bitmap);
    bool reserve(const long _capacity) OVERRIDE;

    explicit PS_BitMap_Fast(const bool _bias, const long _capacity) : PS_BitMap(_bias, 0, _capacity) {
        data = (PS_BitSet **)malloc(capacity * sizeof(PS_BitSet*));
        for (long i = 0; i < capacity; ++i) {                        // init requested bitsets
            data[i] = new PS_BitSet_Fast(bias, capacity);            // init field
            if (data[i] == 0) {
                printf("PS_BitMap_Fast( %s,%li ) : error while init. data[%li]\n", bias ? "true" : "false", capacity, i);
                exit(1);
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
class PS_BitMap_Counted FINAL_TYPE : public PS_BitMap { // derived from a Noncopyable
    long *count_true_per_index;

    bool copy(const PS_BitSet *_other_bitset);   // declared but not implemented because PS_BitSet has no count_true_per_index array
    bool do_reserve(const long _capacity, const bool _init_sets) OVERRIDE;

    PS_BitMap_Counted(const PS_BitMap_Counted&);
    PS_BitMap_Counted();

public:

    void recalcCounters();
    long getCountFor(const long _index) { return count_true_per_index[_index]; }

    long getTrueIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set) OVERRIDE;
    long getFalseIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set) OVERRIDE;
    long getTrueIndicesForRow(const long _row, PS_BitSet::IndexSet &_index_set);
    long getFalseIndicesForRow(const long _row, PS_BitSet::IndexSet &_index_set);
    long getCountOfTrues() OVERRIDE;

    bool get(const long _row, const long _col) OVERRIDE;
    bool set(const long _row, const long _col, const bool _value) OVERRIDE;
    void setTrue(const long _row, const long _col) OVERRIDE;
    void setFalse(const long _row, const long _col) OVERRIDE;

    void print(FILE *out) OVERRIDE;
    void printGNUplot(const char *_title, char *_buffer, PS_FileBuffer *_file) OVERRIDE;
    bool load(PS_FileBuffer *_file) OVERRIDE;

    bool copy(const PS_BitMap_Counted *_other_bitmap);
    bool reserve(const long _capacity) OVERRIDE;

    explicit PS_BitMap_Counted(PS_FileBuffer *_file) : PS_BitMap(false, 0, 0) {
        data                 = 0;
        count_true_per_index = 0;
        load(_file);
    }

    explicit PS_BitMap_Counted(const bool _bias, const long _capacity) : PS_BitMap(_bias, 0, _capacity) {
        data = (PS_BitSet **)malloc(capacity * sizeof(PS_BitSet*));
        for (long i = 0; i < capacity; ++i) {                        // init requested bitsets
            data[i] = new PS_BitSet_Fast(bias, capacity);            // init field
            if (data[i] == 0) {
                printf("PS_BitMap_Counted( %s,%li ) : error while init. data[%li]\n", bias ? "true" : "false", capacity, i);
                exit(1);
            }
        }
        // alloc memory for counters
        count_true_per_index = (long *)malloc(capacity * sizeof(long));

        // preset memory of counters
        memset(count_true_per_index, (bias) ? capacity : 0, capacity * sizeof(long));
    }

    ~PS_BitMap_Counted() OVERRIDE {
        if (count_true_per_index) free(count_true_per_index);
    }
};


long PS_BitMap::getTrueIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set) {
    // get trues in the row
    data[_index]->getTrueIndices(_index_set, _index);
    // scan column _index in the remaining rows
    for (long row = _index+1; row <= max_row; ++row) {
        if (data[row]->Get(_index)) _index_set.insert(row);
    }
    return _index_set.size();
}


long PS_BitMap::getFalseIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set) {
    // get falses in the row
    data[_index]->getFalseIndices(_index_set, _index);
    // scan column _index in the remaining rows
    for (long row = _index+1; row <= max_row; ++row) {
        if (!(data[row]->Get(_index))) _index_set.insert(row);
    }
    return _index_set.size();
}


long PS_BitMap::getCountOfTrues() {
    long count = 0;
    // sum up trues in the rows
    for (long row = 0; row <= max_row; ++row) {
        count += data[row]->getCountOfTrues();
    }
    return count;
}


bool PS_BitMap::set(const long _row, const long _col, const bool _value) {
    reserve(_row+1);
    if (_row > max_row) max_row = _row;
    return data[_row]->Set(_col, _value);
}


bool PS_BitMap::get(const long _row, const long _col) {
    reserve(_row+1);
    return data[_row]->Get(_col);
}


bool PS_BitMap::triangle_set(const long _row, const long _col, const bool _value) {
    if (_row > _col) {
        return set(_col, _row, _value);
    }
    else {
        return set(_row, _col, _value);
    }
}


bool PS_BitMap::triangle_get(const long _row, const long _col) {
    if (_row > _col) {
        return get(_col, _row);
    }
    else {
        return get(_row, _col);
    }
}


bool PS_BitMap::copy(const PS_BitMap *_other_bitmap) {
    bias = _other_bitmap->bias;
    if (!do_reserve(_other_bitmap->capacity, false)) return false;
    for (long i = 0; i < capacity; ++i) {
        if (!data[i]) data[i] = new PS_BitSet(bias, _other_bitmap->data[i]->getCapacity());
        if (!data[i]->copy(_other_bitmap->data[i])) return false;
    }
    max_row = _other_bitmap->max_row;
    return true;
}


bool PS_BitMap::reserve(const long _capacity) {
    return do_reserve(_capacity, true);
}
bool PS_BitMap::do_reserve(const long _capacity, const bool _init_sets) {
    PS_BitSet **new_data;
    long new_capacity_bytes = _capacity * sizeof(PS_BitSet*);
    long old_capacity_bytes =  capacity * sizeof(PS_BitSet*);
    if (_capacity <= capacity) return true;                      // smaller or same size requested ?
    new_data = (PS_BitSet **)malloc(new_capacity_bytes);         // get new memory for pointer array
    if (new_data == 0) return false;
    if (capacity > 0) memcpy(new_data, data, old_capacity_bytes); // copy old pointers
    if (data) free(data);                                        // free old memory
    data = new_data;
    if (_init_sets) {
        for (long i = capacity; i < _capacity; ++i) {            // init new requested bitsets
            data[i] = new PS_BitSet(bias, 1);                    // init field
            if (data[i] == 0) return false;                      // check success
        }
    }
    capacity = _capacity;                                        // store new capacity
    return true;
}


void PS_BitMap::invert() {
    for (long i = 0; i <= max_row; ++i) {
        data[i]->invert();
    }
}


void PS_BitMap::x_or(const PS_BitMap *_other_map) {
    for (long i = 0; i <= max_row; ++i) {
        data[i]->x_or(_other_map->data[i]);
    }
}


void PS_BitMap::print(FILE *out) {
    fprintf(out, "PS_BitMap : bias(%1i) max_row(%6li) capacity(%6li)\n", bias, max_row, capacity);
    for (long i = 0; i < capacity; ++i) {
        fprintf(out, "[%5li] ", i);
        data[i]->print(out, true, i);
    }
}


void PS_BitMap::printGNUplot(const char *_title, char *_buffer, PS_FileBuffer *_file) {
    // write title
    long size;
    size = sprintf(_buffer, "# %s\n#PS_BitMap : bias(%1i) max_row(%li) capacity(%li)\n#col row\n", _title, bias, max_row, capacity);
    _file->put(_buffer, size);

    // write indices of trues per row
    PS_BitSet::IndexSet trues;
    for (long row = 0;
          row < capacity;
          ++row) {
        data[row]->getTrueIndices(trues);
        for (PS_BitSet::IndexSet::iterator col = trues.begin();
              col != trues.end();
              ++col) {
            size = sprintf(_buffer, "%li %li\n", *col, row);
            _file->put(_buffer, size);
        }
    }

    _file->flush();
}


bool PS_BitMap::save(PS_FileBuffer *_file) {
    if (_file->isReadonly()) return false;
    // save max_row
    _file->put_long(max_row);
    // save bias
    _file->put_char((bias) ? '1' : '0');
    // save bitsets
    for (long i = 0; i <= max_row; ++i) {
        data[i]->save(_file);
    }
    return true;
}


bool PS_BitMap::load(PS_FileBuffer *_file) {
    if (!_file->isReadonly()) return false;
    // load max_row
    _file->get_long(max_row);
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitmap
    if (!do_reserve(max_row+1, false)) return false;
    for (long i = 0; i <= max_row; ++i) {
        if (data[i]) {
            delete data[i];
        }
        data[i] = new PS_BitSet(_file);
    }
    return true;
}


inline bool PS_BitMap_Fast::set(const long _row, const long _col, const bool _value) {
    if (_row > max_row) max_row = _row;
    return data[_row]->Set(_col, _value);
}


inline bool PS_BitMap_Fast::get(const long _row, const long _col) {
    if (_row > max_row) max_row = _row;
    return data[_row]->Get(_col);
}


inline void PS_BitMap_Fast::setTrue(const long _row, const long _col) {
    if (_row > max_row) max_row = _row;
    data[_row]->setTrue(_col);
}


inline void PS_BitMap_Fast::setFalse(const long _row, const long _col) {
    if (_row > max_row) max_row = _row;
    data[_row]->setFalse(_col);
}


bool PS_BitMap_Fast::load(PS_FileBuffer *_file) {
    if (!_file->isReadonly()) return false;
    // load max_row
    _file->get_long(max_row);
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitmap
    if (!do_reserve(max_row+1, false)) return false;
    for (long i = 0; i <= max_row; ++i) {
        if (data[i]) {
            delete data[i];
        }
        data[i] = new PS_BitSet_Fast(_file);
    }
    return true;
}

bool PS_BitMap_Fast::copy(const PS_BitMap_Fast *_other_bitmap) {
    bias = _other_bitmap->bias;
    if (!do_reserve(_other_bitmap->capacity, false)) return false;
    for (long i = 0; i < capacity; ++i) {
        if (!data[i]) data[i] = new PS_BitSet_Fast(bias, _other_bitmap->data[i]->getCapacity());
        if (!data[i]->copy(_other_bitmap->data[i])) return false;
    }
    max_row = _other_bitmap->max_row;
    return true;
}

bool PS_BitMap_Fast::reserve(const long _capacity) {
    return do_reserve(_capacity, true);
}
bool PS_BitMap_Fast::do_reserve(const long _capacity, const bool _init_sets) {
    if (_capacity <= capacity) return true;                      // smaller or same size requested ?
    PS_BitSet **new_data;
    long new_capacity_bytes = _capacity * sizeof(PS_BitSet*);
    long old_capacity_bytes =  capacity * sizeof(PS_BitSet*);
    new_data = (PS_BitSet **)malloc(new_capacity_bytes);         // get new memory for pointer array
    if (new_data == 0) return false;
    if (capacity > 0) memcpy(new_data, data, old_capacity_bytes); // copy old pointers
    if (data) free(data);                                        // free old memory
    data = new_data;
    if (_init_sets) {
        for (long i = capacity; i < _capacity; ++i) {            // init new requested bitsets
            data[i] = new PS_BitSet_Fast(bias, 1);               // init field
            if (data[i] == 0) return false;                      // check success
        }
    }
    capacity = _capacity;                                        // store new capacity
    return true;
}


long PS_BitMap_Counted::getTrueIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set) {
    // get total number of trues
    unsigned long total_count_trues = count_true_per_index[_index];
    // get trues in the row
    data[_index]->getTrueIndices(_index_set, _index);
    // scan column _index in the remaining rows until all trues are found
    for (long row = _index+1;
         ((row <= max_row) && (_index_set.size() < total_count_trues));
         ++row) {
        if (data[row]->Get(_index)) _index_set.insert(row);
    }
    return _index_set.size();
}


long PS_BitMap_Counted::getFalseIndicesFor(const long _index, PS_BitSet::IndexSet &_index_set) {
    // get total number of falses
    unsigned long total_count_falses = capacity - count_true_per_index[_index];
    // get falses in the row
    data[_index]->getFalseIndices(_index_set, _index);
    // scan column _index in the remaining rows until all falses are found
    for (long row = _index+1;
         ((row <= max_row) && (_index_set.size() < total_count_falses));
         ++row) {
        if (!(data[row]->Get(_index))) _index_set.insert(row);
    }
    return _index_set.size();
}


long PS_BitMap_Counted::getTrueIndicesForRow(const long _row, PS_BitSet::IndexSet &_index_set) {
    // get trues in the row
    data[_row]->getTrueIndices(_index_set, _row);
    return _index_set.size();
}


long PS_BitMap_Counted::getFalseIndicesForRow(const long _row, PS_BitSet::IndexSet &_index_set) {
    // get falses in the row
    data[_row]->getFalseIndices(_index_set, _row);
    return _index_set.size();
}


long PS_BitMap_Counted::getCountOfTrues() {
    long count = 0;
    // sum up trues in the rows
    for (long row = 0; row <= max_row; ++row) {
        count += data[row]->getCountOfTrues(row);
    }
    return count;
}


bool PS_BitMap_Counted::set(const long _row, const long _col, const bool _value) {
    if (_col > _row) printf("PS_BitMap_Counted::set( %li,%li,%1i ) not allowed\n", _row, _col, _value);
    if (_row > max_row) max_row = _row;
    bool previous_value = data[_row]->Set(_col, _value);
    if (_value && !previous_value) {
        ++count_true_per_index[_row];
        ++count_true_per_index[_col];
    }
    else if (!_value && previous_value) {
        --count_true_per_index[_row];
        --count_true_per_index[_col];
    }
    return previous_value;
}


inline bool PS_BitMap_Counted::get(const long _row, const long _col) {
    if (_col > _row) printf("PS_BitMap_Counted::get( %li,%li ) not allowed\n", _row, _col);
    if (_row > max_row) max_row = _row;
    return data[_row]->Get(_col);
}


inline void PS_BitMap_Counted::setTrue(const long _row, const long _col) {
    if (_col > _row) printf("PS_BitMap_Counted::setTrue( %li,%li ) not allowed\n", _row, _col);
    if (_row > max_row) max_row = _row;
    data[_row]->setTrue(_col);
}


inline void PS_BitMap_Counted::setFalse(const long _row, const long _col) {
    if (_col > _row) printf("PS_BitMap_Counted::setFalse( %li,%li ) not allowed\n", _row, _col);
    if (_row > max_row) max_row = _row;
    data[_row]->setFalse(_col);
}


bool PS_BitMap_Counted::load(PS_FileBuffer *_file) {
    if (!_file->isReadonly()) return false;
    // load max_row
    _file->get_long(max_row);
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitmap
    if (!do_reserve(max_row+1, false)) return false;
    for (long i = 0; i <= max_row; ++i) {
        if (data[i]) {
            delete data[i];
        }
        data[i] = new PS_BitSet_Fast(_file, i);
    }
    recalcCounters();
    return true;
}


bool PS_BitMap_Counted::copy(const PS_BitMap_Counted *_other_bitmap) {
    bias = _other_bitmap->bias;
    if (!do_reserve(_other_bitmap->capacity, false)) return false;
    memcpy(count_true_per_index, _other_bitmap->count_true_per_index, (capacity * sizeof(long)));
    for (long i = 0; i < capacity; ++i) {
        if (!data[i]) data[i] = new PS_BitSet_Fast(bias, _other_bitmap->data[i]->getCapacity());
        if (!data[i]->copy(_other_bitmap->data[i])) return false;
    }
    max_row = _other_bitmap->max_row;
    return true;
}


bool PS_BitMap_Counted::reserve(const long _capacity) {
    return do_reserve(_capacity, true);
}
bool PS_BitMap_Counted::do_reserve(const long _capacity, const bool _init_sets) {
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
    new_data   = (PS_BitSet **)malloc(new_data_bytes);
    new_counts = (long *)malloc(new_counts_bytes);
    //
    // test is we got the memory we wanted
    //
    if (!new_data || !new_counts) {
        // failed to allocate all memory so give up the parts we got
        if (new_data)   free(new_data);
        if (new_counts) free(new_counts);
        return false;
    }
    //
    // initialize new data-array
    //
    if (capacity > 0) memcpy(new_data, data, old_data_bytes);    // copy old pointers
    if (data) free(data);                                        // free old memory
    data = new_data;
    if (_init_sets) {
        for (long i = capacity; i < _capacity; ++i) {            // init new requested bitsets
            data[i] = new PS_BitSet_Fast(bias, i+1);             // init field
            if (data[i] == 0) return false;                      // check success
        }
    }
    else {
        for (long i = capacity; i < _capacity; ++i) {
            data[i] = NULL;
        }
    }
    //
    // initialize new counts-arrays
    //
    if (capacity > 0) memcpy(new_counts, count_true_per_index, old_counts_bytes);
    memset(new_counts+old_counts_bytes, 0, new_counts_bytes-old_counts_bytes);
    if (count_true_per_index) free(count_true_per_index);
    count_true_per_index = new_counts;

    capacity = _capacity;                                        // store new capacity
    return true;
}


void PS_BitMap_Counted::print(FILE *out) {
    fprintf(out, "PS_BitMap_Counted : bias(%1i) max_row(%6li) capacity(%6li)\n", bias, max_row, capacity);
    for (long i = 0; i < capacity; ++i) {
        fprintf(out, "[%5li] %6li ", i, count_true_per_index[i]);
        data[i]->print(out, false);
    }
}


void PS_BitMap_Counted::printGNUplot(const char *_title, char *_buffer, PS_FileBuffer *_file) {
    // write title and header of bitmap
    long size;
    size = sprintf(_buffer, "# %s\n#PS_BitMap_Counted : bias(%1i) max_row(%li) capacity(%li)\n#col row - index of a true\n", _title, bias, max_row, capacity);
    _file->put(_buffer, size);

    // write indices of trues per row
    PS_BitSet::IndexSet trues;
    for (long row = 0;
          row < capacity;
          ++row) {
        data[row]->getTrueIndices(trues);
        for (PS_BitSet::IndexSet::iterator col = trues.begin();
              col != trues.end();
              ++col) {
            size = sprintf(_buffer, "%li %li\n", *col, row);
            _file->put(_buffer, size);
        }
    }

    // write dataset separator and header of counters
    size = sprintf(_buffer, "\n\n# speciesID count (of trues)\n");
    _file->put(_buffer, size);

    // write counters per species
    map<long, long> species_per_count;
    for (long row = 0;
          row < capacity;
          ++row) {
        size = sprintf(_buffer, "%li %li\n", row, count_true_per_index[row]);
        _file->put(_buffer, size);
        ++species_per_count[count_true_per_index[row]];
    }

    // write dataset separator and header of counters
    size = sprintf(_buffer, "\n\n# count (of trues) count (of species)\n");
    _file->put(_buffer, size);
    for (map<long, long>::iterator count = species_per_count.begin();
          count != species_per_count.end();
          ++count) {
        size = sprintf(_buffer, "%li %li\n", count->first, count->second);
        _file->put(_buffer, size);
    }

    _file->flush();
}


void PS_BitMap_Counted::recalcCounters() {
    printf("PS_BitMap_Counted::recalcCounters()\n");
    memset(count_true_per_index, 0, capacity * sizeof(long));
    for (long row = 0; row <= max_row; ++row) {
        PS_BitSet *row_data = data[row];
        if (row_data->getMaxUsedIndex() > row) printf("row %4li 0..%li ??\n", row, row_data->getMaxUsedIndex());
        for (long col = 0; col <= row_data->getMaxUsedIndex(); ++col) {
            if (row_data->Get(col)) {
                ++count_true_per_index[col];
                if (row != col) ++count_true_per_index[row];   // don't count diagonal trues twice
            }
        }
    }
}

#else
#error ps_bitmap.hxx included twice
#endif
