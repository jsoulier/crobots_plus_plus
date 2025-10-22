#pragma once

#include <SDL3/SDL.h>

SDL_GPUGraphicsPipeline* CreateCubePipeline(SDL_GPUDevice* device, SDL_Window* window);
SDL_GPUGraphicsPipeline* CreateSolidPolygonPipeline(SDL_GPUDevice* device, SDL_Window* window);