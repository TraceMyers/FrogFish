#include "UnitData.h"

using namespace Basic::Refs;

namespace Basic::Units {

    UTYPE get_utype(BWAPI::Unit u) {
        BWAPI::UnitType type = u->getType();
        if (type.isBuilding()) {
            return UTYPE::STRUCT;
        }
        else if (type.isWorker()) {
            return UTYPE::WORKER;
        }
        else if (
            type.canAttack() 
            || type.isSpellcaster() 
            || type.isFlyer()
        ) {
            return UTYPE::ARMY;
        }
        else if (u->isMorphing()) {
            return UTYPE::EGG;
        }
        else {
            return UTYPE::LARVA;
        }
    }

    UnitData::UnitData(BWAPI::Unit u) :
        cmd_ready(true),
        cmd_timer(),
        u_task(UTASK::IDLE),
        build_status(BUILD_STATUS::NONE),
        ID(u->getID()), 
        hp(u->getHitPoints()),
        max_hp(u->getType().maxHitPoints()),
        shields(u->getShields()),
        energy(u->getEnergy()),
        ground_weapon_cd(u->getGroundWeaponCooldown()),
        air_weapon_cd(u->getAirWeaponCooldown()),
        spell_cd(u->getSpellCooldown()),
        irradiate_timer(u->getIrradiateTimer()),
        lockdown_timer(u->getLockdownTimer()),
        plague_timer(u->getPlagueTimer()),
        stasis_timer(u->getStasisTimer()),
        transport(u->getTransport()),
        loaded_units(u->getLoadedUnits()),
        teching(u->getTech()),
        upgrading(u->getUpgrade()),
        type(u->getType()), 
        init_type(u->getInitialType()),
        pos(u->getPosition()),
        tilepos(u->getTilePosition()),
        angle(u->getAngle()),
        u_type(get_utype(u)),
        missing(false),
        lifted(false)
    {
        velocity[0] = u->getVelocityX(); 
        velocity[1] = u->getVelocityY();
    }
}