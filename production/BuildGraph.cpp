#include "BuildGraph.h"
#include "../FrogFish.h"
#include "../utility/FrogMath.h"
#include "../basic/Bases.h"
#include "../basic/Units.h"
#include "../test/TestMessage.h"
#include <cmath>
#include <algorithm>

using namespace Basic;

namespace Production::BuildGraph {

// TODO: to scan for nodes that can build out, look for nodes missing edges, rather than
// trying to build out from just any node. Do this at one base per frame
// TODO: inventory scanning algorithm from Unreal first person game helpful?
// TODO: create more effective system for stopping units from building between hatch & resources;
//       allow for exceptions in special circumstances
// TODO: use 'occupied' status for nodes
// TODO: unreserve site function

    namespace {

        struct Reservation {
            Reservation (int _width, int _height, const BWAPI::TilePosition &_upper_left) :
                width(_width), height(_height), upper_left(_upper_left)
            {}
            int width;
            int height;
            BWAPI::TilePosition upper_left;
        };

        static const int    MAX_BASES = 30;
        std::vector<BNode>  _build_nodes[MAX_BASES];
        std::vector<BNode>  geyser_nodes[MAX_BASES];

        int                 base_ct = 0;
        int                 node_ID_counter = 0;
        std::vector<std::pair<BNode,int>>  remove_queue;

        std::vector<std::pair<double, double>>  resource_blocking_vectors[MAX_BASES];
        std::vector<double>                     blocking_vector_magnitudes[MAX_BASES];
        enum                DIRECTIONS {RIGHT, UP, LEFT, DOWN};

        const int           CHUNK_SIZE = 30;
        int                 start_chunk[MAX_BASES] {0};
        int                 end_chunk[MAX_BASES] {CHUNK_SIZE};

        std::map<int, Reservation> tile_reservations;
        int                 reservation_ID_counter = 0;

        void set_resource_blocking_vectors(int base_index) {
            resource_blocking_vectors[base_index].clear();
            const BWEM::Base *base = Bases::all_bases()[base_index];
            auto &minerals = base->Minerals();
            if (minerals.size() <= 1 || Bases::depots(base).size() == 0) {
                return;
            }
            BWAPI::Position hatch_center = base->Center();
            hatch_center.x += 64;
            hatch_center.y += 48;

            auto &geysers = base->Geysers();
            if (geysers.size() > 0) {
                for (auto & geyser : geysers) {
                    BWAPI::Position geyser_pos = geyser->Pos();
                    geyser_pos.x += 48;
                    geyser_pos.y += 32;
                    if (geyser_pos.getApproxDistance(hatch_center) < 400) {
                        std::pair<double, double> geyser_vec = 
                            Utility::FrogMath::full_vector(hatch_center, geyser_pos);
                        resource_blocking_vectors[base_index].push_back(geyser_vec);
                    } 
                }
            }
            for (auto &mineral : minerals) {
                BWAPI::Position mineral_pos = mineral->Pos();
                mineral_pos.x += 32;
                mineral_pos.y += 16;
                if (mineral_pos.getApproxDistance(hatch_center) < 300) {
                    std::pair<double, double> mineral_vec = 
                        Utility::FrogMath::full_vector(hatch_center, mineral_pos);
                    resource_blocking_vectors[base_index].push_back(mineral_vec);
                    double mineral_dist = Utility::FrogMath::magnitude(mineral_vec);
                    blocking_vector_magnitudes[base_index].push_back(mineral_dist);
                }
            }
        }

        BNode find_node_at(int base_index, const BWAPI::TilePosition &tilepos) {
            if (tilepos.isValid()) {
                auto node_it = find_if(
                    _build_nodes[base_index].begin(), 
                    _build_nodes[base_index].end(), 
                    [tilepos]
                        (const BNode &bn)
                        {return bn->tilepos == tilepos;}
                );
                if (node_it != _build_nodes[base_index].end()) {
                    return *node_it;
                }
            }
            return nullptr;
        }

