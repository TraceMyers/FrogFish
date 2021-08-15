#include "DataCollector.h"
#include "utility/BWTimer.h"
#include "Economy.h"
#include "Units.h"
#include "TechAndUpgrades.h"
#include "Moddefs.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <string.h>

#define FILENAMEBUFFLEN 256
#define SOMEABSURDNUM 10000000
using namespace Filter;

BWTimer timer;
BWAPI::Player first = nullptr;
BWAPI::Player second = nullptr;
BWAPI::Player winner = nullptr;
BWAPI::Player players[2];
BWAPI::Race   races[2];
char file_path[FILENAMEBUFFLEN] = "D:\\projects\\frogfish\\data\\";
FILE *data_file;

// NLP model rough idea:
    // every unit type + upgrade + tech set is a unique word
    // existence of tech requirements constantly held in model memory
    // when units are made by self, their words get added to the text in order
    // when unique enemy units are seen, their words get added as soon as seen
    // unit deaths also each a word, but don't care about upgrades
    // output limited to those unique zerg words (only moving units + hatcheries)
    // uses last word + memory

bool set_data_file_path() {
    const int folder_path_char_len = strlen(file_path),
        file_name_remaining_chars = FILENAMEBUFFLEN - folder_path_char_len;
    char *file_path_ptr = file_path;
    char *file_name_ptr = file_path_ptr + folder_path_char_len;
    for (int i = folder_path_char_len; i < FILENAMEBUFFLEN; ++i) {
        file_path[i] = '\0';
    }

    int counter = 0;
    struct stat file_check_buff;
    char numstr_buff[11]; // 10 digits is more than plenty
    while (counter < SOMEABSURDNUM) {
        itoa(counter, numstr_buff, 10);
        strncpy(file_name_ptr, numstr_buff, file_name_remaining_chars);
        int digit_ct = strlen(numstr_buff);
        strncpy(file_name_ptr + digit_ct, ".star", file_name_remaining_chars - digit_ct);
        if (!stat(file_path, &file_check_buff) == 0) {
            return true;
        }
        ++counter;
    }
    return false;
}

void DataCollector::set_players() {
    auto player_set = Broodwar->getPlayers();

    for (auto &player : player_set) {
        if (player->getUnits().size() >= 5) {
            if (!player->isNeutral()) {
                if (first == nullptr) {
                    first = player;
                    players[0] = first;
                    races[0] = first->getRace();
                }
                else if (second == nullptr) {
                    second = player;
                    players[1] = second;
                    races[1] = second->getRace();
                }
            }
        }
    }
    printf("first player: %s\n", first->getName().c_str());
    printf("second player: %s\n", second->getName().c_str());
}

void DataCollector::write_init_data() {
    uint16_t init_data[49];
    memset(init_data, 0, 49 * 2); 
    puts(Broodwar->mapName().c_str());
    memcpy(init_data, Broodwar->mapName().c_str(), strlen(Broodwar->mapName().c_str()));
    memcpy(init_data + 15, players[0]->getName().c_str(), strlen(players[0]->getName().c_str()));
    memcpy(init_data + 30, players[1]->getName().c_str(), strlen(players[1]->getName().c_str()));
    init_data[45] = Broodwar->mapWidth();
    init_data[46] = Broodwar->mapHeight();
    init_data[47] = races[0].getID();
    init_data[48] = races[1].getID();
    fwrite(init_data, 1, 49 * 2, data_file);
}

void DataCollector::write_end_data() {
    char end_data[32];
    *((uint16_t*)end_data) = Moddefs::WINNER;
    const char *winner_name = winner->getName().c_str();
    int name_len = strlen(winner_name);
    int copy_ct = (30 > name_len ? name_len : 30);
    strncpy(end_data + 2, winner->getName().c_str(), copy_ct);
    fwrite(end_data, 1, 32, data_file);
    fclose(data_file);
    data_file = nullptr;
}

void DataCollector::onStart() {
    if (set_data_file_path()) {
        printf("Writing game data to %s", file_path);
        data_file = fopen(file_path, "wb");
    }
    else {
        fprintf(stderr, "DataCollector::onStart(): set_data_file_path() returned false\n");
        exit(1);
    }
    Broodwar->setLocalSpeed(9);
    onStart_alloc_debug_console();
    set_players();
    write_init_data();
    Units::init(players);
    Economy::init(players);
    TechAndUpgrades::init(players);
    timer.start(4, 0);
}

void DataCollector::onFrame() {
	if (Broodwar->isPaused() || winner != nullptr) {return;}

    Units::on_frame_update();

    timer.on_frame_update();
    if (timer.is_stopped()) {
        Units::record_data(data_file);
        Economy::record_data(data_file);
        TechAndUpgrades::record_data(data_file);
        timer.restart();
    }
    if (players[0]->getUnits().size() == 0) {
        printf("%s has no units, so they must be the loser\n", players[0]->getName().c_str());
        winner = players[1];
        write_end_data();
    }
    else if (players[1]->getUnits().size() == 0) {
        printf("%s has no units, so they must be the loser\n", players[1]->getName().c_str());
        winner = players[0];
        write_end_data();
    }
}

void DataCollector::onSendText(std::string text) {
    Broodwar->sendText("%s", text.c_str());
}

void DataCollector::onReceiveText(BWAPI::Player player, std::string text) {
    Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void DataCollector::onPlayerLeft(BWAPI::Player player) {
    Broodwar->sendText("Farewell %s!", player->getName().c_str());
    if (winner == nullptr && (player == players[0] || player == players[1])) {
        printf("%s left the game, so they must be the loser\n", player->getName().c_str());
        winner = (player == players[0] ? players[1] : players[0]);
        write_end_data();
    }
}

void DataCollector::onNukeDetect(BWAPI::Position target) {
    // pass
}

void DataCollector::onUnitDiscover(BWAPI::Unit unit) {
    if (unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
        // the_map.OnGeyserNoticed(unit);
    }
    else {
        Units::queue_store(unit);
    }
    // BWEB::Map::onUnitDiscover(unit);
}

void DataCollector::onUnitEvade(BWAPI::Unit unit) {
    // pass
}

void DataCollector::onUnitShow(BWAPI::Unit unit) {
    // pass
}

void DataCollector::onUnitHide(BWAPI::Unit unit) {
    // pass
}

void DataCollector::onUnitCreate(BWAPI::Unit unit) {
    Units::queue_store(unit);
    if (unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
        Units::queue_remove(unit);
        // the_map.OnGeyserNoticed(unit);
    }
}

void DataCollector::onUnitDestroy(BWAPI::Unit unit) {
    Units::queue_remove(unit);
    if (unit->getType().isMineralField()) {/*the_map.OnMineralDestroyed(unit);*/}
    else if (unit->getType().isSpecialBuilding()) {/*the_map.OnStaticBuildingDestroyed(unit);*/}
    // BWEB::Map::onUnitDestroy(unit);
}

void DataCollector::onUnitMorph(BWAPI::Unit unit) {
    if (unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
        Units::queue_remove(unit);
        // the_map.OnGeyserNoticed(unit);
    }
    else {
        Units::queue_store(unit);
    }
    // BWEB::Map::onUnitMorph(unit);
}

void DataCollector::onUnitRenegade(BWAPI::Unit unit) {
    // pass
}

void DataCollector::onSaveGame(std::string gameName) {
    // pass
}

void DataCollector::onUnitComplete(Unit unit) {
    // pass
}

void DataCollector::onEnd(bool isWinner) {
    if (data_file != nullptr) {
        fclose(data_file);
    }
    FreeConsole();
}
