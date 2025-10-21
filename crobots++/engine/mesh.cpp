#include <SDL3/SDL.h>

#include <cstring>

#include "mesh.hpp"

static constexpr float kVertices[] =
{
   -0.5f,-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f,-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
   -0.5f,-0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
   -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f,-0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
   -0.5f,-0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
   -0.5f, 0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
    0.5f,-0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
   -0.5f, 0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
    0.5f, 0.5f,-0.5f, 0.0f, 0.0f,-1.0f,
   -0.5f,-0.5f,-0.5f,-1.0f, 0.0f, 0.0f,
   -0.5f,-0.5f, 0.5f,-1.0f, 0.0f, 0.0f,
   -0.5f, 0.5f, 0.5f,-1.0f, 0.0f, 0.0f,
   -0.5f,-0.5f,-0.5f,-1.0f, 0.0f, 0.0f,
   -0.5f, 0.5f, 0.5f,-1.0f, 0.0f, 0.0f,
   -0.5f, 0.5f,-0.5f,-1.0f, 0.0f, 0.0f,
    0.5f,-0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f,-0.5f,-0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f,-0.5f, 1.0f, 0.0f, 0.0f,
    0.5f,-0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f,-0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
   -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,
   -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,
   -0.5f, 0.5f,-0.5f, 0.0f, 1.0f, 0.0f,
   -0.5f,-0.5f,-0.5f, 0.0f,-1.0f, 0.0f,
    0.5f,-0.5f,-0.5f, 0.0f,-1.0f, 0.0f,
    0.5f,-0.5f, 0.5f, 0.0f,-1.0f, 0.0f,
   -0.5f,-0.5f,-0.5f, 0.0f,-1.0f, 0.0f,
    0.5f,-0.5f, 0.5f, 0.0f,-1.0f, 0.0f,
   -0.5f,-0.5f, 0.5f, 0.0f,-1.0f, 0.0f,
};

SDL_GPUBuffer* CreateCubeBuffer(SDL_GPUDevice* device)
{
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return nullptr;
    }
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!copyPass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        return nullptr;
    }
    SDL_GPUTransferBuffer* transferBuffer;
    {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = sizeof(kVertices);
        transferBuffer = SDL_CreateGPUTransferBuffer(device, &info);
        if (!transferBuffer)
        {
            SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    SDL_GPUBuffer* buffer;
    {
        SDL_GPUBufferCreateInfo info{};
        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = sizeof(kVertices);
        buffer = SDL_CreateGPUBuffer(device, &info);
        if (!buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    float* vertices = static_cast<float*>(SDL_MapGPUTransferBuffer(device, transferBuffer, false));
    if (!vertices)
    {
        SDL_Log("Failed to map buffer: %s", SDL_GetError());
        return nullptr;
    }
    std::memcpy(vertices, kVertices, sizeof(kVertices));
    {
        SDL_GPUTransferBufferLocation location{};
        SDL_GPUBufferRegion region{};
        location.transfer_buffer = transferBuffer;
        region.buffer = buffer;
        region.size = sizeof(kVertices);
        SDL_UploadToGPUBuffer(copyPass, &location, &region, false);
    }
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    return buffer;
}