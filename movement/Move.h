#pragma once
#pragma message("including Move")

#include <BWAPI.h>
#include <BWEB/BWEB.h>
#include "../utility/BWTimer.h"

namespace Movement::Move {

    enum STATUS {
        WAITING_MOVE,
        WAITING_ATTACK,
        MOVING,
        ATTACK_MOVING,
        DESTINATION,
        UNREACHABLE_WAYPOINT,
        KILLED,
        COHESION_WAIT_TIMEOUT,
        UNUSED_GROUP,
        BAD_GROUP_ID
    };

    enum DIRECTIONS {
        UP,
        RIGHT,
        DOWN,
        LEFT
    };

    enum PATHFINDING_ERROR {
        INVALID_SRC=-1,
        INVALID_DEST=-2,
        UNREACHABLE_DEST=-3
    };

    const int COHENSION_NUM_VALS = 5;
    const double C_MIN = 5.0,
                 C_LOW = 4.0,
                 C_MED = 3.3,
                 C_HIGH = 2.0,
                 C_MAX = 1.0;
    const double COHESION_SCALE[] {C_MIN, C_LOW, C_MED, C_HIGH, C_MAX};

    // Moves a group of units along a path to the nearest valid tileposition to the given 
    // destination. When this move order is no longer needed, remove(group_ID) needs to be called.
    //   <returns>        a reference group ID if path is solved, otherwise returns a 
    //                    PATHFINDING_ERROR code
    //   :attack:         if true, attack move
    //   :wait:           if true, wait until start() is called with this group ID
    //   :group_cohesion: lower number (1.0 min) = more cohesiveness, or how much units stay 
    //                    together when ordered to move together. large number = units are okay
    //                    with spreading out. in other words, it's a radius size multiplier.
    int     move(
                std::vector<BWAPI::Unit> &units, 
                BWAPI::TilePosition dest,
                bool attack=false,
                bool wait=false,
                double group_cohesion=C_MED
            );

    // Turns a single unit into a group of one and calls the std::vector<BWAPI::Unit> move()
    int     move(
                const BWAPI::Unit u,
                BWAPI::TilePosition dest,
                bool attack=false,
                bool wait=false
            );
    
    // Returns the Movement::Move::STATUS code of the group
    STATUS  get_status(int group_ID);

    // If you called move() with wait=true, you have to call start() to start moving
    void    start(int group_ID);

    // Remaining point/pixel distance on path
    int     remaining_distance(int group_ID);

    // Remaining time in frames
    int     remaining_frames(int group_ID);

    // Removes the group from Move storage.
    //   <returns> true if successfully removed a group that existed in Move storage
    bool    remove(int group_ID);

    void    on_frame_update();

    const std::vector<BWAPI::Unit> &get_group(int ID);

    const std::vector<BWAPI::TilePosition> get_path_tiles(int ID);

    int get_waypoint(int ID);

    BWAPI::Position get_avg_position(int ID);

    int get_cohesion_radius(int ID);

    std::vector<int> get_valid_IDs();
}