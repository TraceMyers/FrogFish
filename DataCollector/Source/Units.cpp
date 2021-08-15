#include "Units.h"
#include "UnitArray.h"
#include "UnitData.h"
#include "Moddefs.h"
#include <fstream>
#include <set>

// TODO: units are all reading as powered and detected (on frame 1 and possibly going forward)
namespace Units {

    const int DOUBLE_BYTES_PER_UNIT = 66;
    const int BUFFLEN = 1600 * DOUBLE_BYTES_PER_UNIT;

    namespace {

        BWAPI::Player p1;
        BWAPI::Player p2;

        BWAPI::Race races[2];
        int counts[2][TERRAN_TYPE_CT] {0};

        std::vector<int> p1_IDs;
        std::vector<int> p2_IDs;

        UnitArray store_buff;
        UnitArray remove_buff;

        UnitArray _p1_units;
        std::vector<int> _p1_just_stored;
        std::vector<int> _p1_just_changed_type;
        std::vector<int> _p1_scheduled_for_removal;

        UnitArray _p2_units;
        std::vector<int> _p2_just_stored;
        std::vector<int> _p2_just_changed_type;
        std::vector<int> _p2_scheduled_for_removal;

        std::map<int, UnitData *> ID_to_data;

        uint16_t units_file_buff[BUFFLEN];
        int _index;
        const int UNIT_DATA_START = 7;

        void store_queued() {
            BWAPI::Unit u;
            for (int i = 0; i < store_buff.size(); ++i) {
                u = store_buff[i];
                if (u->getPlayer() == p1) {
                    int ID = u->getID();
                    if (std::find(p1_IDs.begin(), p1_IDs.end(), ID) == p1_IDs.end()) {
                        p1_IDs.push_back(ID);
                        _p1_units.add(u);
                        ID_to_data[ID] = new UnitData(u);
                        _p1_just_stored.push_back(ID);
                    }
                }
                else if (u->getPlayer() == p2) {
                    int ID = u->getID();
                    if (std::find(p2_IDs.begin(), p2_IDs.end(), ID) == p2_IDs.end()) {
                        p2_IDs.push_back(ID);
                        _p2_units.add(u);
                        ID_to_data[ID] = new UnitData(u);
                        _p2_just_stored.push_back(ID);
                    }
                }
            }
            store_buff.clear();
        }

        void schedule_p1_remove(BWAPI::Unit u) {
            int ID = u->getID();
            auto ID_it = std::find(p1_IDs.begin(), p1_IDs.end(), ID);
            if (ID_it != p1_IDs.end()) {
                _p1_scheduled_for_removal.push_back(*ID_it);
                p1_IDs.erase(ID_it);
                _p1_units.remove(u);
            }
        }

        void schedule_p2_remove(BWAPI::Unit u) {
            int ID = u->getID();
            auto ID_it = std::find(p2_IDs.begin(), p2_IDs.end(), ID);
            if (ID_it != p2_IDs.end()) {
                _p2_scheduled_for_removal.push_back(*ID_it);
                p2_IDs.erase(ID_it);
                _p2_units.remove(u);
            }
        }

        void schedule_removals() {
            for (int i = 0; i < remove_buff.size(); ++i) {
                BWAPI::Unit u = remove_buff[i];
                if (u->getPlayer() == p1) {
                    schedule_p1_remove(u);        
                }
                else if (u->getPlayer() == p2) {
                    schedule_p2_remove(u);        
                }
            }
            remove_buff.clear();
        }

