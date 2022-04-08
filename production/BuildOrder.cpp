#include "BuildOrder.h"
#include "../basic/References.h"
#include "../test/TestMessage.h"
#include <BWAPI.h>
#include <fstream>
#include <thread>
#include <cassert>

using namespace Basic;

// MAYBE: change Build Order to linked list implementation to work better with manipulation
// TODO: overlord ban turning off @ first ovie on 2_hatch_hydra


namespace Production::BuildOrder { 

    Item::Item(
        ACTION act,
        BWAPI::UnitType u_type,
        BWAPI::TechType tch_type,
        BWAPI::UpgradeType up_type,
        int cancel_ID,
        OVERLORD_INSERT_BAN ovie_ban
    ) :
        _action(act),
        _unit_type(u_type),
        _tech_type(tch_type),
        _upgrade_type(up_type),
        _cancel_ID(cancel_ID),
        _overlord_insert_ban(ovie_ban)
    {}

    namespace {

        int item_ID_counter = 0;

        struct InternalItem : public Item {

            InternalItem(
                ACTION act,
                BWAPI::UnitType u_type,
                BWAPI::TechType tch_type,
                BWAPI::UpgradeType up_type,
                int cancel_ID,
                OVERLORD_INSERT_BAN ovie_ban
            ) : 
                Item (act, u_type, tch_type, up_type, cancel_ID, ovie_ban)
            {
                set_mineral_cost();
                set_gas_cost();
                set_supply_cost();
                set_larva_cost();
                _ID = item_ID_counter++;
            }

            void set_seconds_until_make(int seconds) {
                _seconds_until_make = seconds;
            }

            void set_mineral_cost() {
                switch(_action) {
                    case Item::ACTION::TECH:
                        _mineral_cost = _tech_type.mineralPrice();
                        break;
                    case Item::ACTION::UPGRADE:
                        _mineral_cost = _upgrade_type.mineralPrice();
                        break;
                    case Item::ACTION::CANCEL:
                        if (_unit_type != BWAPI::UnitTypes::None) {
                            if (_unit_type.isBuilding()) {
                                _mineral_cost = (int)(-_unit_type.mineralPrice() * 0.75);
                            }
                            else {
                                _mineral_cost = -_unit_type.mineralPrice();
                            }
                        }
                        else if (_tech_type != BWAPI::TechTypes::None) {
                            _mineral_cost = -_tech_type.mineralPrice();
                        }
                        else {
                            _mineral_cost = -_upgrade_type.mineralPrice();
                        }
                        break;
                    default:
                        _mineral_cost = _unit_type.mineralPrice();
                }
            }

            void set_gas_cost() {
                switch(_action) {
                    case Item::ACTION::TECH:
                        _gas_cost = _tech_type.gasPrice();
                        break;
                    case Item::ACTION::UPGRADE:
                        _gas_cost = _upgrade_type.gasPrice();
                        break;
                    case Item::ACTION::CANCEL:
                        if (_unit_type != BWAPI::UnitTypes::None) {
                            if (_unit_type.isBuilding()) {
                                _gas_cost = (int)(-_unit_type.gasPrice() * 0.75);
                            }
                            else {
                                _gas_cost = -_unit_type.gasPrice();
                            }
                        }
                        else if (_tech_type != BWAPI::TechTypes::None) {
                            _gas_cost = -_tech_type.gasPrice();
                        }
                        else {
                            _gas_cost = -_upgrade_type.gasPrice();
                        }
                        break;
                    default:
                        _gas_cost = _unit_type.gasPrice();
                }
            }

            void set_supply_cost() {
                switch(_action) {
                    case Item::ACTION::BUILD:
                        _supply_cost = -2 - _unit_type.supplyProvided();
                        break;
                    case Item::ACTION::MORPH:
                        _supply_cost = 
                            _unit_type.supplyRequired() - _unit_type.whatBuilds().first.supplyRequired();
                        break;
                    case Item::ACTION::MAKE:
                        _supply_cost = _unit_type.supplyRequired() - _unit_type.supplyProvided();
                        break;
                    case Item::ACTION::CANCEL:
                        _supply_cost = _unit_type.supplyProvided() - _unit_type.supplyRequired()
                            + _unit_type.whatBuilds().first.supplyRequired();
                    default:
                        _supply_cost = 0;
                }
            }

            void set_larva_cost() {
                switch(_action) {
                    case Item::ACTION::MAKE:
                        _larva_cost = 1;
                        break;
                    default:
                        _larva_cost = 0;
                }
            }
        };
    }

    namespace {
        unsigned                    cur_index;
        std::string                 race;
        std::string                 name;
        std::vector<InternalItem>   items;
        std::vector<InternalItem>   items_just_removed;
        Item                        end_item(
                                        Item::ACTION::NONE, 
                                        BWAPI::UnitTypes::None,
                                        BWAPI::TechTypes::None,
                                        BWAPI::UpgradeTypes::None,
                                        -1,
                                        OVERLORD_INSERT_BAN::NONE
                                    );
        bool                        overlord_insert_ban_on;

        void _push(InternalItem item) {
            items.push_back(item);
        }

