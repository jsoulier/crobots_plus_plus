#pragma once

#include <string_view>

namespace crobots
{

class IRobot;
struct RobotContext;

}

crobots::IRobot* LoadRobot(const std::string_view& name, crobots::RobotContext* context);