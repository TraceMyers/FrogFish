#pragma once
#pragma message("including ConstructionPlanning")

#include "../production/BuildOrder.h"
#include "../basic/Bases.h"
#include <stdint.h>

using namespace Production;

namespace Strategy::ConstructionPlanning {

    class ConstructionPlan {

    private:

        BWAPI::TilePosition build_tp;
        BWAPI::Unit builder;
        const BWEM::Base *build_base;
        const Production::BuildOrder::Item *_item_ptr;
        bool extractor_flag = false;

    public:

        void set_base(const BWEM::Base *base) {build_base = base;}

        void set_item(const Production::BuildOrder::Item& item) {_item_ptr = &item;}

        void set_tilepos(const BWAPI::TilePosition& tp) {build_tp = tp;}

        void set_builder(const BWAPI::Unit &u) {builder = u;}

        void set_extractor_transition(bool value) { extractor_flag = value; }
        
        const BWEM::Base *get_base() const {return build_base;}

        const Production::BuildOrder::Item& get_item() const {return *_item_ptr;}

        const BWAPI::TilePosition &get_tilepos() const {return build_tp;}

        const BWAPI::Unit& get_builder() const {return builder;}

        bool extractor_transitioning() const { return extractor_flag; }

        /*
        TODO:

        bool item_removed_from_build_order() {}

        bool builder_died() {}

        */
    };

    enum CONSTRUCTION_PLAN_ERRORS {
        NO_BASE = -1,
        NO_BUILDER = -2,
        NO_LOCATION = -3,
        NO_PLAN = -4
    };

    int                     make_construction_plan(const Production::BuildOrder::Item& item);

    int                     make_expansion_plan();

    const ConstructionPlan& get_plan(int ID);

    int                     find_plan(const Production::BuildOrder::Item &item);

    void                    destroy_plan(int ID);

    void                    set_extractor_flag(int ID, bool value);

    void                    replace_null_builder_with_extractor(int ID, BWAPI::Unit extractor);

    int                     plans_count();
}