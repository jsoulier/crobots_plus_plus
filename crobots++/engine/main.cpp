#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>

#include "buffer.hpp"
#include "camera.hpp"
#include "engine.hpp"
#include "renderer.hpp"

static EngineParams GetParams(int argc, char** argv)
{
    EngineParams params;
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
                params.Robots.push_back(inner);
            }
        }
        else if (outer == "--timestep")
        {
            std::string inner = argv[i];
            try
            {
                params.Timestep = std::stof(inner);
            }
            catch (const std::invalid_argument& e)
            {
                SDL_Log("Failed to parse timestep: %s", e.what());
                return params;
            }
        }
    }
    return params;
}

int main(int argc, char** argv)
{
    SDL_Window* window;
    Engine engine;
    Renderer renderer;
    Camera camera;
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    if (!engine.Init(GetParams(argc, argv)))
    {
        SDL_Log("Failed to initialize engine");
        return 1;
    }
    window = SDL_CreateWindow("Crobots++", 960, 540, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return 1;
    }
    if (!renderer.Init(window))
    {
        SDL_Log("Failed to initialize renderer");
        return 1;
    }
    camera.SetCenter(engine.GetWidth() / 2.0f, engine.GetWidth() / 2.0f);
    bool running = true;
    uint64_t time2 = SDL_GetTicks();
    uint64_t time1 = time2;
    while (running)
    {
        time2 = SDL_GetTicks();
        float deltaTime = time2 - time1;
        time1 = time2;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (camera.GetType() == CameraType::FreeCam)
                {
                    if (!SDL_GetWindowRelativeMouseMode(window))
                    {
                        SDL_SetWindowRelativeMouseMode(window, true);
                    }
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    if (SDL_GetWindowRelativeMouseMode(window))
                    {
                        SDL_SetWindowRelativeMouseMode(window, false);
                    }
                }
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (SDL_GetWindowRelativeMouseMode(window))
                {
                    SDL_SetWindowRelativeMouseMode(window, false);
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (SDL_GetWindowRelativeMouseMode(window))
                {
                    camera.MouseMotion(event.motion.xrel, event.motion.yrel);
                }
                else if (event.motion.state & SDL_BUTTON_LMASK)
                {
                    camera.MouseMotion(event.motion.xrel, event.motion.yrel);
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                camera.MouseScroll(event.wheel.y);
                break;
            }
        }
        const bool* keys = SDL_GetKeyboardState(nullptr);
        glm::vec3 delta{0.0f};
        delta.x += keys[SDL_SCANCODE_D];
        delta.x -= keys[SDL_SCANCODE_A];
        delta.y += keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_E];
        delta.y -= keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_Q];
        delta.z += keys[SDL_SCANCODE_W];
        delta.z -= keys[SDL_SCANCODE_S];
        camera.Move(delta.x, delta.y, delta.z, deltaTime);
        engine.Tick();
        renderer.Draw(engine, camera);
    }
    renderer.Destroy();
    SDL_DestroyWindow(window);
    engine.Destroy();
    SDL_Quit();
    return 0;
}