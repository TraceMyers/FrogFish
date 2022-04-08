#include "ConstructionPlanning.h"
#include "../production/BuildGraph.h"
#include "../basic/Bases.h"
#include "../basic/Units.h"
#include "../basic/UnitData.h"
#include "../test/TestMessage.h"
#include "BWEM/bwem.h"

using namespace Production;

namespace Strategy::ConstructionPlanning {

    namespace {

        ConstructionPlan plans[64];
        uint64_t used_plans_field = 0;
        int _plans_count = 0;

        int get_free_plans_index() {
            uint64_t bit_checker = 1;
            int i = 0;
            while(bit_checker > 0) {
                if (!(used_plans_field & bit_checker)) {
                    used_plans_field ^= bit_checker;
                    break;
                }
                bit_checker <<= 1;
                i += 1;
            }
            return i;
        }
    }

    // TODO: use logic to make decisions
    int make_construction_plan(const Production::BuildOrder::Item& item) {
        const std::vector<const BWEM::Base *> &bases = Basic::Bases::self_bases();
        const BWEM::Base *construction_base = nullptr;
        BWAPI::Unit builder = nullptr;
        const BWAPI::UnitType& type = item.unit_type();
        BWAPI::TilePosition build_tp;
        for (auto &base : bases) {
            auto &base_workers = Basic::Bases::workers(base);
            if (base_workers.size() > 0) {
                construction_base = base;
                for (auto &worker : base_workers) {
                    auto& worker_data = Basic::Units::data(worker);
                    if (worker_data.u_task == Basic::Refs::MINERALS && worker_data.build_status == NONE) {
                        builder = worker;
                        break;
                    }
                }
            }
            if (construction_base != nullptr && builder != nullptr) {
                break;
            }
        }
        if (construction_base == nullptr) {
            DBGMSG("ConstructionPlanning::make_construction_plan(): Couldn't find base for construction! Building type: %s", type.c_str());
            return NO_BASE;
        }
        else if (builder == nullptr) {
            DBGMSG("ConstructionPlanning::make_construction_plan(): Couldn't find builder for construction! Building type: %s", type.c_str());
            return NO_BUILDER;
        }
        if (type == BWAPI::UnitTypes::Zerg_Extractor) {
            build_tp = Production::BuildGraph::get_geyser_tilepos(construction_base);
        }
        else {
            build_tp = Production::BuildGraph::get_build_tilepos(
                construction_base, 
                type.tileWidth(), 
                type.tileHeight()
            );
        }
        if (build_tp == BWAPI::TilePositions::None) {
            DBGMSG("ConstructionPlanning::make_construction_plan(): Couldn't find builder for construction! Building type: %s", type.c_str());
            return NO_LOCATION;
        }

        int plan_ID = get_free_plans_index();
        plans[plan_ID].set_base(construction_base);
        plans[plan_ID].set_builder(builder);
        plans[plan_ID].set_item(item);
        plans[plan_ID].set_tilepos(build_tp);

        ++_plans_count;
        return plan_ID;
    }

    int make_expansion_plan() {
        // rather than per-map, do generalized model so it can stay up without maintenance
        // So, what features of a map are potentially deciding factors? (maybe run regression here
        // to decide on the features)
        // For each expansion from 2nd to 4th:
        // - proximity by ground to main
        // - proximity by ground to another base
        // - proximity by air to main
        // - proximity by air to average base position
        // - proximity by air to average enemy base position
        // - enemy race
        // - enemy air attack happens within the next two minutes (on any friendly base)
        // - enemy drop happens within the next two minutes (on any friendly base)
        // - current average distance of enemy units to exp base
        // - shares a choke with center of the map
        // - shares a choke with another base
        // 
        // - create unsupervized learning set of match army composition classes
        //      - for example, ZvP, (50% zergling, etc.) vs (30% zealot, etc.) may end up being one class
        //      - use these for later deciding on army composition
        //      - reduce classes to smaller number for use here (maybe 5 per matchup)
        //      - each of these is tested for interaction with the above features, as well as each 
        //          feature on its own
        return 0;
    }

    const ConstructionPlan &get_plan(int ID) {
        return plans[ID];
    }

    int find_plan(const Production::BuildOrder::Item &item) {
        uint64_t field_checker = 1;
        int i = 0;
        while(field_checker > 0) {
            if (field_checker & used_plans_field) {
                if(plans[i].get_item() == item) {
                    return i; 
                }
            }
            field_checker <<= 1;
            ++i;
        }
        return (int)NO_PLAN;
    }

    void destroy_plan(int ID) {
        uint64_t ID_field_num = 1 << ID;

        --_plans_count;
        used_plans_field ^= ID_field_num;
    }

    void set_extractor_flag(int ID, bool value) {
        plans[ID].set_extractor_transition(value);
    }

    void replace_null_builder_with_extractor(int ID, BWAPI::Unit extractor) {
        plans[ID].set_builder(extractor);
    }

    int plans_count() {
        return _plans_count;
    }
}