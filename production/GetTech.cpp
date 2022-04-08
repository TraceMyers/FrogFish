#include "GetTech.h"
#include "../basic/Bases.h"
#include "../basic/Units.h"
#include "BuildOrder.h"
#include "Economy.h"
#include <BWAPI.h>
#include <vector>

namespace Production::GetTech {

    namespace {

        const int CMD_DELAY = 2;

        void init_tech() {
            // TODO: doesn't check if end
            auto &build_item = BuildOrder::current_item();
            if (build_item.action() == BuildOrder::Item::TECH) {
                const BWAPI::TechType &tech_type = build_item.tech_type();
                int mineral_cost = tech_type.mineralPrice();
                int gas_cost = tech_type.gasPrice();
                if (
                    mineral_cost <= Economy::get_free_minerals()
                    && gas_cost <= Economy::get_free_gas()
                ) {
                    BWAPI::UnitType research_struct = tech_type.whatResearches();
                    for (auto &base : Basic::Bases::self_bases()) {
                        for (auto &structure : Basic::Bases::structures(base)) {
                            if (
                                structure->getType() == research_struct
                                && !structure->isResearching()
                                && !structure->isUpgrading()
                                && Basic::Units::data(structure).cmd_ready
                            ) {
                                structure->research(tech_type);
                                Basic::Units::set_cmd_delay(structure, CMD_DELAY);
                                goto ADVANCE_BUILD_ORDER; 
                            }
                        }
                    }
                }
            }
            else if (build_item.action() == BuildOrder::Item::UPGRADE) {
                const BWAPI::UpgradeType &upgrade_type = build_item.upgrade_type();
                int mineral_cost = upgrade_type.mineralPrice();
                int gas_cost = upgrade_type.gasPrice();
                if (
                    mineral_cost <= Economy::get_free_minerals()
                    && gas_cost <= Economy::get_free_gas()
                ) {
                    BWAPI::UnitType research_struct = upgrade_type.whatUpgrades();
                    for (auto &base : Basic::Bases::self_bases()) {
                        for (auto &structure : Basic::Bases::structures(base)) {
                            if (
                                structure->getType() == research_struct
                                && !structure->isResearching()
                                && !structure->isUpgrading()
                                && Basic::Units::data(structure).cmd_ready
                            ) {
                                structure->upgrade(upgrade_type);
                                Basic::Units::set_cmd_delay(structure, CMD_DELAY);
                                goto ADVANCE_BUILD_ORDER;
                            }
                        }
                    }
                }
            }
            if (false) {
                ADVANCE_BUILD_ORDER: BuildOrder::next();
            }
        }
    }

    void on_frame_update() {
        init_tech();
    }
}