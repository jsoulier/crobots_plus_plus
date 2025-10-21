#include <crobots++/robot.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "buffer.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "robot.hpp"
#include "shader.hpp"

struct Instance
{
    glm::mat4 Matrix;
};

struct Robot
{
    std::unique_ptr<crobots::IRobot> Interface;
    crobots::RobotContext Context;
    b2BodyId BodyId;
};

struct Projectile
{
    b2BodyId BodyId;
};

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUGraphicsPipeline* cubePipeline;
static SDL_GPUBuffer* cubeBuffer;
static DynamicBuffer<Instance> instanceBuffer;
static Camera camera;
static std::vector<Robot> robots;
static std::vector<Projectile> projectiles;

static bool Parse(int argc, char** argv)
{
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
                Robot robot;
                robot.Interface.reset(LoadRobot(inner, &robot.Context));
                if (!robot.Interface)
                {
                    continue;
                }
                robots.push_back(std::move(robot));
            }
        }
    }
    if (robots.empty())
    {
        SDL_Log("No robots loaded");
        return false;
    }
    return true;
}

static bool Init()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("Crobots++", 960, 540, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }
#ifndef NDEBUG
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
#else
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, nullptr);
#endif
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        return false;
    }
    cubeBuffer = CreateCubeBuffer(device);
    if (!cubeBuffer)
    {
        SDL_Log("Failed to create cube buffer");
        return false;
    }
    cubePipeline = CreateCubePipeline(device, window);
    if (!cubePipeline)
    {
        SDL_Log("Failed to create cube pipeline");
        return false;
    }
    return true;
}

static void Draw()
{
    SDL_WaitForGPUSwapchain(device, window);
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* swapchainTexture;
    uint32_t width;
    uint32_t height;
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height))
    {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }
    if (!width || !height || !swapchainTexture)
    {
        // not an error
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

int main(int argc, char** argv)
{
    if (!Parse(argc, argv))
    {
        return 1;
    }
    if (!Init())
    {
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
        Draw();
    }
    instanceBuffer.Destroy(device);
    SDL_ReleaseGPUBuffer(device, cubeBuffer);
    SDL_ReleaseGPUGraphicsPipeline(device, cubePipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    robots.clear();
    projectiles.clear();
    return 0;
}