        void update_unit(const BWAPI::Unit &u, UnitData &u_data, BWAPI::Player other_player) {
            u_data.type_ID = u->getType().getID();
            u_data.pos_x = u->getPosition().x;
            u_data.pos_y = u->getPosition().y;
            u_data.target_pos_x = u->getTargetPosition().x;
            u_data.target_pos_y = u->getTargetPosition().y;
            u_data.rally_pos_x = u->getRallyPosition().x;
            u_data.rally_pos_y = u->getRallyPosition().y;
            u_data.velocity_x = u->getVelocityX();
            u_data.velocity_y = u->getVelocityY();
            u_data.angle = u->getAngle();
            u_data.HP = u->getHitPoints();
            u_data.shields = u->getShields();
            u_data.energy = u->getEnergy();
            u_data.kill_count = u->getKillCount();
            u_data.acid_spore_count = u->getAcidSporeCount();
            u_data.interceptor_count = u->getInterceptorCount();
            u_data.spider_mine_count = u->getSpiderMineCount();
            u_data.ground_weapon_cooldown = u->getGroundWeaponCooldown();
            u_data.air_weapon_cooldown = u->getAirWeaponCooldown();
            u_data.spell_cooldown = u->getSpellCooldown();
            u_data.defense_matrix_points = u->getDefenseMatrixPoints();
            u_data.build_type = u->getBuildType().getID();
            u_data.tech_ID = u->getTech().getID();
            u_data.upgrade_ID = u->getUpgrade().getID();
            u_data.remaining_build_time = u->getRemainingBuildTime();
            BWAPI::Unit build_unit = u->getBuildUnit();
            if (build_unit != nullptr) {
                u_data.build_unit_ID = build_unit->getID();
            }
            else {
                u_data.build_unit_ID = ~0;
            }
            BWAPI::Unit target_unit = u->getTarget();
            if (target_unit != nullptr) {
                u_data.target_ID = target_unit->getID();
            }
            else {
                u_data.target_ID = ~0;
            }
            u_data.order_ID = u->getOrder().getID();
            BWAPI::Unit order_target = u->getOrderTarget();
            if (order_target != nullptr) {
                u_data.order_target_ID = order_target->getID();
            }
            else {
                u_data.order_target_ID = ~0;
            }
            BWAPI::Unit rally_unit = u->getRallyUnit();
            if (rally_unit != nullptr) {
                u_data.rally_unit_ID = rally_unit->getID();
            }
            else {
                u_data.rally_unit_ID = ~0;
            }
            BWAPI::Unit addon = u->getAddon();
            if (addon != nullptr) {
                u_data.addon_ID = addon->getID();
            }
            else {
                u_data.addon_ID = ~0;
            }
            BWAPI::Unit nydus_exit = u->getNydusExit();
            if (nydus_exit != nullptr) {
                u_data.nydus_exit_ID = nydus_exit->getID();
            }
            else {
                u_data.nydus_exit_ID = ~0;
            }
            BWAPI::Unit transport = u->getTransport();
            if (transport != nullptr) {
                u_data.transport_ID = transport->getID();
            }
            else {
                u_data.transport_ID = ~0;
            }
            BWAPI::Unit carrier = u->getCarrier();
            if (carrier != nullptr) {
                u_data.carrier_ID = carrier->getID();
            }
            else {
                u_data.carrier_ID = ~0;
            }
            BWAPI::Unit hatchery = u->getHatchery();
            if (hatchery != nullptr) {
                u_data.hatchery_ID = hatchery->getID();
            }
            else {
                u_data.hatchery_ID = ~0;
            }
            u_data.is_attacking = u->isAttacking();
            u_data.is_under_attack = u->isUnderAttack();
            u_data.is_defense_matrixed = u->isDefenseMatrixed();
            u_data.is_ensnared = u->isEnsnared();
            u_data.is_irradiated = u->isIrradiated();
            u_data.is_maelstrommed = u->isMaelstrommed();
            u_data.is_plagued = u->isPlagued();
            u_data.is_stasised = u->isStasised();
            u_data.is_stimmed = u->isStimmed();
            u_data.is_parasited = u->isParasited();
            u_data.is_under_storm = u->isUnderStorm();
            u_data.is_under_dark_swarm = u->isUnderDarkSwarm();
            u_data.is_under_disruption_web = u->isUnderDisruptionWeb();
            u_data.is_burrowed = u->isBurrowed();
            u_data.is_carrying_gas = u->isCarryingGas();
            u_data.is_carrying_minerals = u->isCarryingMinerals();
            u_data.is_powered = u->isPowered();
            u_data.is_cloaked = u->isCloaked();
            u_data.is_completed = u->isCompleted();
            u_data.is_lifted = u->isLifted();
            u_data.is_detected = u->isDetected();
            u_data.is_halluc = u->isHallucination();
            u_data.is_morphing = u->isMorphing();
            u_data.is_sieged = u->isSieged();
            u_data.is_visible = u->isVisible(other_player);
            u_data.is_being_healed = u->isBeingHealed();
            u_data.is_being_constructed = u->isBeingConstructed();
            u_data.is_repairing = u->isRepairing();
            u_data.larva_count = u->getLarva().size();

            int i = 0;
            for (auto training_u : u->getTrainingQueue()) {
                u_data.training_types[i] = training_u.getID();
                ++i;
            }
            for ( ; i < 5; ++i) {
                u_data.training_types[i] = ~0;
            }

            i = 0;
            for (auto loaded_u : u->getLoadedUnits()) {
                u_data.loaded_units[i] = loaded_u->getID();
                ++i;
            }
            for ( ; i < 8; ++i) {
                u_data.loaded_units[i] = ~0;
            }
        }

