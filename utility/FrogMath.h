#pragma once
#pragma message("including FrogMath")

#include <BWAPI.h>

namespace Utility::FrogMath {

    const double PI = 3.14159265359;
    
    double get_angle(BWAPI::Position a, BWAPI::Position b);
    bool angle_is_between(double check, double a, double b, double inside);
    double vector_angle(double vec1_x, double vec1_y, double vec2_x, double vec2_y);
    double vector_angle(const std::pair<double, double>& a, const std::pair<double, double> &b);
    double unit_vector_angle(double vec1_x, double vec1_y, double vec2_x, double vec2_y);
    double unit_vector_angle(const std::pair<double, double>& a, const std::pair<double, double> &b);
    bool pos_is_between_angle(
        BWAPI::Position from_pos,
        BWAPI::Position check, 
        double a_angle, 
        double b_angle,
        double inside_angle
    );
    std::pair<double, double> unit_vector(BWAPI::Position a, BWAPI::Position b);
    std::pair<double, double> full_vector(BWAPI::Position a, BWAPI::Position b);
    double magnitude(const std::pair<double, double> &vec);
    BWAPI::TilePosition average_tileposition(std::vector<BWAPI::Unit> units);
    BWAPI::Position average_position(std::vector<BWAPI::Unit> units);
    float average_speed(std::vector<BWAPI::Unit> units);
    float get_distance(BWAPI::Position a, BWAPI::Position b);
}