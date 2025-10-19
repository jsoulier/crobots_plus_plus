#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

#include "engine.hpp"
#include "log.hpp"
#include "renderer.hpp"

EngineParameters GetParameters(int argc, char** argv)
{
    EngineParameters parameters;
    for (int i = 1; i < argc; i++)
    {
        std::string outer = argv[i];
        if (outer == "--robots")
        {
            for (i++; i < argc; i++)
            {
                std::string inner = argv[i];
                if (inner.starts_with("--"))
                {
                    break;
                }
                parameters.Robots.push_back(inner);
            }
        }
    }
    return parameters;
}

int main(int argc, char** argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        Log(std::format("Failed to initialize SDL: {}", SDL_GetError()));
        return 1;
    }
    if (!RendererInit())
    {
        Log("Failed to initialize renderer");
        return 1;
    }
    if (!EngineInit(GetParameters(argc, argv)))
    {
        Log("Failed to initialize engine");
        return 1;
    }
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            }
        }
        RendererSubmit();
    }
    EngineQuit();
    RendererQuit();
    return 0;
}