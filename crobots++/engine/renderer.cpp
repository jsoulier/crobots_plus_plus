#include <SDL3/SDL.h>

#include <cstdint>
#include <format>

#include "log.hpp"
#include "renderer.hpp"
#include "shader.hpp"

static SDL_Window* window;
static SDL_GPUDevice* device;

bool RendererInit()
{
    window = SDL_CreateWindow("Crobots++", 960, 540, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        Log(std::format("Failed to create window: {}", SDL_GetError()));
        return false;
    }
#ifndef NDEBUG
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);
#else
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, nullptr);
#endif
    if (!device)
    {
        Log(std::format("Failed to create device: {}", SDL_GetError()));
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        Log(std::format("Failed to claim window: {}", SDL_GetError()));
        return false;
    }
    return true;
}

void RendererQuit()
{
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    device = nullptr;
    window = nullptr;
}

void RendererSubmit()
{
    SDL_WaitForGPUSwapchain(device, window);
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        Log(std::format("Failed to acquire command buffer: {}", SDL_GetError()));
        return;
    }
    SDL_GPUTexture* swapchainTexture;
    uint32_t width;
    uint32_t height;
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height))
    {
        Log(std::format("Failed to acquire swapchain texture: {}", SDL_GetError()));
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