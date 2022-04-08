#include "MakeUnits.h"
#include "Economy.h"
#include "BuildOrder.h"
#include "../basic/Bases.h"
#include "../utility/BWTimer.h"
#include "../test/TestMessage.h"
#include <BWAPI.h>
#include <vector>

namespace Production::MakeUnits {

    namespace {
        // TODO: do some science, fix numbers!
        const int EXTRA_DELAY_FRAMES = 10;
        const int OVERLORD_MAKE_SECONDS = 20;
        const int OVERLORD_COST = 100;
        BWTimer overlord_push_delay;

        // inserts one overlord if needed - only over the next interval where
        // there is no block on making overlords
        void auto_insert_overlords() {
            int supply_block_seconds = Economy::seconds_until_supply_blocked();
            if (0 <= supply_block_seconds && supply_block_seconds < 100) { // < 0 means no block over sim duration
                int supply_block_index = Economy::build_order_index_at_supply_block();
                bool can_insert_overlord = BuildOrder::can_insert_overlords();
                int cur_index = BuildOrder::current_index();
                int insert_index = (can_insert_overlord ? -1 : cur_index);
                int target_time = supply_block_seconds - OVERLORD_MAKE_SECONDS;

                for (
                    unsigned BO_index = cur_index, sim_index = 0; 
                    BO_index < supply_block_index; 
                    ++BO_index, ++sim_index
                ) {
                    const BuildOrder::Item &item = BuildOrder::get(BO_index);
                    if (item.overlord_insert_ban() == BuildOrder::OVERLORD_INSERT_BAN::START) {
                        can_insert_overlord = true;
                    }
                    else if (item.overlord_insert_ban() == BuildOrder::OVERLORD_INSERT_BAN::END) {
                        can_insert_overlord = false;
                    }
                    if (!can_insert_overlord) {
                        insert_index = BO_index;
                    }
                   
                    int time_until_make = item.seconds_until_make();
                    if (time_until_make >= target_time && insert_index > 0) {
                        bool moved_late_overlord_sooner = false;
                        for (int i = insert_index; i < BuildOrder::size(); ++i) {
                            // Changed from item to search_item on 3/24/2021
                            auto &search_item = BuildOrder::get(i);
                            if (search_item.unit_type() == BWAPI::UnitTypes::Zerg_Overlord) {
                                BuildOrder::move(i, insert_index);
                                moved_late_overlord_sooner = true;
                                break;
                            }
                        }
                        if (!moved_late_overlord_sooner) {
                            BuildOrder::insert(
                                BuildOrder::Item::MAKE,
                                BWAPI::UnitTypes::Zerg_Overlord,
                                BWAPI::TechTypes::None,
                                BWAPI::UpgradeTypes::None,
                                -1,
                                insert_index
                            );
                        }

                        float minerals_per_sec = Economy::get_minerals_per_sec();
                        int overlord_delay_seconds = (int)((1/minerals_per_sec) * OVERLORD_COST);
                        for (unsigned i = insert_index + 1; i < BuildOrder::size(); ++i) {
                            int old_prediction = BuildOrder::get(i).seconds_until_make();
                            if (old_prediction == BuildOrder::NO_PREDICTION) {
                                break;
                            }
                            BuildOrder::set_seconds_until_make(i, old_prediction + overlord_delay_seconds);
                        }
                        return;
                    }
                }
                DBGMSG("MakeUnits::auto_insert_overlords(): unsolvable block!");
            }
        }

        // TODO: get larva from map analysis, not by selecting first available
        void spend_down() {
            bool still_spending = true;
            for (auto &base : Basic::Bases::self_bases()) {
                for (auto &larva : Basic::Bases::larva(base)) {
                    if (BuildOrder::finished()) {
                        DBGMSG("MakeUnits::spend_down(): Build order finished");
                        still_spending = false;
                        break;
                    }
                    auto &item = BuildOrder::current_item();
                    if (
                        item.action() == BuildOrder::Item::MAKE
                        && Basic::Tech::self_can_make(item.unit_type())
                        && item.mineral_cost() <= Economy::get_free_minerals()
                        && item.gas_cost() <= Economy::get_free_gas()
                        && Broodwar->self()->supplyUsed() + item.supply_cost() 
                            <= Broodwar->self()->supplyTotal()
                    ) {
                        larva->morph(item.unit_type());
                        Basic::Units::set_utask(larva, Basic::Refs::MAKE);
                        Basic::Units::set_cmd_delay(larva, item.unit_type().buildTime() + EXTRA_DELAY_FRAMES);
                        BuildOrder::next();
                        if (item.unit_type() == BWAPI::UnitTypes::Zerg_Overlord) {
                            overlord_push_delay.start(1, 0);
                        }
                    }
                    else {
                        still_spending = false;
                        break;
                    }
                }
                if (!still_spending) {break;}
            }
        }
    }

    void init() {
        // TODO: do away with this hacky bullshit
        overlord_push_delay.start(10, 0);
    }

    void on_frame_update() {
        if (!BuildOrder::finished()) {
            spend_down();
            overlord_push_delay.on_frame_update();
            if (overlord_push_delay.is_stopped()) {
                auto_insert_overlords();
            }
        }
    }
}