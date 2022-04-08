#include "Construction.h"
#include "BuildOrder.h"
#include "Economy.h"
#include "BuildGraph.h"
#include "../movement/Move.h"
#include "../basic/Bases.h"
#include "../basic/Units.h"
#include "../strategy/ConstructionPlanning.h"
#include "../test/TestMessage.h"
#include <BWEB/BWEB.h>
#include <BWAPI.h>
#include <assert.h>

using namespace Strategy;

namespace Production::Construction {
    namespace {

        std::vector<int>                        construction_plan_IDs;
        std::vector<int>                        move_IDs;
        std::vector<int>                        buildgraph_reservation_IDs;
        std::vector<BWAPI::Unit>                canceled_build_units;
        std::vector<BWAPI::Unit>                just_completed_build_units;
        std::vector<const BWEM::Base *>         just_completed_build_bases;
        const int                               FINISH_BUILD_FRAMES = 30;
        const int                               BUILD_CMD_DELAY = 6;
        const int                               NOT_FOUND = -1;
        BWTimer                                 shit_timer;

        BWTimer                                 cancel_attempt_timer;
        bool                                    started_cancel_attempts = false;

        void init_builds() {
            for (
                unsigned build_index = BuildOrder::current_index(); 
                build_index < BuildOrder::size(); 
                ++build_index
            ) {
                const BuildOrder::Item &build_item = BuildOrder::get(build_index);
                if (ConstructionPlanning::find_plan(build_item) < 0) {
                    int plan_ID;
                    if (build_item.action() == BuildOrder::Item::EXPAND) {
                        plan_ID = ConstructionPlanning::make_expansion_plan();
                    }
                    else if (build_item.action() == BuildOrder::Item::BUILD) {
                        plan_ID = ConstructionPlanning::make_construction_plan(build_item);
                    }
                    else {
                        continue;
                    }
                    if (plan_ID < 0) {
                        DBGMSG(
                            "Construction::init_builds(): build order item ID %d Error %d",
                            build_item.ID(),
                            plan_ID
                        );
                        continue;
                    }
                    const ConstructionPlanning::ConstructionPlan &plan = ConstructionPlanning::get_plan(plan_ID);
                    const BWAPI::TilePosition& tp = plan.get_tilepos();
                    const BWAPI::Unit& builder = plan.get_builder();
                    int move_ID = Movement::Move::move(builder, tp, false, true);
                    if (move_ID < 0) {
                        // TODO: pathing error! need to deal with this somehow
                        DBGMSG("Construction::init_builds(): pathing error to (%d, %d)", tp.x, tp.y);
                        ConstructionPlanning::destroy_plan(plan_ID);
                        continue;
                    }

                    // make reservations
                    const BWAPI::UnitType &type = build_item.unit_type();
                    int buildgraph_res_ID = -1;
                    if (type != BWAPI::UnitTypes::Zerg_Extractor) {
                        buildgraph_res_ID = BuildGraph::make_reservation(
                            plan.get_base(),
                            tp,
                            type.tileWidth(),
                            type.tileHeight()
                        );
                    }
                    else {
                        buildgraph_res_ID = BuildGraph::make_geyser_reservation(tp);
                    }
                    if (buildgraph_res_ID < 0) {
                        DBGMSG("Construction::init_builds(): buildgraph res error to (%d, %d)", tp.x, tp.y);
                        ConstructionPlanning::destroy_plan(plan_ID);
                        Movement::Move::remove(move_ID);
                        continue;
                    }
                    Basic::Units::set_build_status(builder, Basic::Refs::BUILD_STATUS::RESERVED);

                    move_IDs.push_back(move_ID);
                    construction_plan_IDs.push_back(plan_ID);
                    buildgraph_reservation_IDs.push_back(buildgraph_res_ID);
                } 
            }
        }

