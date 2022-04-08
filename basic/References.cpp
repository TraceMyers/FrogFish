#include "References.h"

namespace Basic::Refs {


    namespace Zerg {

        const int TYPE_CT = 28;
        const int TECH_CT = 9;
        const int UPGRADE_CT = 16;

        const char *NAMES[28] = {
            "drone",
            "zergling",
            "hydralisk",
            "lurker",
            "ultralisk",
            "defiler",
            "overlord",
            "mutalisk",
            "scourge",
            "queen",
            "guardian",
            "devourer",
            "hatchery",
            "creep_colony",
            "sunken_colony",
            "spore_colony",
            "extractor",
            "spawning_pool",
            "evolution_chamber",
            "hydralisk_den",
            "lair",
            "spire",
            "queens_nest",
            "hive",
            "greater_spire",
            "nydus_canal",
            "ultralisk_cavern",
            "defiler_mound"
        };

        const BWAPI::UnitType TYPES[28] = {
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
            BWAPI::UnitTypes::Zerg_Defiler_Mound
        };

        const BWAPI::UnitType UNIT_REQ[28] = {
            BWAPI::UnitTypes::None,
            BWAPI::UnitTypes::Zerg_Spawning_Pool,
            BWAPI::UnitTypes::Zerg_Hydralisk_Den,
            BWAPI::UnitTypes::Zerg_Hydralisk_Den,
            BWAPI::UnitTypes::Zerg_Ultralisk_Cavern,
            BWAPI::UnitTypes::Zerg_Defiler_Mound,
            BWAPI::UnitTypes::None,
            BWAPI::UnitTypes::Zerg_Spire,
            BWAPI::UnitTypes::Zerg_Spire,
            BWAPI::UnitTypes::Zerg_Queens_Nest,
            BWAPI::UnitTypes::Zerg_Greater_Spire,
            BWAPI::UnitTypes::Zerg_Greater_Spire,
            BWAPI::UnitTypes::None,
            BWAPI::UnitTypes::None,
            BWAPI::UnitTypes::Zerg_Spawning_Pool,
            BWAPI::UnitTypes::Zerg_Spawning_Pool,
            BWAPI::UnitTypes::None,
            BWAPI::UnitTypes::None,
            BWAPI::UnitTypes::None,
            BWAPI::UnitTypes::Zerg_Spawning_Pool,
            BWAPI::UnitTypes::Zerg_Hatchery,
            BWAPI::UnitTypes::Zerg_Lair,
            BWAPI::UnitTypes::Zerg_Lair,
            BWAPI::UnitTypes::Zerg_Queens_Nest,
            BWAPI::UnitTypes::Zerg_Hive,
            BWAPI::UnitTypes::Zerg_Hive,
            BWAPI::UnitTypes::Zerg_Hive,
            BWAPI::UnitTypes::Zerg_Hive
        };
        
        const char* TECH_NAMES[9] = {
            "lurker_aspect",
            "parasite",
            "plague",
            "burrow",
            "consume",
            "ensnare",
            "infestation",
            "dark_swarm",
            "spawn_broodlings"
        };

        const BWAPI::TechType TECH_TYPES[9] = {
            BWAPI::TechTypes::Lurker_Aspect,
            BWAPI::TechTypes::Parasite,
            BWAPI::TechTypes::Plague,
            BWAPI::TechTypes::Burrowing,
            BWAPI::TechTypes::Consume,
            BWAPI::TechTypes::Ensnare,
            BWAPI::TechTypes::Infestation,
            BWAPI::TechTypes::Dark_Swarm,
            BWAPI::TechTypes::Spawn_Broodlings
        };

        const 

        const char* UPGRADE_NAMES[16] = {
            "adrenal_glands",
            "anabolic_synthesis",
            "antennae",
            "chitinous_plating",
            "gamete_meiosis",
            "grooved_spines",
            "metabolic_boost",
            "metasynaptic_node",
            "muscular_augments",
            "pneumatized_carapace",
            "ventral_sacs",
            "zerg_carapace",
            "zerg_flyer_attacks",
            "zerg_flyer_carapace",
            "zerg_melee_attacks",
            "zerg_missile_attacks"
        };

        const BWAPI::UpgradeType UPGRADE_TYPES[16] = {
            BWAPI::UpgradeTypes::Adrenal_Glands,
            BWAPI::UpgradeTypes::Anabolic_Synthesis,
            BWAPI::UpgradeTypes::Antennae,
            BWAPI::UpgradeTypes::Chitinous_Plating,
            BWAPI::UpgradeTypes::Gamete_Meiosis,
            BWAPI::UpgradeTypes::Grooved_Spines,
            BWAPI::UpgradeTypes::Metabolic_Boost,
            BWAPI::UpgradeTypes::Metasynaptic_Node,
            BWAPI::UpgradeTypes::Muscular_Augments,
            BWAPI::UpgradeTypes::Pneumatized_Carapace,
            BWAPI::UpgradeTypes::Ventral_Sacs,
            BWAPI::UpgradeTypes::Zerg_Carapace,
            BWAPI::UpgradeTypes::Zerg_Flyer_Attacks,
            BWAPI::UpgradeTypes::Zerg_Flyer_Carapace,
            BWAPI::UpgradeTypes::Zerg_Melee_Attacks,
            BWAPI::UpgradeTypes::Zerg_Missile_Attacks
        };
    }
}