#include "TestMove.h"
#include "../basic/Units.h"
#include "../movement/Move.h"
#include "../Frogfish.h"
#include "TestMessage.h"
#include "BWEM/bwem.h"

namespace Test::Move {

    namespace {

        // move_unit_to_map_center()
        bool moved_unit_a = false;
        int moved_unit_ID = -1;
        bool move_unit_to_map_center_complete = false;

        bool gathered_group = false;
        int moved_group_ID = -1;
        int destination_index = 0;
        BWAPI::TilePosition oh_the_places [3];
        std::vector<BWAPI::Unit> moving_units;
        bool move_group_around_complete = false;
    }

    void move_unit_to_map_center() {
        if (!moved_unit_a) {
            auto &self_units = Basic::Units::self_units();
            const BWAPI::TilePosition &size = the_map.Size();
            BWAPI::TilePosition map_center (size.x / 2, size.y / 2);

            for (int i = 0; i < self_units.size(); ++i) {
                const BWAPI::Unit& u = self_units[i];
                if (u->getType() == BWAPI::UnitTypes::Zerg_Drone) {
                    moved_unit_ID = Movement::Move::move(u, map_center);
                    moved_unit_a = true;
                    break;
                }
            }
        }
        else if (!move_unit_to_map_center_complete) {
            printf(
                "waypoint: %d, waypoints_count: %d\n", 
                Movement::Move::get_waypoint(moved_unit_ID),
                Movement::Move::get_path_tiles(moved_unit_ID).size()
            );
            bool reached_dest = (Movement::Move::get_status(moved_unit_ID) == Movement::Move::DESTINATION);
            if (reached_dest) {
                printf("Moved unit reached destination!\n");
                bool remove_success = Movement::Move::remove(moved_unit_ID);
                if (remove_success) {
                    printf("SUCCESS: Successfully removed move group!\n");
                }
                else {
                    printf("ERROR: Did not successfully remove move group!\n");
                }

                move_unit_to_map_center_complete = true;
            }
        }
    }

    void move_group_around_the_map() {
        if (!gathered_group) {
            auto &self_units = Basic::Units::self_units();
            const BWAPI::TilePosition &size = the_map.Size();
            oh_the_places[0] = BWAPI::TilePosition(size.x / 2, size.y / 2);
            oh_the_places[1] = BWAPI::TilePosition(10, 10);
            oh_the_places[2] = BWAPI::TilePosition(size.x - 10, size.y - 10); 
            
            // TODO: grabbing units by u_type not working here... why?
            for (int i = 0; i < self_units.size(); ++i) {
                const BWAPI::Unit& u = self_units[i];
                //auto &data = Basic::Units::data(u);
                if (u->getType() == BWAPI::UnitTypes::Zerg_Drone || u->getType() == BWAPI::UnitTypes::Zerg_Hydralisk) {
                    bool already_cached = false;
                    for (auto &cached_unit : moving_units) {
                        if (cached_unit == u) {
                            already_cached = false;
                            break;
                        }
                    }
                    if (!already_cached) {
                        moving_units.push_back(u);
                    }
                }
            }
            if (moving_units.size() > 4) {
                gathered_group = true;
                moved_group_ID = Movement::Move::move(moving_units, oh_the_places[0]);
            }
        }
        else if (!move_group_around_complete) {
            bool reached_dest = (Movement::Move::get_status(moved_group_ID) == Movement::Move::DESTINATION);
            auto& just_destroyed = Basic::Units::self_just_destroyed();
            for (auto unit_it = moving_units.begin(); unit_it < moving_units.end(); ++unit_it) {
                for (int i = 0; i < just_destroyed.size(); ++i) {
                    if (*unit_it == just_destroyed[i]) {
                        unit_it = moving_units.erase(unit_it);
                        break;
                    }
                }
                if (unit_it >= moving_units.end()) {
                    break;
                }
            }
            if (reached_dest) {
                bool remove_success = Movement::Move::remove(moved_group_ID);
                if (remove_success) {
                    DBGMSG("TestMove::move_group_around_the_map(): SUCCESS: Successfully removed move group!");
                    destination_index++;
                    if (destination_index >= 3) {
                        move_group_around_complete = true;
                        return;
                    }
                    moved_group_ID = Movement::Move::move(moving_units, oh_the_places[destination_index]);
                }
                else {
                    DBGMSG("TestMove::move_group_around_the_map(): ERROR: Did not successfully remove move group!");
                    move_group_around_complete = true;
                }
            }
        }
    }
}