        // center of tilepos?
        void flag_resource_blocking_node(
            BNode node, 
            BWAPI::Position base_center,
            const std::vector<std::pair<double, double>> &blocking_vectors,
            const std::vector<double> &vector_magnitudes
        ) {
            std::pair<double, double> node_vec = 
                Utility::FrogMath::full_vector(base_center, node->pos);
            node->blocks_mining = false;
            bool close_angle = false;
            for (auto& resource_vec : blocking_vectors) {
                if (Utility::FrogMath::vector_angle(resource_vec, node_vec) < 0.6) {
                    close_angle = true;
                    break;
                    
                }
            }
            if (close_angle) {
                double node_dist = Utility::FrogMath::magnitude(node_vec); 
                for (double mineral_dist : vector_magnitudes) {
                    if (node_dist <= mineral_dist) {
                        node->blocks_mining = true;
                        return;
                    }
                }
            }
        }

        void seed_creep(int base_index, const BWAPI::TilePosition &tilepos) {
            const BWEM::Tile &_t = the_map.GetTile(tilepos);
            if (
                tilepos.isValid() 
                && find_node_at(base_index, tilepos) == nullptr
                && _t.Walkable()
                && Broodwar->hasCreep(tilepos)
            ) {
                BNode new_node = new BuildNode(_t, tilepos, node_ID_counter);
                flag_resource_blocking_node(
                    new_node, 
                    Bases::all_bases()[base_index]->Center(),
                    resource_blocking_vectors[base_index],
                    blocking_vector_magnitudes[base_index]
                );
                _build_nodes[base_index].push_back(new_node);
                ++node_ID_counter;
            }
        }

        bool node_near_hatch(BNode node, std::vector<BWAPI::TilePosition> hatch_tilepositions) {
            BWAPI::TilePosition node_tilepos = node->tilepos;
            int path[16][2] {
                {0, -3}, {1, 0}, {1, 0}, {0, 1}, 
                {0, 1}, {0, 1}, {0, -3}, {-3, 0}, 
                {-1, 0}, {-1, 0}, {-1, 0}, {0, 1}, 
                {0, 1}, {0, 1}, {0, 1}
            };
            for (int i = 0; i < 16; ++i) {
                node_tilepos.x += path[i][0];
                node_tilepos.y += path[i][1];
                for (auto &hatch_tp : hatch_tilepositions) {
                    if (hatch_tp == node_tilepos) {
                        node->blocks_larva = true;
                        return true;
                    }
                }
            } 
            node->blocks_larva = false;
            return false;
        }

        void update_chunk(int base_index) {
            auto &base = Bases::all_bases()[base_index];
            for (int i = start_chunk[base_index]; i < end_chunk[base_index]; ++i) {
                BNode check_node = _build_nodes[base_index][i];
                int disconnected_ct = 0;
                bool this_node_mineral_blocking = check_node->blocks_mining;
                
                for (auto &edge : check_node->edges) {
                    if (edge == nullptr) {
                        ++disconnected_ct;
                    }
                }
                if (disconnected_ct == 4 || !Broodwar->hasCreep(check_node->tilepos)) {
                    remove_queue.push_back(std::pair<BNode, int>(check_node, base_index));
                    continue;
                }

                check_node->buildable_dimensions[0] = 0;
                auto &hatches = Bases::depots(base);
                std::vector<BWAPI::TilePosition> hatch_tilepositions;
                for (auto &hatch : hatches) {
                    hatch_tilepositions.push_back(Units::data(hatch).tilepos);
                }
                bool below_hatch = node_near_hatch(check_node, hatch_tilepositions);
                if (below_hatch) {
                    continue;
                }

                BNode bnode = check_node->get_edge(RIGHT);
                int j;
                if (bnode != nullptr) {
                    int buildable_size_path[11] {
                        LEFT, DOWN, RIGHT, RIGHT, UP, RIGHT, DOWN, DOWN, LEFT, LEFT, LEFT
                    };
                    for (j = 0; j < 12; ++j) {
                        const BWAPI::TilePosition tp = bnode->tilepos;
                        if (
                            !Broodwar->isBuildable(tp, true) 
                            ||
                            (!this_node_mineral_blocking && bnode->blocks_mining)
                            ||
                            bnode->blocks_larva
                            ||
                            bnode->reserved
                        ) {
                            break;
                        }
                        if (j == 3) {
                            check_node->set_buildable_dimensions(2, 2);
                        }
                        else if (j == 5) {
                            check_node->set_buildable_dimensions(3, 2);
                        }
                        else if (j == 7) {
                            check_node->set_buildable_dimensions(4, 2);
                        }
                        else if (j == 11) {
                            check_node->set_buildable_dimensions(4, 3);
                            break;
                        }
                        bnode = bnode->get_edge(buildable_size_path[j]);
                        if (bnode == nullptr) {
                            break;
                        }
                    }
                }
            }
        }

