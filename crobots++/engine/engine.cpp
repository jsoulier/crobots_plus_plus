#include <memory>
#include <string_view>
#include <vector>

#include "api.hpp"
#include "engine.hpp"
#include "log.hpp"

static std::vector<std::shared_ptr<IRobot>> robots;

bool EngineInit(const EngineParameters& parameters)
{
    Log("Robots: ");
    for (const std::string_view& string : parameters.Robots)
    {
        Log(string);
    }
    for (const std::string_view& string : parameters.Robots)
    {
        IRobot* robot = ApiLoadRobot(string);
        if (robot)
        {
            robots.emplace_back(robot);
        }
    }
    return true;
}

void EngineQuit()
{
    robots.clear();
}