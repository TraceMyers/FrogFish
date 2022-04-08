#pragma once
#pragma message("including BuildGraph")

#include <BWEM/bwem.h>
#include <BWAPI.h>

namespace Production::BuildGraph {

    typedef struct BuildNode {
        const BWEM::Tile&        tile;
        BWAPI::TilePosition      tilepos;
        BWAPI::Position          pos;
        std::vector<BuildNode *> edges;
        std::vector<int>         buildable_dimensions;
        bool                     blocks_mining;
        bool                     blocks_larva;
        bool                     occupied;
        bool                     reserved;
        int                      id;

        BuildNode(const BWEM::Tile &_t, BWAPI::TilePosition tp, int _id) :
        tile(_t),
        tilepos(tp),
        pos(BWAPI::Position(tp)),
        id(_id),
        edges(4),
        buildable_dimensions(2),
        blocks_mining(false),
        blocks_larva(false),
        occupied(false),
        reserved(false)
        {}

        void        set_buildable_dimensions(int x, int y) {
            buildable_dimensions[0] = x;
            buildable_dimensions[1] = y;
        }
        BuildNode * get_edge(int dir) {return edges[dir];}
        bool        operator ==(const BuildNode &other) const;
    } *BNode;

    void                        init();
    void                        on_frame_update();
    bool                        buildable(BWAPI::TilePosition &tilepos);
    std::vector<BNode> *        build_nodes();
    void                        clear();
    bool                        base_has_graph(const BWEM::Base *base);
    BWAPI::TilePosition         get_build_tilepos(const BWEM::Base *base, int width, int height);
    BWAPI::TilePosition         get_geyser_tilepos(const BWEM::Base *base);
    int                         make_reservation(
                                    const BWEM::Base *b,
                                    const BWAPI::TilePosition &tilepos, 
                                    int width, 
                                    int height
                                );
    int                         make_geyser_reservation(const BWAPI::TilePosition &tilepos);
    void                        end_reservation(int ID);
    bool                        reservation_exists(int ID);
    void                        free_data();
}