        void try_expand(int base_index) {
            for (int i = start_chunk[base_index]; i < end_chunk[base_index]; ++i) {
                BNode cur_node = _build_nodes[base_index][i];
                const BWAPI::TilePosition &tp = cur_node->tilepos;
                if (Broodwar->isVisible(tp)) {
                    int cardinal[4][2] {{1, 0}, {0, -1}, {-1, 0}, {0, 1}};
                    for (int j = 0; j < 4; ++j) {
                        if (cur_node->edges[j] == nullptr) {
                            BWAPI::TilePosition check_tp(tp.x + cardinal[j][0], tp.y + cardinal[j][1]);
                            if (
                                check_tp.isValid()
                                && the_map.GetTile(check_tp).Walkable()
                                && Broodwar->hasCreep(check_tp)
                            ) {
                                BNode check_node = find_node_at(base_index, check_tp);
                                if (check_node == nullptr) {
                                    const BWEM::Tile &t = the_map.GetTile(check_tp);
                                    check_node = new BuildNode(t, check_tp, node_ID_counter);
                                    flag_resource_blocking_node(
                                        check_node, 
                                        Bases::all_bases()[base_index]->Center(),
                                        resource_blocking_vectors[base_index],
                                        blocking_vector_magnitudes[base_index]
                                    );
                                    _build_nodes[base_index].push_back(check_node);
                                    ++node_ID_counter;
                                }
                                cur_node->edges[j] = check_node;
                                switch(j) {
                                    case RIGHT: // 0
                                        check_node->edges[LEFT] = cur_node;
                                        break;
                                    case UP:    // 1
                                        check_node->edges[DOWN] = cur_node;
                                        break;
                                    case LEFT:  // 2
                                        check_node->edges[RIGHT] = cur_node;
                                        break;
                                    case DOWN:  // 3
                                        check_node->edges[UP] = cur_node;
                                }
                            }
                        }
                    }
                }
            }
        }

        void remove_dead_nodes() {
            BNode edge;
            for (auto node : remove_queue) {
                auto &bnode = node.first;
                int base_index = node.second;
                if((edge = bnode->get_edge(RIGHT)) != nullptr)  edge->edges[LEFT] = nullptr;
                if((edge = bnode->get_edge(UP)) != nullptr)     edge->edges[DOWN] = nullptr;
                if((edge = bnode->get_edge(LEFT)) != nullptr)   edge->edges[RIGHT] = nullptr;
                if((edge = bnode->get_edge(DOWN)) != nullptr)   edge->edges[UP] = nullptr;
                auto node_it = std::remove(
                    _build_nodes[base_index].begin(), 
                    _build_nodes[base_index].end(), 
                    bnode
                );
                if (node_it != _build_nodes[base_index].end()) {
                    _build_nodes[base_index].erase(node_it);
                }
                delete bnode;
            }
            remove_queue.clear();
        }

