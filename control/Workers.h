#pragma once
#pragma message("including Workers")


namespace Control::Workers {
    void send_idle_workers_to_mine();
    void send_mineral_workers_to_gas();
}