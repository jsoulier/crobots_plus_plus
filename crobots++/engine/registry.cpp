#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>
#include <string_view>

#include "registry.hpp"

static constexpr const char* kNewRobot = "NewRobot";

crobots::IRobot* Registry::Load(const std::string_view& name, const std::shared_ptr<crobots::RobotContext>& context)
{
    for (Entry& entry : Entries)
    {
        if (entry.Name != name)
        {
            continue;
        }
        crobots::IRobot* robot = entry.Function(context);
        if (!robot)
        {
            SDL_Log("Failed to create robot: %s, %s", name.data(), SDL_GetError());
            return nullptr;
        }
        return robot;
    }
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
    crobots::IRobot* robot = function(context);
    if (!robot)
    {
        SDL_Log("Failed to create robot: %s, %s", name.data(), SDL_GetError());
        SDL_UnloadObject(object);
        return nullptr;
    }
    Entries.emplace_back(name, object, function);
    return robot;
}

void Registry::Destroy()
{
    for (Entry& entry : Entries)
    {
        SDL_UnloadObject(entry.Object);
    }
}