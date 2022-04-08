#pragma once
#pragma message("including Construction")

#include "BuildOrder.h"
#include <BWAPI.h>
#include <BWEM/bwem.h>

namespace Production::Construction {
    void init();
    void on_frame_update();
    bool worker_reserved_for_building(const BWAPI::Unit unit);
    bool cancel_build(const Production::BuildOrder::Item &item);
    const std::vector<BWAPI::Unit>& just_completed();
    const std::vector<const BWEM::Base *>& just_completed_from_bases();
}