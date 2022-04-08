#include "Tech.h"
#include "Bases.h"
#include "Units.h"
#include "References.h"
#include "../production/BuildOrder.h"
#include <BWAPI.h>

// TODO: implement enemy as well
// TODO: implement for upgrades
// TODO: implement for tech
// TODO: make sim-able

using namespace Basic;
using namespace Basic::Refs;

namespace Basic::Tech {

    namespace {
        std::map<BWAPI::UnitType, bool> _self_can_make;
        std::map<BWAPI::UnitType, bool> _self_can_upgrade;
        std::map<BWAPI::UnitType, bool> _self_can_tech;
    }

    void on_frame_update() {
        std::vector<BWAPI::UnitType> struct_types;
        auto &self_bases = Bases::self_bases(); 
        for (auto &base : self_bases) {
            auto &structures = Bases::structures(base);
            for (auto &structure : structures) {
                bool recorded = false;
                auto &unit_data = Units::data(structure);
                if (unit_data.cmd_ready) {
                    const BWAPI::UnitType &this_type = unit_data.type;
                    for (auto &type : struct_types) {
                        if (this_type == type) {
                            recorded = true;
                            break;
                        }
                    }
                    if (!recorded) {
                        struct_types.push_back(this_type);
                    }
                }
            }
        }

        for (int i = 0; i < Zerg::TYPE_CT; ++i) {
            auto &this_type = Zerg::TYPES[i];
            auto &immediate_requirement = Zerg::UNIT_REQ[i];
            bool requirement_met = false;
            for (auto & structure : struct_types) {
                if (
                    structure == immediate_requirement 
                    || immediate_requirement == BWAPI::UnitTypes::None
                ) {
                    requirement_met = true;
                    break;
                }
            }
            if (requirement_met) {
                if (this_type == BWAPI::UnitTypes::Zerg_Lurker) {
                    if (Broodwar->self()->hasResearched(BWAPI::TechTypes::Lurker_Aspect)) {
                        _self_can_make[this_type] = true;
                    }
                    else {
                        _self_can_make[this_type] = false;
                    }
                }
                else {
                    _self_can_make[this_type] = true;
                }
            }
            else {
                _self_can_make[this_type] = false;
            }
        }
    }

    bool self_can_make(BWAPI::UnitType &ut) {
        return _self_can_make[ut];
    }

    int self_upgrade_level(const BWAPI::UpgradeType &upgrade_type) {
        return Broodwar->self()->getUpgradeLevel(upgrade_type);
    }

    bool self_has_tech(const BWAPI::TechType &tech_type) {
        return Broodwar->self()->hasResearched(tech_type);
    }
}