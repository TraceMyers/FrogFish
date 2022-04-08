#include "Units.h"
#include "../FrogFish.h"
#include <map>

// TODO: 
//      - CMDR namespace that unreadies units on command sent
//      - store Player data
//      - added extra dereference right? Should probably eventually change it back

using namespace Basic;
using namespace Basic::Refs;

namespace Basic::Units {

    namespace {

        std::vector<int> self_IDs;
        std::vector<int> enemy_IDs;

        UnitArray store_buff;
        UnitArray remove_buff;

        UnitArray _self_units;
        UnitArray _self_just_stored;
        UnitArray _self_scheduled_for_removal;
        UnitArray _self_just_changed_type;

        UnitArray _enemy_units;
        UnitArray _enemy_just_stored;
        UnitArray _enemy_scheduled_for_removal;
        UnitArray _enemy_just_changed_type;
        UnitArray _enemy_just_moved;

        std::map<int, UnitData *> ID_to_data;

        BWAPI::Player self;
        BWAPI::Player enemy;

        void store_queued() {
            BWAPI::Unit u;
            for (int i = 0; i < store_buff.size(); ++i) {
                u = store_buff[i];
                if (u->getPlayer() == self) {
                    int ID = u->getID();
                    if (std::find(self_IDs.begin(), self_IDs.end(), ID) == self_IDs.end()) {
                        self_IDs.push_back(ID);
                        _self_units.add(u);
                        ID_to_data[ID] = new UnitData(u);
                        _self_just_stored.add(u);
                    }
                }
                else if (u->getPlayer() == enemy) {
                    int ID = u->getID();
                    if (std::find(enemy_IDs.begin(), enemy_IDs.end(), ID) == enemy_IDs.end()) {
                        enemy_IDs.push_back(ID);
                        _enemy_units.add(u);
                        ID_to_data[ID] = new UnitData(u);
                        _enemy_just_stored.add(u);
                    }
                }
            }
            store_buff.clear();
        }

        void schedule_self_remove(BWAPI::Unit u) {
            int ID = u->getID();
            auto ID_it = std::find(self_IDs.begin(), self_IDs.end(), ID);
            if (ID_it != self_IDs.end()) {
                self_IDs.erase(ID_it);
                _self_units.remove(u);
                _self_scheduled_for_removal.add(u);
            }
        }

        void schedule_enemy_remove(BWAPI::Unit u) {
            int ID = u->getID();
            auto ID_it = std::find(enemy_IDs.begin(), enemy_IDs.end(), ID);
            if (ID_it != enemy_IDs.end()) {
                enemy_IDs.erase(ID_it);
                _enemy_units.remove(u);
                _enemy_scheduled_for_removal.add(u);
            }
        }

        void schedule_removals() {
            for (int i = 0; i < remove_buff.size(); ++i) {
                BWAPI::Unit u = remove_buff[i];
                if (u->getPlayer() == self) {
                    schedule_self_remove(u);        
                }
                else if (u->getPlayer() == enemy) {
                    schedule_enemy_remove(u);        
                }
            }
            remove_buff.clear();
        }

        void update_unit(const BWAPI::Unit &u, UnitData &u_data) {
            u_data.type = u->getType();
            u_data.hp = u->getHitPoints();
            u_data.max_hp = u_data.type.maxHitPoints();
            u_data.shields = u->getShields();
            u_data.energy = u->getEnergy();
            u_data.pos = u->getPosition();
            u_data.tilepos = u->getTilePosition();
            u_data.velocity[0] = u->getVelocityX();
            u_data.velocity[1] = u->getVelocityY();
            u_data.angle = u->getAngle();
            u_data.missing = false;
            u_data.lifted = u->isLifted();
            u_data.u_type = get_utype(u);
            u_data.ground_weapon_cd = u->getGroundWeaponCooldown();
            u_data.air_weapon_cd = u->getAirWeaponCooldown();
            u_data.spell_cd = u->getSpellCooldown();
            u_data.irradiate_timer = u->getIrradiateTimer();
            u_data.lockdown_timer = u->getLockdownTimer();
            u_data.plague_timer = u->getPlagueTimer();
            u_data.stasis_timer = u->getStasisTimer();
            u_data.transport = u->getTransport();
            u_data.loaded_units = u->getLoadedUnits();
            u_data.teching = u->getTech();
            u_data.upgrading = u->getUpgrade();
            u_data.velocity[0] = u->getVelocityX(); 
            u_data.velocity[1] = u->getVelocityY();
            u_data.cmd_timer.on_frame_update();
            if (u_data.cmd_timer.is_stopped()) {
                u_data.cmd_ready = true;
            }
        }