        void update_units() {
            for (int i = 0; i < _p1_units.size(); ++i) {
                const BWAPI::Unit &u = _p1_units[i];
                int ID = u->getID();
                UnitData &u_data = *ID_to_data[ID];
                if (u_data.type_ID != u->getType().getID()) {
                    _p1_just_changed_type.push_back(ID);
                }
                update_unit(u, u_data, p2);
            }
            for (int i = 0; i < _p2_units.size(); ++i) {
                const BWAPI::Unit &u = _p2_units[i];
                int ID = u->getID();
                UnitData &u_data = *ID_to_data[ID];
                if (u_data.type_ID != u->getType().getID()) {
                    _p2_just_changed_type.push_back(ID);
                }
                update_unit(u, u_data, p1);
            }
        }

        void clear_just_stored_and_removed() {
            _p1_just_stored.clear();
            for (unsigned int i = 0; i < _p1_scheduled_for_removal.size(); ++i) {
                int ID = _p1_scheduled_for_removal[i];
                delete ID_to_data[ID];
                ID_to_data.erase(ID);
            }
            _p1_scheduled_for_removal.clear();
            _p1_just_changed_type.clear();

            _p2_just_stored.clear();
            for (unsigned int i = 0; i < _p2_scheduled_for_removal.size(); ++i) {
                int ID = _p2_scheduled_for_removal[i];
                delete ID_to_data[ID];
                ID_to_data.erase(ID);
            }
            _p2_scheduled_for_removal.clear();
            _p2_just_changed_type.clear();
        }
    }

    void init(const BWAPI::Player *players) {
        p1 = players[0];
        p2 = players[1];
        races[0] = p1->getRace();
        races[1] = p2->getRace();
        units_file_buff[0] = Moddefs::UNITS;
    }

    void clear_units_file_buff() {
        memset((unsigned char*)(units_file_buff) + 2, 0, BUFFLEN * 2 - 2);
        _index = UNIT_DATA_START; // where individual unit data begins
    }

