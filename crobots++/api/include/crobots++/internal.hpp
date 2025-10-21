#pragma once

#include <crobots++/math.hpp>

namespace crobots
{

class RobotContext
{
public:
    Meters X;
    Meters Y;
    MetersPerSecond Speed;
    Radians Rotation;
};

}