#pragma once
#pragma message("including Tech")

#include "Bases.h"
#include <BWAPI.h>

namespace Basic::Tech {
    void on_frame_update();
    bool self_can_make(BWAPI::UnitType &ut);
    int  self_upgrade_level(const BWAPI::UpgradeType &upgrade_type);
    bool self_has_tech(const BWAPI::TechType &tech_type);
}