    void write_unit_to_buff(const UnitData &u_data) {
        units_file_buff[_index++] = (uint16_t)u_data.ID; // 0
        units_file_buff[_index++] = (uint16_t)u_data.type_ID; // 1
        units_file_buff[_index++] = (uint16_t)u_data.pos_x; // 2
        units_file_buff[_index++] = (uint16_t)u_data.pos_y; // 3
        units_file_buff[_index++] = (uint16_t)u_data.target_pos_x; // 4
        units_file_buff[_index++] = (uint16_t)u_data.target_pos_y;  // 5
        units_file_buff[_index++] = (uint16_t)u_data.rally_pos_x; // 6
        units_file_buff[_index++] = (uint16_t)u_data.rally_pos_y; // 7
        units_file_buff[_index++] = u_data.velocity_x; // 8
        units_file_buff[_index++] = u_data.velocity_y; // 9
        units_file_buff[_index++] = u_data.angle; // 10
        units_file_buff[_index++] = (uint16_t)u_data.HP; // 11
        units_file_buff[_index++] = (uint16_t)u_data.init_HP; // 12
        units_file_buff[_index++] = (uint16_t)u_data.shields; // 13
        units_file_buff[_index++] = (uint16_t)u_data.energy; // 14
        units_file_buff[_index++] = (uint16_t)u_data.kill_count; // 15
        units_file_buff[_index++] = (uint16_t)u_data.acid_spore_count; // 16
        units_file_buff[_index++] = (uint16_t)u_data.interceptor_count; // 17
        units_file_buff[_index++] = (uint16_t)u_data.spider_mine_count; // 18
        units_file_buff[_index++] = (uint16_t)u_data.ground_weapon_cooldown; // 19
        units_file_buff[_index++] = (uint16_t)u_data.air_weapon_cooldown; // 20
        units_file_buff[_index++] = (uint16_t)u_data.spell_cooldown; // 21
        units_file_buff[_index++] = (uint16_t)u_data.defense_matrix_points; // 22
        units_file_buff[_index++] = (uint16_t)u_data.build_type; // 23
        units_file_buff[_index++] = (uint16_t)u_data.tech_ID; // 24
        units_file_buff[_index++] = (uint16_t)u_data.upgrade_ID; // 25
        units_file_buff[_index++] = (uint16_t)u_data.remaining_build_time; // 26
        units_file_buff[_index++] = (uint16_t)u_data.build_unit_ID; // 27
        units_file_buff[_index++] = (uint16_t)u_data.target_ID; // 28
        units_file_buff[_index++] = (uint16_t)u_data.order_ID; // 29
        units_file_buff[_index++] = (uint16_t)u_data.order_target_ID; // 30
        units_file_buff[_index++] = (uint16_t)u_data.rally_unit_ID; // 31
        units_file_buff[_index++] = (uint16_t)u_data.addon_ID; // 32
        units_file_buff[_index++] = (uint16_t)u_data.nydus_exit_ID; // 33
        units_file_buff[_index++] = (uint16_t)u_data.transport_ID; // 34
        units_file_buff[_index++] = (uint16_t)u_data.carrier_ID; // 35
        units_file_buff[_index++] = (uint16_t)u_data.hatchery_ID; // 36
        units_file_buff[_index++] = (uint16_t)u_data.larva_count; // 37
        memcpy(&units_file_buff[_index], u_data.training_types, 5*sizeof(uint16_t)); // 38, 39, 40, 41, 42
        _index += 5; // 5 double bytes
        memcpy(&units_file_buff[_index], u_data.loaded_units, 8*sizeof(uint16_t)); // 43, 44, 45, 46, 47, 48, 49, 50
        _index += 8; // 8 double bytes
        units_file_buff[_index] |= (uint16_t)u_data.is_attacking; // 51
        units_file_buff[_index] |= (uint16_t)u_data.is_under_attack << 1; // 52
        units_file_buff[_index] |= (uint16_t)u_data.is_defense_matrixed << 2; // 53
        units_file_buff[_index] |= (uint16_t)u_data.is_ensnared << 3; // 54
        units_file_buff[_index] |= (uint16_t)u_data.is_irradiated << 4; // 55
        units_file_buff[_index] |= (uint16_t)u_data.is_maelstrommed << 5; // 56
        units_file_buff[_index] |= (uint16_t)u_data.is_plagued << 6; // 57
        units_file_buff[_index] |= (uint16_t)u_data.is_stasised << 7; // 58
        units_file_buff[_index] |= (uint16_t)u_data.is_stimmed << 8; // 59
        units_file_buff[_index] |= (uint16_t)u_data.is_parasited << 9; // 60
        units_file_buff[_index] |= (uint16_t)u_data.is_under_storm << 10; // 61
        units_file_buff[_index] |= (uint16_t)u_data.is_under_dark_swarm << 11; // 62
        units_file_buff[_index] |= (uint16_t)u_data.is_under_disruption_web << 12; // 63
        units_file_buff[_index] |= (uint16_t)u_data.is_burrowed << 13; // 64
        units_file_buff[_index] |= (uint16_t)u_data.is_carrying_gas << 14; // 65
        units_file_buff[_index] |= (uint16_t)u_data.is_carrying_minerals << 15; // 66
        ++_index;
        units_file_buff[_index] |= (uint16_t)u_data.is_powered; // 67
        units_file_buff[_index] |= (uint16_t)u_data.is_cloaked << 1; // 68
        units_file_buff[_index] |= (uint16_t)u_data.is_completed << 2; // 69
        units_file_buff[_index] |= (uint16_t)u_data.is_lifted << 3; // 70
        units_file_buff[_index] |= (uint16_t)u_data.is_detected << 4; // 71
        units_file_buff[_index] |= (uint16_t)u_data.is_halluc << 5; // 72
        units_file_buff[_index] |= (uint16_t)u_data.is_morphing << 6; // 73
        units_file_buff[_index] |= (uint16_t)u_data.is_sieged << 7; // 74
        units_file_buff[_index] |= (uint16_t)u_data.is_visible << 8;  // 75
        units_file_buff[_index] |= (uint16_t)u_data.is_being_healed << 9; // 76
        units_file_buff[_index] |= (uint16_t)u_data.is_being_constructed << 10; // 77
        units_file_buff[_index] |= (uint16_t)u_data.is_repairing << 11; // 78
        units_file_buff[_index] |= (uint16_t)u_data.just_stored << 12; // 79
        units_file_buff[_index] |= (uint16_t)u_data.just_changed_type << 13; // 80
        units_file_buff[_index] |= (uint16_t)u_data.just_removed << 14; // 81
        units_file_buff[_index] |= (uint16_t)(u_data.player == p1 ? 0 : 1) << 15; // 82
        ++_index;
        // 106 bytes per unit
    }

