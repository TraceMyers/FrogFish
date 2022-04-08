#include "Bases.h"
#include "Units.h"
#include <unordered_map>

using namespace Basic;

namespace Basic::Bases {

    namespace {
        
        struct BaseData {
            std::vector<BWAPI::Unit>
                larva,
                workers,
                structures,
                resource_depots;

            void clear() {
                larva.clear();
                workers.clear();
                structures.clear();
                resource_depots.clear();
            }
        };

        enum WHO {
            SELF,
            ENEMY
        };

        std::vector<const BWEM::Base *> _all_bases;
        std::vector<const BWEM::Base *> _neutral_bases;
        std::vector<const BWEM::Base *> _self_bases;
        std::vector<const BWEM::Base *> _enemy_bases;

        std::vector<const BWEM::Base *> _self_just_stored;
        std::vector<const BWEM::Base *> _self_just_removed;
        std::vector<const BWEM::Base *> _enemy_just_stored;
        std::vector<const BWEM::Base *> _enemy_just_removed;

        std::unordered_map<const BWEM::Base *, BaseData> base_to_data;

        void add_self_base(const BWEM::Base *b) {
            auto base_it = std::find(_neutral_bases.begin(), _neutral_bases.end(), b);
            if (base_it != _neutral_bases.end()) {
                _self_bases.push_back(*base_it);
                _self_just_stored.push_back(*base_it);
                _neutral_bases.erase(base_it);
            }
            else {
                printf("Basic::Bases::add_self_base(): tried to add non-neutral base\n");
            }
        }

        void add_enemy_base(const BWEM::Base *b) {
            auto base_it = std::find(_neutral_bases.begin(), _neutral_bases.end(), b);
            if (base_it != _neutral_bases.end()) {
                _enemy_bases.push_back(*base_it);
                _enemy_just_stored.push_back(*base_it);
                _neutral_bases.erase(base_it);
            }
            else {
                printf("Basic::Bases::add_enemy_base(): tried to add non-neutral base\n");
            }
        }

        void assign_by_structures(const UnitArray &units, WHO who) {
            for (int i = 0; i < units.size(); ++i) {
                const BWAPI::Unit &unit = units[i];
                const Units::UnitData &unit_data = Units::data(unit);
                if (unit_data.u_type == Refs::UTYPE::STRUCT) {
                    const BWAPI::TilePosition structure_tilepos = unit_data.tilepos;
                    if (the_map.Valid(structure_tilepos)) {
                        const BWEM::Area *structure_area = the_map.GetArea(structure_tilepos);
                        if (structure_area != nullptr) {
                            auto &area_bases = structure_area->Bases();
                            std::vector<const BWEM::Base *> potential_bases;
                            for (auto &base : area_bases) {
                                auto base_it = std::find(
                                    _neutral_bases.begin(), 
                                    _neutral_bases.end(),
                                    &base);
                                if (base_it != _neutral_bases.end()) {
                                    potential_bases.push_back(&base);
                                }
                            }
                            if (potential_bases.size() == 1) {
                                const BWEM::Base *new_base = potential_bases[0];
                                if (who == SELF) {add_self_base(new_base);}
                                else             {add_enemy_base(new_base);}
                            }
                            else if (potential_bases.size() > 1) {
                                std::vector<double> distances;
                                for (auto &base : potential_bases) {
                                    distances.push_back(
                                        structure_tilepos.getApproxDistance(base->Location())
                                    );
                                }
                                auto min_dist_it = 
                                    std::min_element(distances.begin(), distances.end());
                                int base_index = std::distance(distances.begin(), min_dist_it);
                                const BWEM::Base *new_base = potential_bases[base_index];
                                if (who == SELF) {add_self_base(new_base);}
                                else             {add_enemy_base(new_base);}
                            }
                        }
                    }
                }
            }
        }

