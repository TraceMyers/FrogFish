#pragma once
#pragma message("including UnitMaker")

#include "Economy.h"
#include "BuildOrder.h"
#include "../basic/Tech.h"

namespace Production::MakeUnits {
    void init();
    void on_frame_update();
}