        // checks if a build path is valid and returns the upper left (tilepos) BNode if so, nullptr if not
        BNode valid_build_path(int base_index, const BWAPI::TilePosition &tilepos, int width, int height) {
            BNode start_bnode = find_node_at(base_index, tilepos);
            BNode bnode = start_bnode;
            int 
                ref_side_steps = width - 1,
                side_steps = ref_side_steps,
                down_steps = height - 1;
            DIRECTIONS side_dir = RIGHT;
            bool bad_path = false;
            while (down_steps >= 0) {
                while (true) {
                    if (bnode == nullptr || bnode->occupied || bnode->reserved) {
                        bad_path = true;
                        break;
                    }
                    --side_steps;
                    if (side_steps < 0) {break;}
                    bnode = bnode->edges[side_dir];
                }
                if (bad_path) {break;}
                side_dir = (side_dir == RIGHT ? LEFT : RIGHT);
                side_steps = ref_side_steps;
                bnode = bnode->edges[DOWN];
                --down_steps;
            }
            return (bad_path ? nullptr : start_bnode);
        }

        // does not check path validity; should always be used after valid_build_path
        void set_build_path_reserved_status(BNode upper_left, int width, int height, bool status) {
            BNode bnode = upper_left;
            int 
                ref_side_steps = width - 1,
                side_steps = ref_side_steps,
                down_steps = height - 1;
            DIRECTIONS side_dir = RIGHT;
            while (down_steps >= 0) {
                while (true) {
                    bnode->reserved = status;
                    --side_steps;
                    if (side_steps < 0) {break;}
                    bnode = bnode->edges[side_dir];
                }
                side_dir = (side_dir == RIGHT ? LEFT : RIGHT);
                side_steps = ref_side_steps;
                bnode = bnode->edges[DOWN];
                --down_steps;
            }
        }
    }

    void init() {
        base_ct = Bases::all_bases().size();
        for (int i = 0; i < base_ct; ++i) {
            for (int j = 0; j < 3; ++j) {
                resource_blocking_vectors[i].push_back(std::pair<double, double>(-1, -1));
            }
        }
    }

    void on_frame_update() {
        auto &bases = Bases::all_bases();
        for (int i = 0; i < base_ct; ++i) {
            const BWEM::Base *base = bases[i];
            for (auto &structure : Bases::structures(base)) {
                const Units::UnitData &unit_data = Units::data(structure);
                seed_creep(i, unit_data.tilepos);
            }
            int base_nodes_sz = _build_nodes[i].size();
            if (base_nodes_sz > 0) {
                if (end_chunk[i] > base_nodes_sz) {
                    if (start_chunk[i] >= base_nodes_sz) {
                        start_chunk[i] = 0;
                        if (CHUNK_SIZE >= base_nodes_sz) {
                            end_chunk[i] = base_nodes_sz;
                        }
                        else {
                            end_chunk[i] = CHUNK_SIZE;
                        }
                    }
                    else {
                        end_chunk[i] = base_nodes_sz;
                    }
                }
                try_expand(i);
                update_chunk(i);
                if (resource_blocking_vectors[i][0].first < 0) {
                    // TODO: change out with version that updates over the game,
                    // and change func so it doesn't group minerals and gas together,
                    // in case they're - for instance - on opposite sides
                    // Also, resource blocking nodes extend past resources,
                    // when they shouldn't
                    set_resource_blocking_vectors(i);
                }
                start_chunk[i] += CHUNK_SIZE;
                end_chunk[i] += CHUNK_SIZE;
            }
        }
        remove_dead_nodes();
    }

    std::vector<BNode> *build_nodes() {
        return _build_nodes;
    }

    bool base_has_graph(const BWEM::Base *base) {
        auto &bases = Bases::all_bases();
        for (unsigned int i = 0; i < bases.size(); ++i) {
            auto &b = bases[i];
            if (b == base) {
                return _build_nodes[i].size() > 0;
            }
        }
        return false;
    }