    void record_data(FILE *data_file) {
        clear_units_file_buff();
        int buff_index = 1;
        int bit_shift = 0;
        for (int p_index = 0; p_index < 2; ++p_index) {
            int type_ct = 0;
            const BWAPI::UnitType *types;
            // To keep the length of this section the consistent (number of terran units), trailing zeroes are kept
            // 6 bytes = 8 * 6 = 48 bits
            if (races[p_index] == BWAPI::Races::Zerg) {
                types = ZERG_TYPES;
                type_ct = ZERG_TYPE_CT;
            }
            else if (races[p_index] == BWAPI::Races::Terran) {
                types = TERRAN_TYPES;
                type_ct = TERRAN_TYPE_CT;
            }
            else {
                types = PROTOSS_TYPES;
                type_ct = PROTOSS_TYPE_CT;
            }
            BWAPI::Player p = (p_index == 0 ? p1 : p2);
            for (int bit = 0; bit < type_ct; ++bit) {
                if (p->hasUnitTypeRequirement(types[bit])) {
                    units_file_buff[buff_index] |= 1 << bit_shift;
                }
                ++bit_shift;
                if (bit_shift == 16) {
                    bit_shift = 0;
                    ++buff_index;
                }
            }
            buff_index = 4;
            bit_shift = 0;
        } // 48 * 2 + 16 bits = 112 bits = 14 bytes (7 double bytes) up to this point

        // informing the script how many units to read
        units_file_buff[_index++] = ID_to_data.size(); // 14 bytes (7 double bytes) up to this point

        for (auto const &ID_and_data : ID_to_data) {
            int unit_ID = ID_and_data.first;
            UnitData &unit_data = *(ID_and_data.second);
            unit_data.just_stored = false;
            unit_data.just_changed_type = false;
            unit_data.just_removed = false;
            if (unit_data.player == p1) {
                for (int i = 0; i < _p1_just_stored.size(); ++i) {
                    if (unit_ID == _p1_just_stored[i]) {
                        ID_to_data[unit_ID]->just_stored = true;
                    }
                }
                for (int i = 0; i < _p1_just_changed_type.size(); ++i) {
                    if (unit_ID == _p1_just_changed_type[i]) {
                        ID_to_data[unit_ID]->just_changed_type = true;
                    }
                }
                for (int i = 0; i < _p1_scheduled_for_removal.size(); ++i) {
                    if (unit_ID == _p1_scheduled_for_removal[i]) {
                        ID_to_data[unit_ID]->just_removed = true;
                    }
                }
            }
            else {
                for (int i = 0; i < _p2_just_stored.size(); ++i) {
                    if (unit_ID == _p2_just_stored[i]) {
                        ID_to_data[unit_ID]->just_stored = true;
                    }
                }
                for (int i = 0; i < _p2_just_changed_type.size(); ++i) {
                    if (unit_ID = _p2_just_changed_type[i]) {
                        ID_to_data[unit_ID]->just_changed_type = true;
                    }
                }
                for (int i = 0; i < _p2_scheduled_for_removal.size(); ++i) {
                    if (unit_ID == _p2_scheduled_for_removal[i]) {
                        ID_to_data[unit_ID]->just_removed = true;
                    }
                }
            } // 132 bytes per unit
            write_unit_to_buff(unit_data);
        }

        fwrite(units_file_buff, 2, _index, data_file);

        clear_just_stored_and_removed();
    }

    void on_frame_update() {
        store_queued();
        schedule_removals();
        update_units();
    }

    void queue_store(const BWAPI::Unit u) {
        bool unit_in_buff = (store_buff.find(u) != -1);
        if (!unit_in_buff) {
            store_buff.add(u);
        }
    }

    void queue_remove(const BWAPI::Unit u) {
        bool unit_in_buff = (remove_buff.find(u) != -1);
        if (!unit_in_buff) {
            remove_buff.add(u);
        }
    }

    void set_build_status(BWAPI::Unit u, BUILD_STATUS status) {
        int ID = u->getID();
        auto &unit_data = ID_to_data[ID];
        // unit_data->build_status = status;
    }

    const UnitArray &self_units() {
        return _p1_units;
    }

    const UnitArray &enemy_units() {
        return _p2_units;
    }

    const UnitData &data(BWAPI::Unit u) {return *ID_to_data[u->getID()];}
}