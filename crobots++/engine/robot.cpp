#include <SDL3/SDL.h>

#include <string_view>

#include "robot.hpp"

static constexpr const char* kNewRobot = "NewRobot";
using NewRobotFunction = IRobot*(*)();

IRobot* LoadRobot(const std::string_view& name)
{
    std::filesystem::path path = SDL_GetBasePath();
    path /= name;
#if defined(SDL_PLATFORM_WIN32)
    path.replace_extension(".dll");
#elif defined(SDL_PLATFORM_LINUX)
    path.replace_extension(".so");
#elif defined(SDL_PLATFORM_APPLE)
    path.replace_extension(".dylib");
#endif
    SDL_SharedObject* object = SDL_LoadObject(path.string().data());
    if (!object)
    {
        SDL_Log("Failed to load robot: %s, %s", path.string().data(), SDL_GetError());
        return nullptr;
    }
    NewRobotFunction function = reinterpret_cast<NewRobotFunction>(SDL_LoadFunction(object, kNewRobot));
    if (!function)
    {
        SDL_Log("Failed to load %s: %s, %s", kNewRobot, name.data(), SDL_GetError());
        SDL_UnloadObject(object);
        return nullptr;
    }
    IRobot* robot = function();
    if (!robot)
    {
        SDL_Log("Failed to create robot: %s, %s", name.data(), SDL_GetError());
        SDL_UnloadObject(object);
        return nullptr;
    }
    SDL_UnloadObject(object);
    return robot;
}