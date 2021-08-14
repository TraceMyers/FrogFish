#pragma once
#include "BWAPI.h"

namespace Units {
    const int ZERG_TYPE_CT = 30;
    const int TERRAN_TYPE_CT = 35;
    const int PROTOSS_TYPE_CT = 32;

    void init(const BWAPI::Player *players);
    void record_data(FILE *data_file);
    void queue_store(const BWAPI::Unit u);
    void queue_remove(const BWAPI::Unit u);
    void on_frame_update();

    const BWAPI::UnitType ZERG_TYPES[ZERG_TYPE_CT] = {
        BWAPI::UnitTypes::Zerg_Drone,
        BWAPI::UnitTypes::Zerg_Zergling,
        BWAPI::UnitTypes::Zerg_Hydralisk,
        BWAPI::UnitTypes::Zerg_Lurker,
        BWAPI::UnitTypes::Zerg_Ultralisk,
        BWAPI::UnitTypes::Zerg_Defiler,
        BWAPI::UnitTypes::Zerg_Overlord,
        BWAPI::UnitTypes::Zerg_Mutalisk,
        BWAPI::UnitTypes::Zerg_Scourge,
        BWAPI::UnitTypes::Zerg_Queen,
        BWAPI::UnitTypes::Zerg_Guardian,
        BWAPI::UnitTypes::Zerg_Devourer,
        BWAPI::UnitTypes::Zerg_Hatchery,
        BWAPI::UnitTypes::Zerg_Creep_Colony,
        BWAPI::UnitTypes::Zerg_Sunken_Colony,
        BWAPI::UnitTypes::Zerg_Spore_Colony,
        BWAPI::UnitTypes::Zerg_Extractor,
        BWAPI::UnitTypes::Zerg_Spawning_Pool,
        BWAPI::UnitTypes::Zerg_Evolution_Chamber,
        BWAPI::UnitTypes::Zerg_Hydralisk_Den,
        BWAPI::UnitTypes::Zerg_Lair,
        BWAPI::UnitTypes::Zerg_Spire,
        BWAPI::UnitTypes::Zerg_Queens_Nest,
        BWAPI::UnitTypes::Zerg_Hive,
        BWAPI::UnitTypes::Zerg_Greater_Spire,
        BWAPI::UnitTypes::Zerg_Nydus_Canal,
        BWAPI::UnitTypes::Zerg_Ultralisk_Cavern,
        BWAPI::UnitTypes::Zerg_Defiler_Mound,
        BWAPI::UnitTypes::Zerg_Egg,
        BWAPI::UnitTypes::Zerg_Lurker_Egg
    };

    const BWAPI::UnitType TERRAN_TYPES[TERRAN_TYPE_CT] = {
        BWAPI::UnitTypes::Terran_Academy,
        BWAPI::UnitTypes::Terran_Armory,
        BWAPI::UnitTypes::Terran_Barracks,
        BWAPI::UnitTypes::Terran_Battlecruiser,
        BWAPI::UnitTypes::Terran_Bunker,
        BWAPI::UnitTypes::Terran_Command_Center,
        BWAPI::UnitTypes::Terran_Comsat_Station,
        BWAPI::UnitTypes::Terran_Control_Tower,
        BWAPI::UnitTypes::Terran_Covert_Ops,
        BWAPI::UnitTypes::Terran_Dropship,
        BWAPI::UnitTypes::Terran_Engineering_Bay,
        BWAPI::UnitTypes::Terran_Factory,
        BWAPI::UnitTypes::Terran_Firebat,
        BWAPI::UnitTypes::Terran_Ghost,
        BWAPI::UnitTypes::Terran_Goliath,
        BWAPI::UnitTypes::Terran_Machine_Shop,
        BWAPI::UnitTypes::Terran_Marine,
        BWAPI::UnitTypes::Terran_Medic,
        BWAPI::UnitTypes::Terran_Missile_Turret,
        BWAPI::UnitTypes::Terran_Nuclear_Missile,
        BWAPI::UnitTypes::Terran_Nuclear_Silo,
        BWAPI::UnitTypes::Terran_Physics_Lab,
        BWAPI::UnitTypes::Terran_Refinery,
        BWAPI::UnitTypes::Terran_Science_Facility,
        BWAPI::UnitTypes::Terran_Science_Vessel,
        BWAPI::UnitTypes::Terran_SCV,
        BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode,
        BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode,
        BWAPI::UnitTypes::Terran_Starport,
        BWAPI::UnitTypes::Terran_Supply_Depot,
        BWAPI::UnitTypes::Terran_Valkyrie,
        BWAPI::UnitTypes::Terran_Vulture,
        BWAPI::UnitTypes::Terran_Vulture_Spider_Mine,
        BWAPI::UnitTypes::Terran_Wraith
    };

    const BWAPI::UnitType PROTOSS_TYPES[PROTOSS_TYPE_CT] = {
        BWAPI::UnitTypes::Protoss_Arbiter,
        BWAPI::UnitTypes::Protoss_Arbiter_Tribunal,
        BWAPI::UnitTypes::Protoss_Archon,
        BWAPI::UnitTypes::Protoss_Assimilator,
        BWAPI::UnitTypes::Protoss_Carrier,
        BWAPI::UnitTypes::Protoss_Citadel_of_Adun,
        BWAPI::UnitTypes::Protoss_Corsair,
        BWAPI::UnitTypes::Protoss_Cybernetics_Core,
        BWAPI::UnitTypes::Protoss_Dark_Archon,
        BWAPI::UnitTypes::Protoss_Dark_Templar,
        BWAPI::UnitTypes::Protoss_Dragoon,
        BWAPI::UnitTypes::Protoss_Fleet_Beacon,
        BWAPI::UnitTypes::Protoss_Forge,
        BWAPI::UnitTypes::Protoss_Gateway,
        BWAPI::UnitTypes::Protoss_High_Templar,
        BWAPI::UnitTypes::Protoss_Interceptor,
        BWAPI::UnitTypes::Protoss_Nexus,
        BWAPI::UnitTypes::Protoss_Observatory,
        BWAPI::UnitTypes::Protoss_Observer,
        BWAPI::UnitTypes::Protoss_Photon_Cannon,
        BWAPI::UnitTypes::Protoss_Probe,
        BWAPI::UnitTypes::Protoss_Pylon,
        BWAPI::UnitTypes::Protoss_Reaver,
        BWAPI::UnitTypes::Protoss_Robotics_Facility,
        BWAPI::UnitTypes::Protoss_Robotics_Support_Bay,
        BWAPI::UnitTypes::Protoss_Scarab,
        BWAPI::UnitTypes::Protoss_Scout,
        BWAPI::UnitTypes::Protoss_Shield_Battery,
        BWAPI::UnitTypes::Protoss_Shuttle,
        BWAPI::UnitTypes::Protoss_Stargate,
        BWAPI::UnitTypes::Protoss_Templar_Archives,
        BWAPI::UnitTypes::Protoss_Zealot
    };
}