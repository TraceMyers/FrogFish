#include "FrogFish.h"
#include "utility/BWTimer.h"
#include "utility/DebugDraw.h"
#include "basic/Units.h"
#include "basic/Bases.h"
#include "basic/Tech.h"
#include "production/BuildGraph.h"
#include "production/BuildOrder.h"
#include "production/Economy.h"
#include "production/MakeUnits.h"
#include "production/Construction.h"
#include "movement/Move.h"
#include "test/TestMove.h"
#include "test/TestMessage.h"
#include "control/Workers.h"
#include "production/GetTech.h"
#include <BWAPI.h>
#include <iostream>
#include <string>
#include <vector>

using namespace Filter;
/*
Name ideas:
Frankie Fishface

*/
// TODO:
//  - using terran test_make_then_move build order causes bug where too many construction plans
//      are added
//  - unsupervised learning of build order types
//  - NLP of build order?
//  - fish puns 
//  - Arty the FrogFish animation in upper left corner
//  - Commander pipeline
//  

BWTimer timer;

void FrogFish::onStart() {
    Broodwar->sendText("Hello Sailor!");
    Broodwar->enableFlag(Flag::UserInput);
    Broodwar->setLocalSpeed(10);
    Broodwar->setCommandOptimizationLevel(2);
    onStart_alloc_debug_console();
    //onStart_send_workers_to_mine();
    onStart_init_bwem_and_bweb();
    Basic::Units::init();
    Basic::Bases::init();
    Production::BuildGraph::init();
    Production::Economy::init();
    Production::Construction::init();
    Production::BuildOrder::load("protoss", "2_hatch_hydra");
    Production::BuildOrder::print();
    Production::MakeUnits::init();
    Test::Message::init(15);
    timer.start(100,0);
}

void FrogFish::onFrame() {
	if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self()) {return;}

    // 1. update basic data that everything else references
    // clear base storage stuff
    Basic::Units::on_frame_update();
    Basic::Bases::on_frame_update();
    Basic::Tech::on_frame_update();

    // 2. update production data
    Production::BuildGraph::on_frame_update();
    Production::Economy::on_frame_update();
    Production::MakeUnits::on_frame_update();
    Production::Construction::on_frame_update();
    Production::GetTech::on_frame_update();

	Movement::Move::on_frame_update();

    // 3. issue commands that require newly assigned lists
    Control::Workers::send_idle_workers_to_mine();
    Control::Workers::send_mineral_workers_to_gas();

    Utility::DebugDraw::draw_units();
    Utility::DebugDraw::draw_bases();
    Utility::DebugDraw::draw_build_nodes();
    Utility::DebugDraw::draw_all_movement_paths();

    Test::Message::on_frame_update();

    // if a module is going to check whether or not an item has been removed this frame
    // it needs to check before this update
    Production::BuildOrder::on_frame_update();
    if (Production::BuildOrder::finished()) {
        //Test::Move::move_group_around_the_map();
    }
    timer.on_frame_update();
    if (timer.is_stopped()) {
        // timer.restart();
        //timer.start(0,1);
    }
}

void FrogFish::onSendText(std::string text) {
    Broodwar->sendText("%s", text.c_str());
}

void FrogFish::onReceiveText(BWAPI::Player player, std::string text) {
    Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void FrogFish::onPlayerLeft(BWAPI::Player player) {
    Broodwar->sendText("Farewell %s!", player->getName().c_str());
}

void FrogFish::onNukeDetect(BWAPI::Position target) {
    if (target) {
        Broodwar << "Nuclear Launch Detected at " << target << std::endl;
    }
    else {
        Broodwar->sendText("Where's the nuke?");
    }
}

void FrogFish::onUnitDiscover(BWAPI::Unit unit) {
    if (unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
        the_map.OnGeyserNoticed(unit);
    }
    else {
        Basic::Units::queue_store(unit);
    }
    BWEB::Map::onUnitDiscover(unit);
}

void FrogFish::onUnitEvade(BWAPI::Unit unit) {
    // pass
}

void FrogFish::onUnitShow(BWAPI::Unit unit) {
    // pass
}

void FrogFish::onUnitHide(BWAPI::Unit unit) {
    // pass
}

void FrogFish::onUnitCreate(BWAPI::Unit unit) {
    Basic::Units::queue_store(unit);
    if (unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
        Basic::Units::queue_remove(unit);
        the_map.OnGeyserNoticed(unit);
    }
}

void FrogFish::onUnitDestroy(BWAPI::Unit unit) {
    Basic::Units::queue_remove(unit);
    if (unit->getType().isMineralField()) {the_map.OnMineralDestroyed(unit);}
    else if (unit->getType().isSpecialBuilding()) {the_map.OnStaticBuildingDestroyed(unit);}
    BWEB::Map::onUnitDestroy(unit);
}

void FrogFish::onUnitMorph(BWAPI::Unit unit) {
    if (unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
        Basic::Units::queue_remove(unit);
        the_map.OnGeyserNoticed(unit);
    }
    else {
        Basic::Units::queue_store(unit);
    }
    BWEB::Map::onUnitMorph(unit);
}

void FrogFish::onUnitRenegade(BWAPI::Unit unit) {
    // pass
}

void FrogFish::onSaveGame(std::string gameName) {
    // pass
}

void FrogFish::onUnitComplete(Unit unit) {
    // broadcast for structure usage
}

void FrogFish::onEnd(bool isWinner) {
    FreeConsole();
    // free unit data
    // free base data
}