#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>
#include <BWEB/BWEB.h>
#include <windows.h>
#include <stdio.h>
#include <assert.h>

namespace {auto &the_map = BWEM::Map::Instance();}

using namespace BWAPI;
using namespace Filter;

class FrogFish : public AIModule {

private:

    void onStart_alloc_debug_console() {
        FILE *pFile = nullptr;
        AllocConsole();
        freopen_s(&pFile, "CONOUT$", "w", stdout);
    }

    void onStart_send_workers_to_mine() {
        for (Unit u : Broodwar->self()->getUnits()) {
            if (u->getType().isWorker()) {
                u->gather(u->getClosestUnit(IsMineralField));
            }
        }
    }

    void onStart_init_bwem_and_bweb() {
        try {
            printf("Map init\n");
            Broodwar << "Map init..." << std::endl;
            the_map.Initialize();
            the_map.EnableAutomaticPathAnalysis();
            bool starting_locs_ok = the_map.FindBasesForStartingLocations();
            assert(starting_locs_ok);
        }
        catch (const std::exception e) {
            Broodwar << "EXCEPTION: " << e.what() << std::endl;
        }
        BWEB::Map::onStart();
        BWEB::Blocks::findBlocks();
    }

public:
    // Virtual functions for callbacks, leave these as they are.
    virtual void onStart();
    virtual void onEnd(bool isWinner);
    virtual void onFrame();
    virtual void onSendText(std::string text);
    virtual void onReceiveText(Player player, std::string text);
    virtual void onPlayerLeft(Player player);
    virtual void onNukeDetect(Position target);
    virtual void onUnitDiscover(Unit unit);
    virtual void onUnitEvade(Unit unit);
    virtual void onUnitShow(Unit unit);
    virtual void onUnitHide(Unit unit);
    virtual void onUnitCreate(Unit unit);
    virtual void onUnitDestroy(Unit unit);
    virtual void onUnitMorph(Unit unit);
    virtual void onUnitRenegade(Unit unit);
    virtual void onSaveGame(std::string gameName);
    virtual void onUnitComplete(Unit unit);
    // Everything below this line is safe to modify.
};

