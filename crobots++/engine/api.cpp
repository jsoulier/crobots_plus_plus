#include <SDL3/SDL.h>

#include <format>
#include <string_view>

#include "api.hpp"
#include "log.hpp"

static constexpr const char* kNewRobot = "NewRobot";
using NewRobotFunction = IRobot*(*)();

IRobot* ApiLoadRobot(const std::string_view& string)
{
    std::filesystem::path path = SDL_GetBasePath();
    path /= string;
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
        Log(std::format("Failed to load robot: {}, {}", path.string(), SDL_GetError()));
        return nullptr;
    }
    NewRobotFunction function = reinterpret_cast<NewRobotFunction>(SDL_LoadFunction(object, kNewRobot));
    if (!function)
    {
        Log(std::format("Failed to load function: {}, {}", path.string(), SDL_GetError()));
        SDL_UnloadObject(object);
        return nullptr;
    }
    IRobot* robot = function();
    if (!robot)
    {
        Log(std::format("Failed to create robot: {}, {}", path.string(), SDL_GetError()));
        SDL_UnloadObject(object);
        return nullptr;
    }
    SDL_UnloadObject(object);
    return robot;
}