        // TODO: account for time predictions that lengthen en route
        // TODO: (later on, in another module) use plan/buildgraph info to keep units
        //       from blocking construction
        // TODO: account for deaths
        void advance_builds() {
            DBGMSG("Construction::advance_builds(): plan IDs: %d, plans count: %d", 
                (int)construction_plan_IDs.size(),
                ConstructionPlanning::plans_count()
            );
            auto &cur_build_item = BuildOrder::current_item();
            if (cur_build_item.action() == BuildOrder::Item::CANCEL) {
                if (!started_cancel_attempts) {
                    started_cancel_attempts = true;
                    cancel_attempt_timer.start(20, 0);
                }
                int cancel_build_ID = cur_build_item.cancel_ID();
                const auto &cancel_index = BuildOrder::find_by_ID(cancel_build_ID, false);
                bool cancel_success = false;
                if (cancel_index >= 0) {
                    cancel_success = cancel_build(BuildOrder::get(cancel_index));
                }
                else {
                    DBGMSG(
                        "Construction::advance_builds(): bad cancel index %d, ID %d", 
                        cancel_index,
                        cancel_build_ID
                    );
                }
                if (cancel_attempt_timer.is_stopped() || cancel_success) {
                    // Advance the build order even if cancel doesn't work
                    // TODO: resolve !cancel_success
                    started_cancel_attempts = false;
                    cancel_attempt_timer.start(0, 0);
                    if (!cancel_success) {
                        DBGMSG("Construction::advance_builds(): going to next item without cancel success");
                    }
                    BuildOrder::next();
                }
            }
            auto construction_plan_IDs_it = construction_plan_IDs.begin();
            auto move_IDs_it = move_IDs.begin();
            auto buildgraph_reservation_IDs_it = buildgraph_reservation_IDs.begin();
            
            while (construction_plan_IDs_it < construction_plan_IDs.end()) {
                int plan_ID = *construction_plan_IDs_it;
                auto& plan = ConstructionPlanning::get_plan(plan_ID);
                auto& build_item = plan.get_item();
                // Drones are destroyed to make extractors -- special case
                if (plan.extractor_transitioning()) {
                    auto& units = Basic::Units::self_just_stored();
                    bool replaced_null_builder = false;
                    for (int i = 0; i < units.size(); ++i) {
                        BWAPI::Unit unit = units[i];
                        BWAPI::UnitType type = unit->getType();
                        BWAPI::TilePosition tp = unit->getTilePosition();
                        if (type == BWAPI::UnitTypes::Zerg_Extractor && tp == plan.get_tilepos()) {
                            ConstructionPlanning::replace_null_builder_with_extractor(plan_ID, unit);
                            ConstructionPlanning::set_extractor_flag(plan_ID, false);
                            Basic::Units::set_build_status(unit, BUILD_STATUS::GIVEN_BUILD_CMD);
                            replaced_null_builder = true;
                            break;
                        }
                    }
                    // TODO: flag if it takes too long
                    if (!replaced_null_builder) { 
                        ++buildgraph_reservation_IDs_it;
                        ++construction_plan_IDs_it;
                        ++move_IDs_it;
                        continue; 
                    }
                }
                const BWAPI::Unit& builder = plan.get_builder();
                const Basic::Units::UnitData builder_data = Basic::Units::data(builder);
                auto& build_status = builder_data.build_status;
                if (build_status == BUILD_STATUS::RESERVED) {
                    int build_time = build_item.seconds_until_make();
                    if (build_time != NOT_FOUND) {
                        int move_ID = *move_IDs_it;
                        int travel_time = Movement::Move::remaining_frames(move_ID);
                        int start_build_frames = build_time * 24;
                        if (start_build_frames <= travel_time) {
                            Movement::Move::start(move_ID);
                            Basic::Units::set_utask(builder, UTASK::BUILD);
                            Basic::Units::set_build_status(builder, BUILD_STATUS::MOVING);
                            DBGMSG("Construction::advance_builds(): ID[%d] moving", build_item.ID());
                        }
                    }
                }
                else if (build_status == BUILD_STATUS::MOVING) {
                    int move_ID = *move_IDs_it;
                    // TODO: account for potential move errors
                    if (Movement::Move::get_status(move_ID) == Movement::Move::DESTINATION) {
                        Basic::Units::set_build_status(builder, BUILD_STATUS::AT_SITE);
                        // Could result in errors where bad ID is used, since the 
                        // bad move ID is kept for vector concurrency; might move this to the end
                        Movement::Move::remove(move_ID);
                        DBGMSG("Construction::advance_builds(): ID[%d] at site", build_item.ID());
                    }
                }
                else if (build_status == BUILD_STATUS::AT_SITE) {
                    if (
                        BuildOrder::current_item() == build_item
                        && build_item.mineral_cost() <= Economy::get_free_minerals()
                        && build_item.gas_cost() <= Economy::get_free_gas()
                    ) {
                        const BWAPI::UnitType &build_type = build_item.unit_type();
                        const BWAPI::TilePosition &build_loc = plan.get_tilepos();
                        builder->build(build_type, build_loc);
                        Basic::Units::set_build_status(builder, BUILD_STATUS::GIVEN_BUILD_CMD);
                        Basic::Units::set_cmd_delay(builder, BUILD_CMD_DELAY);
                        // Starts the process of checking whether the drone has yet been destroyed.
                        // Only a concern with making extractors
                        if (build_type == BWAPI::UnitTypes::Zerg_Extractor) {
                            ConstructionPlanning::set_extractor_flag(plan_ID, true);
                        }
                        DBGMSG("Construction::advance_builds(): ID[%d] given build cmd", build_item.ID());
                    }
                }
                else if(build_status == BUILD_STATUS::GIVEN_BUILD_CMD) {
                    const BWAPI::UnitType unit_type = builder->getType();
                    auto &unit_data = Basic::Units::data(builder);
                    if (unit_type.isBuilding()) {
                        Production::BuildGraph::end_reservation(*buildgraph_reservation_IDs_it);
                        Basic::Units::set_build_status(builder, BUILD_STATUS::BUILDING);
                        Basic::Units::set_cmd_delay(builder, unit_type.buildTime() + FINISH_BUILD_FRAMES);
                        DBGMSG("Construction::advance_builds(): ID[%d] building", build_item.ID());
                        BuildOrder::next();
                    }
                    else if (unit_data.cmd_ready) {
                        const BWAPI::UnitType &build_type = build_item.unit_type();
                        const BWAPI::TilePosition &build_loc = plan.get_tilepos();
                        builder->build(build_type, build_loc);
                        Basic::Units::set_cmd_delay(builder, BUILD_CMD_DELAY);
                        DBGMSG("Construction::advance_builds(): ID[%d] given build cmd again", build_item.ID());
                    }
                    // TODO: deal with not advancing to 'BUILDING' status after some time
                }
                else if (build_status == BUILD_STATUS::BUILDING) {
                    auto &unit_data = Basic::Units::data(builder);
                    if (unit_data.cmd_ready) {
                        Basic::Units::set_build_status(builder, BUILD_STATUS::COMPLETED);
                        DBGMSG("Construction::advance_builds(): ID[%d] completed building", build_item.ID());
                    }
                }
                else if (build_status == BUILD_STATUS::COMPLETED) {
                    // TODO: builds just compeleted registry
                    just_completed_build_units.push_back(builder);
                    just_completed_build_bases.push_back(plan.get_base());
                    ConstructionPlanning::destroy_plan(plan_ID);
                    buildgraph_reservation_IDs_it = buildgraph_reservation_IDs.erase(buildgraph_reservation_IDs_it);
                    construction_plan_IDs_it = construction_plan_IDs.erase(construction_plan_IDs_it);
                    move_IDs_it = move_IDs.erase(move_IDs_it);
                    if (construction_plan_IDs_it == construction_plan_IDs.end()) {
                        break;
                    }
                }
                else {
                    DBGMSG(
                        "Construction::advance_builds(): ID[%d] bad build_status: %d", 
                        build_item.ID(), 
                        (int)build_status
                    );
                }
                ++buildgraph_reservation_IDs_it;
                ++construction_plan_IDs_it;
                ++move_IDs_it;
            }
        }
    }

