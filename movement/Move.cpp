#include "Move.h"
#include "../basic/UnitData.h"
#include "../basic/Units.h"
#include "../utility/FrogMath.h"
#include "../FrogFish.h"
#include "../test/TestMessage.h"
#include <BWAPI.h>
#include <BWEB/BWEB.h>
#include <math.h>


// TODO: air unit move/attack move
// TODO: move_group() units decelerating at waypoints

#define REMOVE_IF_DESTROYED(_unit, _unit_it, _group, _units_just_destroyed) {\
    if (_units_just_destroyed.find(_unit) >= 0) {\
        _unit_it = _group.erase(_unit_it);\
        if (_unit_it == _group.end()) {break;}\
        else                        {continue;}\
    }\
}

namespace Movement::Move {

    namespace {

        std::vector<std::vector<BWAPI::Unit>>   groups;
        std::vector<double>                     cohesion_radii;
        std::vector<STATUS>                     statuses;
        std::vector<BWEB::Path>                 paths;
        std::vector<int>                        waypoints;
        std::vector<double>                     distance_remaining;
        std::vector<BWAPI::Position>            previous_positions;
        std::vector<double>                     average_speeds;
        std::vector<BWTimer>                    cohesion_wait_timers;
        std::vector<BWTimer>                    waypoint_reachable_timers;
        std::vector<int>                        unused_group_IDs;
        BWEB::Path                              check_waypoint_path;

        const int MOVE_CMD_DELAY_FRAMES = 4,
                  COHESION_WAIT_MAX_FRAMES = 48, // wait frames before timeout = this - 1
                  WAYPOINT_REACHABLE_CHECK_INTERVAL = 5;

        
        bool surrounding_tilepositions_walkable(const BWAPI::TilePosition &search_tp) {
            // TODO: remove ridiculous number of redundant checks between this and nearest_valid_tilepos()
            BWAPI::TilePosition nearby_tiles[8] = {
                BWAPI::TilePosition(search_tp.x + 1, search_tp.y),
                BWAPI::TilePosition(search_tp.x - 1, search_tp.y),
                BWAPI::TilePosition(search_tp.x, search_tp.y + 1),
                BWAPI::TilePosition(search_tp.x, search_tp.y - 1),
                BWAPI::TilePosition(search_tp.x + 1, search_tp.y + 1),
                BWAPI::TilePosition(search_tp.x + 1, search_tp.y - 1),
                BWAPI::TilePosition(search_tp.x - 1, search_tp.y + 1),
                BWAPI::TilePosition(search_tp.x - 1, search_tp.y - 1)
            };
            bool walkable_surrounding_tiles = true;
            BWAPI::TilePosition nearby_tile;
            for (int j = 0; j < 8; ++j) {
                nearby_tile = nearby_tiles[j];
                if (
                    !nearby_tile.isValid() 
                    || !BWEB::Map::isWalkable(nearby_tile)
                ) {
                    walkable_surrounding_tiles = false;
                    break;
                }
            }
            return walkable_surrounding_tiles;
        }
        

        BWAPI::TilePosition nearest_valid_tilepos(BWAPI::TilePosition dest) {
            // TODO: doesn't allow for moving units into a choke (surrounding_tileposiitions_walkable)
            const int MAX_SEARCH_STEPS = 50;
            if (dest.isValid()) {
                if (
                    BWEB::Map::isWalkable(dest)
                    && surrounding_tilepositions_walkable(dest)
                ) {
                    return dest;
                }

                DIRECTIONS dir = UP;
                int inner_steps_bound = 1;
                int inner_steps = 0;
                int outer_steps_bound = 2;
                int outer_steps = 0;
                BWAPI::TilePosition search_tp = dest;
                for (int i = 0; i < MAX_SEARCH_STEPS; ++i) {
                    switch(dir) {
                        case UP:
                            --search_tp.y;
                            break;
                        case RIGHT:
                            ++search_tp.x;
                            break;
                        case DOWN:
                            ++search_tp.y;
                            break;
                        case LEFT:
                            --search_tp.x;
                    }
                    if (
                        search_tp.isValid() 
                        && Broodwar->isWalkable(BWAPI::WalkPosition(search_tp))
                        && surrounding_tilepositions_walkable(search_tp)
                    ) {
                        break;
                    }
                    ++inner_steps;
                    if (inner_steps == inner_steps_bound) {
                        inner_steps = 0;
                        ++outer_steps;
                        if (outer_steps == outer_steps_bound) {
                            ++inner_steps_bound;
                            outer_steps = 0;
                            switch(dir) {
                                case UP:
                                    dir = RIGHT;
                                    break;
                                case RIGHT:
                                    dir = DOWN;
                                    break;
                                case DOWN:
                                    dir = LEFT;
                                    break;
                                case LEFT:
                                    dir = UP;
                            }
                        }
                    }
                }
                return search_tp;
            }
            else {return BWAPI::TilePositions::None;}
        }