        void _insert(InternalItem item, int i) {
            items.insert(items.begin() + i, item);
        }

        void _insert_next(InternalItem item) {
            items.insert(items.begin() + cur_index, item);
        }

    }

    void on_frame_update() {
        items_just_removed.clear();
    }

    void load(const char *_race, const char *build_name) {
        overlord_insert_ban_on = false;
        cur_index = 0;
        items.clear();
        race = _race;
        name = build_name;
        std::ifstream in_file;
        // TODO: change this to relative path
        // bot should be in bwapi-data/AI
        in_file.open("C:\\ProgramData\\bwapi\\starcraft\\bwapi-data\\read\\BuildOrders.txt");
        if (!in_file) {
            printf("not able to read build order file\n");
        }
        std::string word;
        bool loaded = false;
        while (!loaded && in_file >> word) {
            if (word == race) {
                in_file >> word;
                if (word == build_name) {
                    while(in_file >> word) {
                        if (word[0] != '-') {
                            Item::ACTION _action;
                            BWAPI::UnitType _unit_type = BWAPI::UnitTypes::None;
                            BWAPI::TechType _tech_type = BWAPI::TechTypes::None;
                            BWAPI::UpgradeType _upgrade_type = BWAPI::UpgradeTypes::None;
                            int _count = 1;
                            int _cancel_ID = -1;
                            OVERLORD_INSERT_BAN ban;

                            in_file >> word;
                            word.pop_back();
                            if (word == "build") {
                                _action = Item::BUILD;
                            }
                            else if (word == "make") {
                                _action = Item::MAKE;
                            }
                            else if (word == "morph") {
                                _action = Item::MORPH;
                            }
                            else if (word == "tech") {
                                _action = Item::TECH;
                            }
                            else if (word == "upgrade") {
                                _action = Item::UPGRADE;
                            }
                            else if (word == "cancel") {
                                _action = Item::CANCEL;
                            }
                            else {
                                printf("BuildOrder error: _action = %s\n", word.c_str());
                            }

                            in_file >> word;
                            word.pop_back();
                            _count = std::stoi(word);

                            in_file >> word;
                            word.pop_back();
                            if (
                                _action == Item::BUILD
                                || _action == Item::MAKE
                                || _action == Item::MORPH
                                || _action == Item::CANCEL
                            ) {
                                for (int i = 0; i < Refs::Zerg::TYPE_CT; ++i) {
                                    if (word == Refs::Zerg::NAMES[i]) {
                                        _unit_type = Refs::Zerg::TYPES[i];
                                    }
                                }
                            }
                            else if (_action == Item::TECH || _action == Item::CANCEL) {
                                for (int i = 0; i < Refs::Zerg::TECH_CT; ++i) {
                                    if (word == Refs::Zerg::TECH_NAMES[i]) {
                                        _tech_type = Refs::Zerg::TECH_TYPES[i];
                                    }
                                }
                            }
                            else if (_action == Item::UPGRADE || _action == Item::CANCEL) {
                                for (int i = 0; i < Refs::Zerg::UPGRADE_CT; ++i) {
                                    if (word == Refs::Zerg::UPGRADE_NAMES[i]) {
                                        _upgrade_type = Refs::Zerg::UPGRADE_TYPES[i];
                                    }
                                }
                            }

                            in_file >> word;
                            word.pop_back();
                            if (word != "null") {
                                // loading from file, index == ID
                                _cancel_ID = std::stoi(word);
                            }

                            in_file >> word;
                            if (word == "null") {
                                ban = OVERLORD_INSERT_BAN::NONE;
                            }
                            else if (word  == "on") {
                                ban = OVERLORD_INSERT_BAN::START;
                            }
                            else {
                                ban = OVERLORD_INSERT_BAN::END;
                            }

                            for (int i = 0; i < _count; ++i) {
                                push(
                                    _action,
                                    _unit_type,
                                    _tech_type,
                                    _upgrade_type,
                                    _cancel_ID,
                                    ban
                                );
                            }
                        }
                        else {
                            loaded = true;
                            break;
                        }
                    }
                }
            }
        }
        if (items.size() > 0 && items[0].overlord_insert_ban() == OVERLORD_INSERT_BAN::START) {
            overlord_insert_ban_on = true;
        }
        in_file.close();
    }

    void push(
        Item::ACTION act,
        BWAPI::UnitType u_type,
        BWAPI::TechType tch_type,
        BWAPI::UpgradeType up_type,
        int cancel_ID,
        OVERLORD_INSERT_BAN ban
    ) {
        InternalItem item(
            act,
            u_type,
            tch_type,
            up_type,
            cancel_ID,
            ban
        );
        _push(item);
    }

    void insert(
        Item::ACTION act,
        BWAPI::UnitType u_type,
        BWAPI::TechType tch_type,
        BWAPI::UpgradeType up_type,
        int cancel_ID,
        int insert_index,
        OVERLORD_INSERT_BAN ban
    ) {
        InternalItem item(
            act,
            u_type,
            tch_type,
            up_type,
            cancel_ID,
            ban
        );
        _insert(item, insert_index);
    }

