#pragma once

#include <vector>
#include <string>

#include "api.hpp"

struct EngineParameters
{
    std::vector<std::string> Robots;
};

bool EngineInit(const EngineParameters& parameters);
void EngineQuit();