        void update_self_units() {
            for (int i = 0; i < _self_units.size(); ++i) {
                const BWAPI::Unit &u = _self_units[i];
                UnitData &u_data = *ID_to_data[u->getID()];
                if (u_data.type != u->getType()) {
                    _self_just_changed_type.add(u);
                    // TODO: test number, diff per morph?
                    set_cmd_delay(u, 16);
                    set_utask(u, Basic::Refs::IDLE);
                }
                update_unit(u, u_data);
            }
        }

        void update_enemy_units() { 
            for (int i = 0; i < _enemy_units.size(); ++i) {
                const BWAPI::Unit &u = _enemy_units[i];
                UnitData &u_data = *ID_to_data[u->getID()];
                if (u->isVisible()) {
                    if (u_data.type != u->getType()) {
                        if (u->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
                            schedule_enemy_remove(u);
                            continue;
                        }
                        else {
                            _enemy_just_changed_type.add(u);
                        }
                    }
                    else if (u_data.pos != u->getPosition()) {
                        _enemy_just_moved.add(u);
                    }
                    update_unit(u, u_data);
                }
                else if (
                    !u_data.missing 
                    && !u->isVisible()
                    && Broodwar->isVisible(u_data.tilepos)
                ) {
                    if (u_data.u_type == UTYPE::STRUCT) {
                        // TODO: might not want to assume lifted w/ terran structure, but need
                        //       code to account for possibility
                        schedule_enemy_remove(u);
                    }
                    else {
                        _enemy_just_moved.add(u);
                    }
                    u_data.missing = true; 
                }
            } 
        }

        void clear_just_stored_and_removed() {
            _self_just_stored.clear();
            for (int i = 0; i < _self_scheduled_for_removal.size(); ++i) {
                int ID = _self_scheduled_for_removal[i]->getID();
                delete ID_to_data[ID];
                ID_to_data.erase(ID);
            }
            _self_scheduled_for_removal.clear();
            _self_just_changed_type.clear();
            _enemy_just_stored.clear();
            for (int i = 0; i < _enemy_scheduled_for_removal.size(); ++i) {
                int ID = _enemy_scheduled_for_removal[i]->getID();
                delete ID_to_data[ID];
                ID_to_data.erase(ID);
            }
            _enemy_scheduled_for_removal.clear();
            _enemy_just_changed_type.clear();
            _enemy_just_moved.clear();
        }

    }

    void init() {
        self = Broodwar->self();
        enemy = Broodwar->enemy();
    }

    void on_frame_update() {
        clear_just_stored_and_removed();
        store_queued();
        schedule_removals();
        update_self_units();
        update_enemy_units();
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

    // only for self units
    void set_utask(BWAPI::Unit u, UTASK task) {
        int ID = u->getID();
        auto &unit_data = ID_to_data[ID];
        unit_data->u_task = task;
    }

    void set_cmd_delay(BWAPI::Unit u, int cmd_delay_frames) {
        int ID = u->getID();
        auto &unit_data = ID_to_data[ID];
        unit_data->cmd_ready = false;
        unit_data->cmd_timer.start(0, cmd_delay_frames);

    }

    void set_build_status(BWAPI::Unit u, BUILD_STATUS status) {
        int ID = u->getID();
        auto &unit_data = ID_to_data[ID];
        unit_data->build_status = status;
    }

    const UnitArray &self_units() {
        return _self_units;
    }

    const UnitArray &enemy_units() {
        return _enemy_units;
    }

    const UnitData &data(BWAPI::Unit u) {return *ID_to_data[u->getID()];}

    const UnitArray &self_just_stored() {return _self_just_stored;}

    const UnitArray &self_just_destroyed() {return _self_scheduled_for_removal;}

    const UnitArray &self_just_changed_type() {return _self_just_changed_type;}

    const UnitArray &enemy_just_stored() {return _enemy_just_stored;}

    const UnitArray &enemy_just_destroyed() {return _enemy_scheduled_for_removal;}

    const UnitArray &enemy_just_changed_type() {return _enemy_just_changed_type;}

    const UnitArray &enemy_just_moved() {return _enemy_just_moved;}
}