    BWAPI::TilePosition get_build_tilepos(const BWEM::Base *base, int width, int height) {
        auto &bases = Bases::all_bases();
        for (unsigned int i = 0; i < bases.size(); ++i) {
            auto &b = bases[i];
            auto &base_nodes = _build_nodes[i];
            if (b == base && base_nodes.size() > 0) {
                for (auto &node : base_nodes) {
                    if (
                        node->buildable_dimensions[0] >= width
                        && node->buildable_dimensions[1] >= height
                        && !node->blocks_mining
                    ) {
                        return node->tilepos;
                    }
                }
            }
        }
        return BWAPI::TilePositions::None;
    }

    BWAPI::TilePosition get_geyser_tilepos(const BWEM::Base *base) {
        auto &geysers = base->Geysers();
        for (auto geyser : geysers) {
            if (geyser->Unit()->getType() == BWAPI::UnitTypes::Zerg_Extractor) {
                continue;
            }
            auto &tp = geyser->TopLeft();
            bool reserved = false;
            for (auto &iter = tile_reservations.begin(); iter != tile_reservations.end(); ++iter) {
                const Reservation &res = iter->second;
                if (res.upper_left == tp) {
                    reserved = true;
                }
            }
            if (!reserved) {
                return tp;
            }
        }
        return BWAPI::TilePositions::None;
    }

    int make_reservation(
        const BWEM::Base *b,
        const BWAPI::TilePosition &tilepos, 
        int width, 
        int height
    ) {
        auto &bases = Bases::all_bases();
        for (auto &base = bases.begin(); base != bases.end(); ++base) {
            if (*base == b) {
                BNode upper_left = valid_build_path(std::distance(bases.begin(), base), tilepos, width, height);
                if (upper_left != nullptr) {
                    tile_reservations.insert(
                        std::pair<int, Reservation> (
                            reservation_ID_counter,
                            Reservation(width, height, tilepos)
                    ));
                    set_build_path_reserved_status(upper_left, width, height, true);
                    return reservation_ID_counter++;
                }
                break;
            }
        }
        return -1;
    }

    int make_geyser_reservation(const BWAPI::TilePosition &tilepos) {
        tile_reservations.insert(
            std::pair<int, Reservation> (
                reservation_ID_counter,
                Reservation(1, 1, tilepos)
        ));
        return reservation_ID_counter++;
    }

    void end_reservation(int ID) {
        tile_reservations.erase(ID);
    }

    bool reservation_exists(int ID) {
        for (auto &iter = tile_reservations.begin(); iter != tile_reservations.end(); ++iter) {
            int res_ID = iter->first;
            if (res_ID == ID) {
                return true;
            }
        }
        return false;
    }

    void free_data() {
        for (int i = 0; i < base_ct; ++i) {
            const std::vector<BNode> &base_nodes = _build_nodes[i];
            for (unsigned int j = 0; j < base_nodes.size(); ++j) {
                delete base_nodes[j];
            }
            _build_nodes[i].clear();
        }
    }

// -----------------------------------------
// ------- getting base distances ----------
// -----------------------------------------

// for (int i = 0; i < base_ct; ++i) {
//     // for each base, create a sorted list of bases by walk-distance closeness
//     const BWEM::Base *from_base = all_bases[i];
//     bases[i] = from_base;
//     int distances[MAX_BASES] {-1};
//     for (int j = 0; j < base_ct; ++j) {
//         if (i != j) {
//             BWEB::Path path;  
//             path.createUnitPath(from_base->Center(), all_bases[j]->Center());                
//             distances[j] = path.getDistance();
//         }
//         else {
//             distances[j] = 0;
//         }
//     }
//     int prev_min = -1;
//     for (int j = 0; j < base_ct; ++j) {
//         int min_dist = INT_MAX;
//         int min_index = 0;
//         for (int k = 0; k < base_ct; ++k) {
//             if (distances[k] < min_dist && distances[k] > prev_min) {
//                 min_dist = distances[k];
//                 min_index = k;
//             }
//         }
//         closest[i][j] = all_bases[min_index];
//         prev_min = min_dist;
//     }
// }
}