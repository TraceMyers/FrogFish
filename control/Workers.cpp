#include "Workers.h"
#include "../basic/Bases.h"
#include "../basic/Units.h"
#include "../basic/References.h"
#include "../production/Construction.h"
#include <BWEM/bwem.h>
#include <BWAPI.h>

namespace Control::Workers {

void send_idle_workers_to_mine() {
    const std::vector<const BWEM::Base *>& bases = Basic::Bases::self_bases();
    for (auto base : bases) {
        const std::vector<BWAPI::Unit> &workers = Basic::Bases::workers(base);
        int counter = 0;
        for (auto worker : workers) {
            auto &unit_data = Basic::Units::data(worker);
            if (unit_data.u_task == Basic::Refs::IDLE && unit_data.cmd_ready) {
                worker->gather(worker->getClosestUnit(Filter::IsMineralField));
                Basic::Units::set_utask(worker, Basic::Refs::MINERALS);
                Basic::Units::set_cmd_delay(worker, 2);
            }
        }
    }
}

// TODO: needs management over time and smarter init (maybe less than 3 available?)
void send_mineral_workers_to_gas() {
    auto &just_completed_buildings = Production::Construction::just_completed();
    auto &just_completed_bases = Production::Construction::just_completed_from_bases();
    auto jcbuild_it = just_completed_buildings.begin();
    auto jcbase_it  = just_completed_bases.begin();
    for (
        ;
        jcbuild_it < just_completed_buildings.end();
        ++jcbuild_it, ++jcbase_it
    ) {
        auto building  = *jcbuild_it;
        auto base      = *jcbase_it;
        if (building->getType() == BWAPI::UnitTypes::Zerg_Extractor) {
            int workers_sent_to_gas = 0;
            auto &workers = Basic::Bases::workers(base);
            for (auto worker : workers) {
                const auto& data = Basic::Units::data(worker);
                if (data.build_status == Basic::Refs::NONE && data.u_task == Basic::Refs::MINERALS) {
                    worker->gather(building);
                    Basic::Units::set_utask(worker, Basic::Refs::GAS);
                    Basic::Units::set_cmd_delay(worker, 2);
                    ++workers_sent_to_gas;
                }
                if (workers_sent_to_gas == 3) {
                    break;
                }
            }
        }
    }
}

}