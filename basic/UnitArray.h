#pragma once
#pragma message("including UnitArray")

#include <BWAPI.h>

// TODO: Take off the heap, put all UnitArrays together into one big buffer or just have several separate ones

namespace Basic {

    class UnitArray {

        /* PUBLIC */
        public:

        UnitArray();
        void              add(const BWAPI::Unit u);
        bool              remove(const BWAPI::Unit u);
        bool              remove_at(int i);
        int               size() const;
        int               find(const BWAPI::Unit u) const;
        const BWAPI::Unit get_by_ID(int ID) const;
        bool              remove_by_ID(int ID);
        void              clear();
        const BWAPI::Unit operator [] (int i) const;

        /* PRIVATE */
        private:
        
        static const int UNITARRAY_INIT_SIZE = 100;
        static const int UNITARRAY_RESIZE_CONST = 100;
        BWAPI::Unit *    array;
        int              internal_size;
        int              _size;

        void resize();
    };

}