    void insert_next(
        Item::ACTION act,
        BWAPI::UnitType u_type,
        BWAPI::TechType tch_type,
        BWAPI::UpgradeType up_type,
        int cancel_ID,
        OVERLORD_INSERT_BAN ban
    ) {
        InternalItem item(
            act,
            u_type,
            tch_type,
            up_type,
            cancel_ID,
            ban
        );
        _insert_next(item);
    }
    
    int current_index() {
        return cur_index;
    }

    const Item &current_item() {
        if (cur_index < items.size()) {return items[cur_index];}
        return end_item;
    }

    void next() {
        unsigned items_size = items.size();
        if (cur_index < items_size) {
            ++cur_index;
        }
        if (cur_index < items_size) {
            auto& cur_item = items[cur_index];
            OVERLORD_INSERT_BAN ban = cur_item.overlord_insert_ban();
            if (ban == OVERLORD_INSERT_BAN::START) {
                overlord_insert_ban_on = true;
                DBGMSG("BuildOrder::next(): Overlord insert ban *ON*");
            }
            else if (ban == OVERLORD_INSERT_BAN::END) {
                overlord_insert_ban_on = false;
                DBGMSG("BuildOrder::next(): Overlord insert ban *OFF*");
            }
            DBGMSG("BuildOrder::next(): advancing to %d", cur_index);
            return;
        }
        DBGMSG("BuildOrder::next(): Build order reached the end.");
    }

    // unsafe - can cause read access error
    const Item &get(int i) {
        return items[i];
    }

    int find_by_ID(int ID, bool from_current) {
        int i = (from_current ? cur_index : 0);
        for ( ; i < items.size(); ++i) {
            if (items[i].ID() == ID) {
                return i;
            }
        }
        return -1;
    }

    void move(int from, int to) {
        InternalItem item = items[from];
        items.erase(items.begin() + from);
        items.insert(items.begin() + to, item);
    }

    // careful using while iterating; good for removing single items
    void remove(unsigned i) {
        InternalItem &item = items[i];
        items_just_removed.push_back(item);
        items.erase(items.begin() + i);
    }

    // careful using while iterating; good for removing a list of items collected for removal
    // post-loop
    void remove(const Item& item) {
        for (auto item_it = items.begin(); item_it < items.end(); ++item_it) {
            auto &_item = *item_it;
            if (_item == item) {
                items.erase(item_it);
                break;
            }
        }
    }
    
    // MAYBE: iterator functions for fast/easy removal? Might not need

    const Item &just_removed_get(unsigned i) {
        return items_just_removed[i];
    }

    int just_removed_size() {
        return items_just_removed.size();
    }

    bool can_insert_overlords() {
        return overlord_insert_ban_on;
    }

    unsigned size() {
        return items.size();
    }

    bool finished() {
        return (unsigned)cur_index >= items.size();
    }

    void set_seconds_until_make(int index, int seconds) {
        items[index].set_seconds_until_make(seconds);
    }

    void print(unsigned int start) {
        for (; start < items.size(); ++start) {
            print_item(start);
        }
    }
    
    void print_item(unsigned int i) {
        bool ovie_ban_on = false;
        const Item &item = items[i];
        std::cout << "index [" << i << "] ";
        std::cout << "\nID [" << item.ID() << "]: \n\t";
        switch(item.action()) {
            case Item::MAKE:
                std::cout << "Make    " << item.unit_type().c_str();
                break;
            case Item::MORPH:
                std::cout << "Morph   " << item.unit_type().c_str();
                break;
            case Item::BUILD:
                std::cout << "Build   " << item.unit_type().c_str();
                break;
            case Item::TECH:
                std::cout << "Tech    " << item.tech_type().c_str();
                break;
            case Item::UPGRADE:
                std::cout << "Upgrade " << item.upgrade_type().c_str();
                break;
            case Item::CANCEL:
                std::cout << "Cancel  ";
                if (item.unit_type() != BWAPI::UnitTypes::None) {
                    std::cout << item.unit_type();
                }
                else if (item.tech_type() != BWAPI::TechTypes::None) {
                    std::cout << item.tech_type();
                }
                else {
                    std::cout << item.upgrade_type();
                }
        }
        std::cout << "\n\tCancel ID: " << item.cancel_ID();
        std::cout << "\n\tMineral Cost: " << item.mineral_cost();
        std::cout << "\n\tGas Cost:     " << item.gas_cost();
        std::cout << "\n\tLarva Cost:   " << item.larva_cost();
        std::cout << "\n\tSupply Cost:  " << item.supply_cost();

        char* ban_types[] = {"START/ON", "END/OFF", "ON", "OFF"};
        int j;
        if (item.overlord_insert_ban() == OVERLORD_INSERT_BAN::START) {
            ovie_ban_on = true;
            j = 0;
        }
        else if (item.overlord_insert_ban() == OVERLORD_INSERT_BAN::END) {
            ovie_ban_on = false;
            j = 1;
        }
        else {
            if (ovie_ban_on) {
                j = 2;
            }
            else {
                j = 3;
            }
        }
        std::cout << "\n\tOverlord Ban: " << ban_types[j] << std::endl;
    }
}