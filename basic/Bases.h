#pragma once
#pragma message("including Bases")

#include "../FrogFish.h"
#include <BWEM/bwem.h>
#include <BWAPI.h>
#include <vector>

namespace Basic::Bases {
    void                                   init();
    void                                   on_frame_update();
    void                                   clear_just_added_and_removed();
    bool                                   is_self(const BWEM::Base *b);
    bool                                   is_enemy(const BWEM::Base *b);
    bool                                   is_neutral(const BWEM::Base *b);
    const std::vector<const BWEM::Base *> &all_bases();
    const std::vector<const BWEM::Base *> &neutral_bases();
    const std::vector<const BWEM::Base *> &self_bases();
    const std::vector<const BWEM::Base *> &enemy_bases();
    const std::vector<const BWEM::Base *> &self_just_stored();
    const std::vector<const BWEM::Base *> &self_just_removed();
    const std::vector<const BWEM::Base *> &enemy_just_stored();
    const std::vector<const BWEM::Base *> &enemy_just_removed(); 
    const std::vector<BWAPI::Unit>         larva (const BWEM::Base *b);
    const std::vector<BWAPI::Unit>         workers (const BWEM::Base *b);
    const std::vector<BWAPI::Unit>         structures (const BWEM::Base *b);
    const std::vector<BWAPI::Unit>         depots (const BWEM::Base *b);
    void                                   remove_struct_from_owner_base(const BWAPI::Unit unit);
}
