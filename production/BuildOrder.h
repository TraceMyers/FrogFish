#pragma once
#pragma message("including BuildOrder")

#include <BWAPI.h>
#include <fstream>
#include <thread>

namespace Production::BuildOrder {

    enum OVERLORD_INSERT_BAN {
        NONE,
        START,
        END
    };

    const int NO_PREDICTION = -1;

    struct Item {

        /* PUBLIC */
        public:

        enum ACTION {
            MAKE, 
            MORPH, 
            BUILD, 
            EXPAND,
            TECH, 
            UPGRADE, 
            CANCEL, 
            NONE
        };

        Item(
            ACTION act,
            BWAPI::UnitType u_type,
            BWAPI::TechType tch_type,
            BWAPI::UpgradeType up_type,
            int cancel_ID,
            OVERLORD_INSERT_BAN ovie_ban
        );

        ACTION action() const {
            return _action;
        }

        BWAPI::UnitType unit_type() const {
            return _unit_type;
        }

        BWAPI::TechType tech_type() const {
            return _tech_type;
        }

        BWAPI::UpgradeType upgrade_type() const {
            return _upgrade_type;
        }

        OVERLORD_INSERT_BAN overlord_insert_ban() const {
            return _overlord_insert_ban;
        }

        int cancel_ID() const {
            return _cancel_ID;
        }

        int mineral_cost() const {
            return _mineral_cost;
        }

        int gas_cost() const {
            return _gas_cost;
        }

        int larva_cost() const {
            return _larva_cost;
        }

        int supply_cost() const {
            return _supply_cost;
        }
        
        int ID() const {
            return _ID;
        }

        int seconds_until_make() const {
            return _seconds_until_make;
        }

        inline bool operator==(const Item& a) const {return a.ID() == this->ID();}

        /* PROTECTED */
        protected:

        ACTION _action;
        BWAPI::UnitType _unit_type;
        BWAPI::TechType _tech_type;
        BWAPI::UpgradeType _upgrade_type;
        OVERLORD_INSERT_BAN _overlord_insert_ban = OVERLORD_INSERT_BAN::NONE;
        int _cancel_ID;
        int _mineral_cost = 0;
        int _gas_cost = 0;
        int _larva_cost = 0;
        int _supply_cost = 0;
        int _ID = 0;
        int _seconds_until_make = 0;
    };

    void            on_frame_update();
    void            load(const char *_race, const char *build_name);
    void            push(
                        Item::ACTION _action,
                        BWAPI::UnitType _unit_type,
                        BWAPI::TechType _tech_type,
                        BWAPI::UpgradeType _upgrade_type,
                        int _cancel_ID,
                        OVERLORD_INSERT_BAN ban=OVERLORD_INSERT_BAN::NONE
                    );
    void            insert(
                        Item::ACTION _action,
                        BWAPI::UnitType _unit_type,
                        BWAPI::TechType _tech_type,
                        BWAPI::UpgradeType _upgrade_type,
                        int _cancel_ID,
                        int insert_index,
                        OVERLORD_INSERT_BAN ban=OVERLORD_INSERT_BAN::NONE
                    );
    void            insert_next(
                        Item::ACTION _action,
                        BWAPI::UnitType _unit_type,
                        BWAPI::TechType _tech_type,
                        BWAPI::UpgradeType _upgrade_type,
                        int _cancel_ID,
                        OVERLORD_INSERT_BAN ban=OVERLORD_INSERT_BAN::NONE
                    );
    int             current_index();
    const Item &    current_item();
    void            next();
    const Item &    get(int i);
    int             find_by_ID(int ID, bool from_current=true);
    void            move(int from, int to);
    void            remove(unsigned i);
    void            remove(const Item& item);
    const Item &    just_removed_get(unsigned i);
    int             just_removed_size();
    bool            can_insert_overlords();
    unsigned        size();
    bool            finished();
    void            set_seconds_until_make(int index, int seconds);
    void            print(unsigned int start=0);
    void            print_item(unsigned int i);

}