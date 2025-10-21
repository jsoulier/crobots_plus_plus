#pragma once

#include <string_view>

class IRobot;

IRobot* LoadRobot(const std::string_view& name);