        BWAPI::Position nearest_valid_pos(BWAPI::Position dest) {
            return BWAPI::Position(nearest_valid_tilepos(BWAPI::TilePosition(dest)));
        }

        double calculate_cohesion_radius(std::vector<BWAPI::Unit> &units, double cohesion_factor) {
            int group_width = 0, group_height = 0;
            for (auto &unit : units) {
                group_width += unit->getRight() - unit->getLeft();
                group_height += unit->getBottom() - unit->getTop();
            }
            int n = units.size();
            double avg_width = (double)group_width / n,
                  avg_height = (double)group_height / n,
                  radius = 2.0 * sqrt((n * avg_height * avg_width) / Utility::FrogMath::PI);
            return radius * cohesion_factor;
        }

        inline void move_unit(BWAPI::Unit unit, const BWAPI::Position &target_pos, const STATUS &status) {
            if (status == ATTACK_MOVING) {unit->patrol(target_pos);}
            else                         {unit->move(target_pos);}
            Basic::Units::set_cmd_delay(unit, MOVE_CMD_DELAY_FRAMES);
        }

        // Only for attack-moving units that never engage along the way. Since attack-moving units 
        // are moved with patrol, to avoid a patrol at the very end, they are given an attack_move
        // or move command at the end.
        void end_attack_move(std::vector<BWAPI::Unit> group, const BWAPI::Position &target_pos) {
            for (auto &unit : group) {
                BWAPI::UnitType &type = unit->getType();
                if (
                    type == BWAPI::UnitTypes::Zerg_Lurker
                    || type == BWAPI::UnitTypes::Zerg_Defiler
                    || type == BWAPI::UnitTypes::Zerg_Overlord
                    || type == BWAPI::UnitTypes::Zerg_Queen
                ) {
                    unit->move(target_pos);
                }
                else {unit->attack(target_pos);}
            }
        }

        // TODO: fairly major refactor; warts obvious (IP)
        // TODO: find a way to periodically verify pathing; might just clear and remake?
        void move_group(
            std::vector<BWAPI::Unit> &group,
            BWEB::Path &path,
            const double &radius,
            STATUS &status,
            int &waypoint,
            double &dist_remain,
            BWAPI::Position &prev_pos,
            BWTimer &wait_timer,
            BWTimer &waypoint_check_timer
        ) {
            std::vector<BWAPI::TilePosition> &path_tiles = path.getTiles();
            BWAPI::TilePosition target_tilepos = path_tiles[waypoint]; 
            BWAPI::Position target_pos (target_tilepos.x * 32, target_tilepos.y * 32);
            BWAPI::Position avg_pos = Utility::FrogMath::average_position(group);
            double dist_from_waypoint = path.getDistanceFrom(waypoint);
            int dist_to_target_pos = avg_pos.getApproxDistance(target_pos);
            dist_remain = dist_from_waypoint + dist_to_target_pos;
            prev_pos = avg_pos;
            const Basic::UnitArray &units_just_destroyed = Basic::Units::self_just_destroyed();

            if (dist_to_target_pos <= radius) {
                bool wait_for_cohesion = false;
                for (auto unit_it = group.begin(); unit_it < group.end(); ++unit_it) {
                    auto &unit = *unit_it;
                    REMOVE_IF_DESTROYED(unit, unit_it, group, units_just_destroyed);
                    double distance_outside_radius = 
                        avg_pos.getApproxDistance(unit->getPosition()) - radius;
                    if (distance_outside_radius > 0) {
                        wait_for_cohesion = true;
                    }
                    auto &unit_data = Basic::Units::data(unit);
                    if (unit_data.cmd_ready) {move_unit(unit, target_pos, status);}
                }
                if (!wait_for_cohesion) {
                    wait_timer.start(0, 0);
                    ++waypoint;
                    if (waypoint == path_tiles.size() - 1) {
                        if (status == ATTACK_MOVING) {end_attack_move(group, target_pos);}
                        status = DESTINATION;
                    }
                }
                else {
                    DBGMSG("Move::move_group(): Waiting for cohesion");
                    int wait_frames_left = wait_timer.get_frames_left();
                    if (wait_frames_left == 1)      {status = COHESION_WAIT_TIMEOUT;}
                    else if (wait_frames_left == 0) {wait_timer.start(0, COHESION_WAIT_MAX_FRAMES);}
                }
            }
            else for (auto unit_it = group.begin(); unit_it < group.end(); ++unit_it) {
                auto &unit = *unit_it;
                REMOVE_IF_DESTROYED(unit, unit_it, group, units_just_destroyed);
                auto &unit_data = Basic::Units::data(unit);
                if (unit_data.cmd_ready) {move_unit(unit, target_pos, status);}
            }
            if (group.size() == 0) {status = KILLED;}
        }
    }

