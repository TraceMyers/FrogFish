#pragma once
#pragma message("including UnitData")

#include "../utility/BWTimer.h"
#include "References.h"
#include <BWAPI.h>

using namespace Basic::Refs;

// TODO: move back to wrapper class?
// TODO: use bit field for statuses

namespace Basic::Units {

    UTYPE get_utype(BWAPI::Unit u);

    struct UnitData {
        UnitData(BWAPI::Unit u);    
        // self only
        bool                cmd_ready;
        BWTimer             cmd_timer;
        UTASK               u_task;
        BUILD_STATUS        build_status;
        int                 irradiate_timer;
        int                 lockdown_timer;
        int                 plague_timer;
        int                 stasis_timer;
        int                 ground_weapon_cd;
        int                 air_weapon_cd;
        int                 spell_cd;
        BWAPI::Unit         transport;
        BWAPI::Unitset      loaded_units;
        BWAPI::TechType     teching;
        BWAPI::UpgradeType  upgrading;
        // both
        int                 ID;
        int                 hp;
        int                 max_hp;
        int                 shields;
        int                 energy;
        double              velocity[2] {0.0, 0.0};
        double              angle;
        bool                lifted;
        BWAPI::UnitType     type;
        BWAPI::UnitType     init_type;
        BWAPI::Position     pos;
        BWAPI::TilePosition tilepos;
        UTYPE               u_type;
        int                 group_ID;
        // enemy
        bool                missing;
    };
}