        void assign_new_bases() {
            const UnitArray &self_new_units = Units::self_just_stored();
            assign_by_structures(self_new_units, SELF);
            const UnitArray &self_morphed_units = Units::self_just_changed_type();
            assign_by_structures(self_morphed_units, SELF);
            const UnitArray &enemy_new_units = Units::enemy_just_stored();
            assign_by_structures(enemy_new_units, ENEMY);

            const BWAPI::Race &enemy_race = Broodwar->enemy()->getRace();
            if (enemy_race == BWAPI::Races::Zerg) {
                const UnitArray &enemy_morphed_units = Units::enemy_just_changed_type();
                assign_by_structures(enemy_morphed_units, ENEMY);
                const UnitArray &enemy_moved_units = Units::enemy_just_moved();
                assign_by_structures(enemy_moved_units, ENEMY);
            }
            else if (enemy_race == BWAPI::Races::Terran) {
                const UnitArray &enemy_moved_units = Units::enemy_just_moved();
                assign_by_structures(enemy_moved_units, ENEMY);
            }
        }

        void unassign_empty_bases() {
            for (auto base_it = _self_bases.begin(); base_it != _self_bases.end(); ) {
                const BWEM::Base *&base = *base_it;
                unsigned structure_ct = base_to_data[base].structures.size();
                if (structure_ct == 0) {
                    _neutral_bases.push_back(*base_it);
                    _self_just_removed.push_back(*base_it);
                    base_it = _self_bases.erase(base_it);
                }
                else {
                    ++base_it;
                }
            }
            for (auto base_it = _enemy_bases.begin(); base_it != _enemy_bases.end(); ) {
                const BWEM::Base *&base = *base_it;
                unsigned structure_ct = base_to_data[base].structures.size();
                if (structure_ct == 0) {
                    _neutral_bases.push_back(*base_it);
                    _self_just_removed.push_back(*base_it);
                    base_it = _enemy_bases.erase(base_it);
                }
                else {
                    ++base_it;
                }
            }
        }

        bool base_has_structure(const BaseData &bdata, const BWAPI::Unit &u) {
            auto &base_structs = bdata.structures;
            for (auto it = base_structs.begin(); it != base_structs.end(); ++it) {
                if(*it == u) {
                    return true;
                }
            }
            return false;
        }

        bool base_remove_structure(BaseData &bdata, const BWAPI::Unit &u) {
            auto &base_structs = bdata.structures;
            for (auto it = base_structs.begin(); it != base_structs.end(); ++it) {
                if(*it == u) {
                    base_structs.erase(it);
                    return true;
                }
            }
            return false;
        }

        bool base_remove_depot(BaseData &bdata, const BWAPI::Unit &u) {
            auto &base_depots = bdata.resource_depots;
            for (auto it = base_depots.begin(); it != base_depots.end(); ++it) {
                if(*it == u) {
                    base_depots.erase(it);
                    return true;
                }
            }
            return false;
        }

        bool base_has_larva(const BaseData &bdata, const BWAPI::Unit &u) {
            auto &base_larva = bdata.larva;
            for (auto it = base_larva.begin(); it != base_larva.end(); ++it) {
                if(*it == u) {
                    return true;
                }
            }
            return false;
        }

        bool base_remove_larva(BaseData &bdata, const BWAPI::Unit &u) {
            auto &base_larva = bdata.larva;
            for (auto it = base_larva.begin(); it != base_larva.end(); ++it) {
                if(*it == u) {
                    base_larva.erase(it);
                    return true;
                }
            }
            return false;
        }

        bool base_has_worker(const BaseData &bdata, const BWAPI::Unit &u) {
            auto &base_workers = bdata.workers;
            for (auto it = base_workers.begin(); it != base_workers.end(); ++it) {
                if(*it == u) {
                    return true;
                }
            }
            return false;
        }

        bool base_remove_worker(BaseData &bdata, const BWAPI::Unit &u) {
            auto &base_workers = bdata.workers;
            for (auto it = base_workers.begin(); it != base_workers.end(); ++it) {
                if(*it == u) {
                    base_workers.erase(it);
                    return true;
                }
            }
            return false;
        }

        void assign_asset_to_base(const BWEM::Base *b, const BWAPI::Unit &u, UTYPE u_type) {
            BaseData &base_data = base_to_data[b];
            switch(u_type) {
                case UTYPE::STRUCT:
                    base_data.structures.push_back(u);
                    if (u->getType().isResourceDepot()) {base_data.resource_depots.push_back(u);}
                    break;
                case UTYPE::WORKER:
                    base_data.workers.push_back(u);
                    break;
                case UTYPE::LARVA:
                    base_data.larva.push_back(u);
                    break;
                default:
                    printf("Basic::Bases::assign_asset_to_base() bad type\n");
                    assert(false);
            }
        }

