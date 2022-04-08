#include "FrogMath.h"
#include "../basic/Units.h"
#include "../basic/Tech.h"
#include <BWAPI.h>
#include <cmath>

namespace Utility::FrogMath{

    // 'a' = from, 'b' = to. Returns angle in radians.
    double get_angle(BWAPI::Position a, BWAPI::Position b) {
        double angle = atan2(b.y - a.y, b.x - a.x); 
        return angle;
    }

    // Requires a reference angle for what constitutes "inside", or in-between
    // a and b. This allows it to work with angles > 180 degrees
    // always returns true if 'check' == a or b
    bool angle_is_between(double check, double a, double b, double inside) {
        if (b < a) {
            if (inside > b && inside < a) {
                return check > b && check < a;
            }
            else {
                return (check < b || check > a);
            }
        }
        else {
            if (inside > a && inside < b) {
                return check > a && check < b;
            }
            else {
                return (check < a || check > b);
            }
        }
    }

    // expensive?
    double vector_angle(double vec1_x, double vec1_y, double vec2_x, double vec2_y) {
        return acos(
            (vec1_x*vec2_x + vec1_y*vec2_y) 
            / (sqrt(vec1_x*vec1_x + vec1_y*vec1_y) * sqrt(vec2_x*vec2_x + vec2_y*vec2_y))
        );
    }

    double vector_angle(const std::pair<double, double>& a, const std::pair<double, double> &b) {
        return acos(
            (a.first*b.first + a.second*b.second) 
            / (sqrt(a.first*a.first + a.second*a.second) * sqrt(b.first*b.first + b.second*b.second))
        );
    }

    double unit_vector_angle(double vec1_x, double vec1_y, double vec2_x, double vec2_y) {
        return acos(vec1_x*vec2_x + vec1_y*vec2_y);
    }

    double unit_vector_angle(const std::pair<double, double>& a, const std::pair<double, double> &b) {
        return acos(a.first*b.first + a.second*b.second);
    }

    // Requires a reference angle for what constitutes "inside", or in-between
    // a and b. This allows it to work with angles > 180 degrees
    // always returns true if 'check' == a or b
    bool pos_is_between_angle(
        BWAPI::Position from_pos,
        BWAPI::Position check, 
        double a_angle, 
        double b_angle,
        double inside_angle
    ) {
        double check_angle = get_angle(from_pos, check);

        return angle_is_between(check_angle, a_angle, b_angle, inside_angle);
    }

    // 'a' = from, 'b' = to.
    std::pair<double, double> unit_vector(BWAPI::Position a, BWAPI::Position b) {
        b.x -= a.x;
        b.y -= a.y;
        double 
            one_over_hyp = 1.0 / sqrt((double)b.x * b.x + (double)b.y * b.y),
            x = (double)b.x * one_over_hyp,
            y = (double)b.y * one_over_hyp;
        std::pair<double, double> unit_vec;
        unit_vec.first = x;
        unit_vec.second = y;
        return unit_vec;
    }

    std::pair<double, double> full_vector(BWAPI::Position a, BWAPI::Position b) {
        std::pair<double, double> full_vec;
        full_vec.first = b.x - a.x;
        full_vec.second = b.y - a.y;
        return full_vec;
    }

    double magnitude(const std::pair<double, double> &vec) {
        return sqrt(vec.first * vec.first + vec.second * vec.second);
    }

    BWAPI::TilePosition average_tileposition(std::vector<BWAPI::Unit> units) {
        BWAPI::TilePosition avg_tilepos;
        avg_tilepos.x = 0;
        avg_tilepos.y = 0;
        for (BWAPI::Unit &u: units) {
            const BWAPI::TilePosition &tp = Basic::Units::data(u).tilepos;
            avg_tilepos.x += tp.x;
            avg_tilepos.y += tp.y;
        }
        int units_size = units.size();
        avg_tilepos.x = avg_tilepos.x / units_size;
        avg_tilepos.y = avg_tilepos.y / units_size;
        return avg_tilepos;
    }

    BWAPI::Position average_position(std::vector<BWAPI::Unit> units) {
        BWAPI::Position avg_pos;
        avg_pos.x = 0;
        avg_pos.y = 0;
        for (BWAPI::Unit &u: units) {
            const BWAPI::Position &p = u->getPosition();
            avg_pos.x += p.x;
            avg_pos.y += p.y;
        }
        avg_pos.x = avg_pos.x / units.size();
        avg_pos.y = avg_pos.y / units.size();
        return avg_pos;
    }

    // TODO: check for terran unit type upgrades when added
    float average_speed(std::vector<BWAPI::Unit> units) {
        bool zergling_speed = Basic::Tech::self_upgrade_level(BWAPI::UpgradeTypes::Metabolic_Boost) > 0;
        bool hydralisk_speed = Basic::Tech::self_upgrade_level(BWAPI::UpgradeTypes::Muscular_Augments) > 0;
        bool ultralisk_speed = Basic::Tech::self_upgrade_level(BWAPI::UpgradeTypes::Anabolic_Synthesis) > 0;
        float total_speed = 0.0f;
        for (auto &unit : units) {
            BWAPI::UnitType &unit_type = unit->getType();
            float unit_speed = (float)unit_type.topSpeed();
            if (
                (unit_type == BWAPI::UnitTypes::Zerg_Zergling && zergling_speed)
                || (unit_type == BWAPI::UnitTypes::Zerg_Hydralisk && hydralisk_speed)
                || (unit_type == BWAPI::UnitTypes::Zerg_Ultralisk && ultralisk_speed)
            ) {
                unit_speed *= 1.5f;
            }
            total_speed += unit_speed;
        }
        return total_speed / units.size();
    }

    float get_distance(BWAPI::Position a, BWAPI::Position b) {
        float x_diff = a.x - b.x;
        float y_diff = a.y - b.y;
        return (float)sqrt(x_diff*x_diff + y_diff*y_diff);
    }
}