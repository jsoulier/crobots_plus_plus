#pragma once

#include <crobots++/crobots++.hpp>

#include <string_view>

using namespace crobots;

IRobot* ApiLoadRobot(const std::string_view& string);