        void assign_assets(WHO who) {
            auto &units = (who == SELF ? Units::self_units() : Units::enemy_units());
            for (int i = 0; i < units.size(); ++i) {
                const BWAPI::Unit &unit = units[i];
                const Units::UnitData &unit_data = Units::data(unit);
                if (
                    unit_data.u_type == UTYPE::WORKER
                    || unit_data.u_type == UTYPE::STRUCT
                    || unit_data.u_type == UTYPE::LARVA
                    || unit_data.type == BWAPI::UnitTypes::Zerg_Egg
                ) {
                    const BWAPI::TilePosition asset_tp = unit_data.tilepos;
                    if (the_map.Valid(asset_tp)) {
                        const BWEM::Area *asset_area = the_map.GetArea(asset_tp);
                        UTYPE u_type = unit_data.u_type;
                        if (asset_area != nullptr) {
                            std::vector<const BWEM::Base *> potential_bases;
                            auto &bases = (who == SELF ? _self_bases : _enemy_bases);
                            for (auto base_it = bases.begin(); base_it != bases.end(); ++base_it) {
                                const BWEM::Base *base = *base_it;
                                BaseData &base_data = base_to_data[base];
                                if (asset_area->Id() == base->GetArea()->Id()) {
                                    if (unit_data.missing) {
                                        if (
                                            u_type == UTYPE::STRUCT
                                            && base_remove_structure(base_data, unit)
                                        ) {
                                            base_remove_depot(base_data, unit);
                                        }
                                    }
                                    else if (
                                        (u_type == UTYPE::STRUCT 
                                        && !base_has_structure(base_data, unit))
                                        ||
                                        (u_type == UTYPE::WORKER
                                        && !base_has_worker(base_data, unit))
                                        ||
                                        (u_type == UTYPE::LARVA
                                        && !base_has_larva(base_data, unit)) 
                                    ) {
                                        potential_bases.push_back(base);
                                    }
                                    else if (
                                        u_type != UTYPE::STRUCT
                                        && base_remove_structure(base_data, unit)
                                    ) {
                                        base_remove_depot(base_data, unit);
                                    }
                                    else if (
                                        u_type != UTYPE::WORKER
                                        && base_remove_worker(base_data, unit)
                                    ) {}
                                    else if (
                                        u_type != UTYPE::LARVA
                                        && base_remove_larva(base_data, unit)
                                    ) {}
                                }
                                else if (
                                    u_type == UTYPE::STRUCT
                                    && base_remove_structure(base_data, unit)
                                ) {
                                    base_remove_depot(base_data, unit);
                                }
                                else if (
                                    u_type == UTYPE::WORKER
                                    && base_remove_worker(base_data, unit)
                                ) {}
                                else if (
                                    u_type == UTYPE::LARVA
                                    && base_remove_larva(base_data, unit)
                                ) {}
                            }
                            if (potential_bases.size() == 1) {
                                auto &base = potential_bases[0];
                                assign_asset_to_base(base, unit, u_type);
                            }
                            else if (potential_bases.size() > 1) {
                                std::vector<double> distances;
                                for (auto &base : potential_bases) {
                                    distances.push_back(
                                        asset_tp.getApproxDistance(base->Location())
                                    );
                                }
                                auto min_dist = std::min_element(distances.begin(), distances.end());
                                int base_i = std::distance(distances.begin(), min_dist);
                                auto &base = potential_bases[base_i];
                                assign_asset_to_base(base, unit, u_type);
                            }
                        }
                    }
                }
            }
        }