    int move(
        std::vector<BWAPI::Unit> &units, 
        BWAPI::TilePosition dest,
        bool attack,
        bool wait,
        double cohesion_factor
    ) {
        dest = nearest_valid_tilepos(dest);
        if (dest == BWAPI::TilePositions::None) {
            return INVALID_DEST;
        } 
        BWAPI::TilePosition src = Utility::FrogMath::average_tileposition(units); 
        src = nearest_valid_tilepos(src);
        if (src == BWAPI::TilePositions::None) {
            return INVALID_SRC;
        }
        
        int group_ID;
        int unused_group_IDs_size = unused_group_IDs.size();
        if (unused_group_IDs_size > 0) {
            group_ID = unused_group_IDs[unused_group_IDs_size - 1];
            paths[group_ID].clear();
            paths[group_ID].createUnitPath(BWAPI::Position(src), BWAPI::Position(dest));
            if (!paths[group_ID].isReachable()) {return UNREACHABLE_DEST;}
            if (attack) {
                if (wait) {statuses[group_ID] = WAITING_ATTACK;}
                else      {statuses[group_ID] = ATTACK_MOVING;}
            }
            else {
                if (wait) {statuses[group_ID] = WAITING_MOVE;}
                else      {statuses[group_ID] = MOVING;}
            }
            groups[group_ID] = units;
            waypoints[group_ID] = 0;
            distance_remaining[group_ID] = paths[group_ID].getDistance();
            previous_positions[group_ID] = BWAPI::Position(src);
            average_speeds[group_ID] = Utility::FrogMath::average_speed(units);
            cohesion_radii[group_ID] = calculate_cohesion_radius(units, cohesion_factor);
            cohesion_wait_timers[group_ID].start(0, 0);
            waypoint_reachable_timers[group_ID].start(0, WAYPOINT_REACHABLE_CHECK_INTERVAL);
            unused_group_IDs.erase(unused_group_IDs.end() - 1);
        }
        else {
            BWEB::Path p = BWEB::Path();
            p.createUnitPath(BWAPI::Position(src), BWAPI::Position(dest));
            if (!p.isReachable()) {return UNREACHABLE_DEST;}
            if (attack) {
                if (wait) {statuses.push_back(WAITING_ATTACK);}
                else      {statuses.push_back(ATTACK_MOVING);}
            }
            else {
                if (wait) {statuses.push_back(WAITING_MOVE);}
                else      {statuses.push_back(MOVING);}
            }
            group_ID = groups.size();
            paths.push_back(p);
            groups.push_back(units);
            waypoints.push_back(0);
            distance_remaining.push_back(p.getDistance());
            previous_positions.push_back(BWAPI::Position(src));
            average_speeds.push_back(Utility::FrogMath::average_speed(units));
            cohesion_radii.push_back(calculate_cohesion_radius(units, cohesion_factor));
            cohesion_wait_timers.push_back(BWTimer());
            BWTimer waypoint_timer = BWTimer();
            waypoint_timer.start(0, WAYPOINT_REACHABLE_CHECK_INTERVAL);
            waypoint_reachable_timers.push_back(waypoint_timer);
        }
        return group_ID;
    }

