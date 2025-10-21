#pragma once

#include <string_view>

namespace crobots
{

class IRobot;
class RobotContext;

}

crobots::IRobot* LoadRobot(const std::string_view& name, crobots::RobotContext* context);