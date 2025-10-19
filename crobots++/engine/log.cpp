#include <SDL3/SDL.h>

#include <string_view>

#include "log.hpp"

void Log(const std::string_view& string)
{
    Log("engine", string);
}

void Log(const std::string_view& source, const std::string_view& string)
{
    SDL_Log("[%s] %s", source.data(), string.data());
}