    int move(const BWAPI::Unit u, BWAPI::TilePosition dest, bool attack, bool wait) {
        return move(std::vector<BWAPI::Unit> {u}, dest, attack, wait, C_MED);
    }

    STATUS get_status(int group_ID) {
        return (0 <= group_ID && group_ID < groups.size() ? statuses[group_ID] : BAD_GROUP_ID);
    }

    void start(int group_ID) {
        if (0 <= group_ID && group_ID < groups.size()) {
            if      (statuses[group_ID] == WAITING_ATTACK)  {statuses[group_ID] = ATTACK_MOVING;}
            else if (statuses[group_ID] == WAITING_MOVE)    {statuses[group_ID] = MOVING;}
        }
    }

    int remaining_distance(int group_ID) {
        return distance_remaining[group_ID];
    }

    int remaining_frames(int group_ID) {
        return (int)round(distance_remaining[group_ID] / average_speeds[group_ID]);
    }

    bool remove(int group_ID) {
        if (group_ID < groups.size() && statuses[group_ID] != UNUSED_GROUP) {
            groups[group_ID].clear();
            statuses[group_ID] = UNUSED_GROUP;
            unused_group_IDs.push_back(group_ID);
            return true;
        }
        return false;
    }

    void on_frame_update() {
        auto group_it = groups.begin();
        auto cohesion_radii_it = cohesion_radii.begin();
        auto status_it = statuses.begin();
        auto path_it = paths.begin();
        auto waypoint_it = waypoints.begin();
        auto distance_remaining_it = distance_remaining.begin();
        auto previous_pos_it = previous_positions.begin();
        auto cohesion_wait_timer_it = cohesion_wait_timers.begin();
        auto waypoint_reachable_timer_it = waypoint_reachable_timers.begin();

        while (group_it != groups.end()) {
            STATUS &status = *status_it;
            if (status == MOVING || status == ATTACK_MOVING) {
                auto &group = *group_it;
                auto &path = *path_it;
                auto &radius = *cohesion_radii_it;
                auto &waypoint = *waypoint_it;
                auto &dist_remain = *distance_remaining_it;
                auto &prev_pos = *previous_pos_it;
                auto &wait_timer = *cohesion_wait_timer_it;
                auto &waypoint_check_timer = *waypoint_reachable_timer_it;
                move_group(
                    group, 
                    path, 
                    radius, 
                    status, 
                    waypoint, 
                    dist_remain, 
                    prev_pos,
                    wait_timer, 
                    waypoint_check_timer
                );
                wait_timer.on_frame_update();
                waypoint_check_timer.on_frame_update();
            }
            ++group_it;
            ++cohesion_radii_it;
            ++status_it;
            ++path_it;
            ++waypoint_it;
            ++distance_remaining_it;
            ++previous_pos_it;
            ++cohesion_wait_timer_it;
            ++waypoint_reachable_timer_it;
        }
    }

    const std::vector<BWAPI::Unit> &get_group(int ID) {
        return groups[ID];
    }

    const std::vector<BWAPI::TilePosition> get_path_tiles(int ID) { 
        return paths[ID].getTiles(); 
    }

    int get_waypoint(int ID) {
        return waypoints[ID];
    }

    BWAPI::Position get_avg_position(int ID) {
        return previous_positions[ID];
    }

    int get_cohesion_radius(int ID) {
        return cohesion_radii[ID];
    }

    // returns a vector of IDs that can be used to call get_group() and get_path_tiles()
    std::vector<int> get_valid_IDs() {
        std::vector<int> IDs;
        for (unsigned i = 0; i < statuses.size(); ++i) {
            if (statuses[i] != UNUSED_GROUP) {
                IDs.push_back(i);
            }
        }
        return IDs;
    }
}