    // TODO: figure out a way to not have to wait for a second
    void init() {
        shit_timer.start(1, 0);
    }

    void on_frame_update() {
        just_completed_build_units.clear();
        just_completed_build_bases.clear();
        shit_timer.on_frame_update();
        if (shit_timer.is_stopped() && !BuildOrder::finished()) {
            init_builds();
        }
        advance_builds();
        // TODO: babysit canceled build units until they've definitely stopped building
    }

    bool cancel_build(const Production::BuildOrder::Item &item) {
        int plan_ID = ConstructionPlanning::find_plan(item);
        int move_ID;
        int buildgraph_ID;
        if (plan_ID >= 0) {
            auto &plan = ConstructionPlanning::get_plan(plan_ID);
            if (plan.extractor_transitioning()) {
                return false;
            }
            auto construction_plan_IDs_it = construction_plan_IDs.begin();
            auto move_IDs_it = move_IDs.begin();
            auto buildgraph_reservation_IDs_it = buildgraph_reservation_IDs.begin();
            bool removed = false;
            while (construction_plan_IDs_it < construction_plan_IDs.end()) {
                if (*construction_plan_IDs_it == plan_ID) {
                    move_ID = *move_IDs_it;
                    buildgraph_ID = *buildgraph_reservation_IDs_it;
                    buildgraph_reservation_IDs.erase(buildgraph_reservation_IDs_it);
                    construction_plan_IDs.erase(construction_plan_IDs_it);
                    move_IDs.erase(move_IDs_it);
                    removed = true;
                    break;
                }
                ++buildgraph_reservation_IDs_it;
                ++construction_plan_IDs_it;
                ++move_IDs_it;
            }
            if (removed) {
                auto& builder = plan.get_builder();
                auto& builder_data = Basic::Units::data(builder);
                if (builder_data.build_status == BUILD_STATUS::BUILDING) {
                    // important for extractors. can't wait until it's done automatically next frame
                    // or get read access violation
                    builder->cancelConstruction();
                    Basic::Bases::remove_struct_from_owner_base(builder);
                }
                else if (builder_data.build_status == BUILD_STATUS::GIVEN_BUILD_CMD) {
                    builder->stop();
                }
                Basic::Units::set_utask(builder, Basic::Refs::IDLE);
                Basic::Units::set_build_status(builder, Basic::Refs::BUILD_STATUS::NONE);
                canceled_build_units.push_back(builder);

                Movement::Move::remove(move_ID);
                ConstructionPlanning::destroy_plan(plan_ID);
                if (Production::BuildGraph::reservation_exists(buildgraph_ID)) {
                    Production::BuildGraph::end_reservation(buildgraph_ID);
                }

                return true;
            }
            else {
                DBGMSG("Construction::cancel_build() unsuccessful removal");
            }
        }
        else {
            DBGMSG("Construction::cancel_build() can't find item.");
        }
        return false;
    }

    const std::vector<BWAPI::Unit>& just_completed() {
        return just_completed_build_units;
    }

    const std::vector<const BWEM::Base *>& just_completed_from_bases() {
        return just_completed_build_bases;
    }
}
