#include "UnitArray.h"
#include <iterator>


Basic::UnitArray::UnitArray() :
    array(new BWAPI::Unit [UNITARRAY_INIT_SIZE]),
    internal_size(UNITARRAY_INIT_SIZE),
    _size(0)
{}

void Basic::UnitArray::add(const BWAPI::Unit u) {
    array[_size] = u;
    ++_size;
    if (_size == internal_size) {
        resize();
    }
}

bool Basic::UnitArray::remove(const BWAPI::Unit u) {
    for (int i = 0; i < _size; ++i) {
        if (array[i] == u) {
            array[i] = array[_size - 1];
            --_size;
            return true;
        }
    }
    return false;
}

bool Basic::UnitArray::remove_at(int i) {
    if (i < _size && _size > 0) {
        array[i] = array[_size - 1];
        --_size;
        return true;
    }
    return false;
}

int Basic::UnitArray::size() const {return _size;}

int Basic::UnitArray::find(const BWAPI::Unit u) const {
    for (int i = 0; i < _size; ++i) {
        if (array[i] == u) {
            return i;
        }
    }
    return -1;
}

const BWAPI::Unit Basic::UnitArray::get_by_ID(int ID) const {
    for (int i = 0 ; i < _size; ++i) {
        if (array[i]->getID() == ID) {
            return array[i];
        }
    }
    return nullptr;
}

bool Basic::UnitArray::remove_by_ID(int ID) {
    for (int i = 0 ; i < _size; ++i) {
        if (array[i]->getID() == ID) {
            array[i] = array[_size - 1];
            --_size;
            return true;
        }
    }
    return false;
}

void Basic::UnitArray::clear() {_size = 0;}

const BWAPI::Unit Basic::UnitArray::operator [] (int i) const {return array[i];}


// --------------------------------------------------------------------------------------------
// Private / Internal
// --------------------------------------------------------------------------------------------

void Basic::UnitArray::resize() {
    BWAPI::Unit *temp 
        = new BWAPI::Unit [internal_size + UNITARRAY_RESIZE_CONST];
    memcpy((void *)temp, array, sizeof(BWAPI::Unit) * internal_size);
    delete array; 
    array = temp;
    internal_size += UNITARRAY_RESIZE_CONST;
}