        void remove_dead_assets(WHO who) {
            auto &bases = (who == SELF ? _self_bases : _enemy_bases);
            auto &just_destroyed = (
                who == SELF 
                ? Units::self_just_destroyed()
                : Units::enemy_just_destroyed()
            );
            for (int i = 0; i < just_destroyed.size(); ++i) {
                const BWAPI::Unit &u = just_destroyed[i];
                const Units::UnitData &unit_data = Units::data(u);
                const UTYPE u_type = unit_data.u_type;
                bool is_depot = unit_data.type.isResourceDepot();
                printf("processing just destroyed : %s\n", unit_data.type.c_str());
                if (u_type == UTYPE::STRUCT) {
                    if (is_depot) {
                        for (auto &base : bases) {
                            BaseData &base_data = base_to_data[base];
                            base_remove_structure(base_data, u);
                            base_remove_depot(base_data, u);
                        }
                    }
                    else {
                        for (auto &base : bases) {
                            BaseData &base_data = base_to_data[base];
                            base_remove_structure(base_data, u);
                        }
                    }
                }
                else if (u_type == UTYPE::WORKER) {
                    for (auto &base : bases) {
                        BaseData &base_data = base_to_data[base];
                        base_remove_worker(base_data, u);
                    }
                }
                else if (u_type == UTYPE::LARVA) {
                    for (auto &base : bases) {
                        BaseData &base_data = base_to_data[base];
                        base_remove_larva(base_data, u);
                    }
                }
            }
        }
    }

    void init() {
        const std::vector<BWEM::Area> &areas = the_map.Areas();
        for (auto &area : areas) {
            const std::vector<BWEM::Base> &bases = area.Bases();
            for (unsigned int i = 0; i < bases.size(); ++i) {
                _neutral_bases.push_back(&bases[i]);
                _all_bases.push_back(&bases[i]);
                base_to_data[&bases[i]] = BaseData();
            }
        }
    }

    void on_frame_update() {
        clear_just_added_and_removed();
        assign_new_bases();
        assign_assets(SELF);
        assign_assets(ENEMY);
        remove_dead_assets(SELF);
        remove_dead_assets(ENEMY);
        unassign_empty_bases();
    }

    void clear_just_added_and_removed() {
        _self_just_stored.clear();
        _enemy_just_stored.clear();
        for (auto base : _self_just_removed) {
            base_to_data[base].clear();
        }
        _self_just_removed.clear();
        for (auto base : _enemy_just_removed) {
            base_to_data[base].clear();
        }
        _enemy_just_removed.clear();
    }

    bool is_self(const BWEM::Base *b) {
        for (auto &base : _self_bases) {
            if (base == b) {return true;}
        }
        return false;
    }

    bool is_enemy(const BWEM::Base *b) {
        for (auto &base : _enemy_bases) {
            if (base == b) {return true;}
        }
        return false;
    }

    bool is_neutral(const BWEM::Base *b) {
        for (auto &base : _neutral_bases) {
            if (base == b) {return true;}
        }
        return false;
    }

    const std::vector<const BWEM::Base *> &all_bases() {return _all_bases;}

    const std::vector<const BWEM::Base *> &neutral_bases() {return _neutral_bases;}

    const std::vector<const BWEM::Base *> &self_bases() {return _self_bases;}

    const std::vector<const BWEM::Base *> &enemy_bases() {return _enemy_bases;}

    const std::vector<const BWEM::Base *> &self_just_stored() {return _self_just_stored;}

    const std::vector<const BWEM::Base *> &self_just_removed() {return _self_just_removed;}

    const std::vector<const BWEM::Base *> &enemy_just_stored() {return _enemy_just_stored;}

    const std::vector<const BWEM::Base *> &enemy_just_removed() {return _enemy_just_removed;}

    const std::vector<BWAPI::Unit> larva (const BWEM::Base *b) {return base_to_data[b].larva;}

    const std::vector<BWAPI::Unit> workers (const BWEM::Base *b) {return base_to_data[b].workers;}

    const std::vector<BWAPI::Unit> structures (const BWEM::Base *b) {return base_to_data[b].structures;}

    const std::vector<BWAPI::Unit> depots (const BWEM::Base *b) {return base_to_data[b].resource_depots;}

    // currently only used for when extractors are killed & canceled
    // since it isn't removed in time to avoid a read access violation otherwise
    void remove_struct_from_owner_base(const BWAPI::Unit unit) {
        auto base_it = std::find_if(
            _self_bases.begin(),
            _self_bases.end(),
            [unit](const BWEM::Base *b) {
                std::vector<BWAPI::Unit> &base_structs = base_to_data[b].structures;
                auto it = std::find(base_structs.begin(), base_structs.end(), unit);
                return it != base_structs.end();
            }
        );
        if (base_it != _self_bases.end()) {
            BaseData &base_data = base_to_data[*base_it];
            base_remove_structure(base_data, unit);
            base_remove_depot(base_data, unit);
        }
    }
}