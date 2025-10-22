#include <SDL3/SDL.h>

#include <cstdint>

#include "pipeline.hpp"
#include "shader.hpp"

SDL_GPUGraphicsPipeline* CreateCubePipeline(SDL_GPUDevice* device, SDL_Window* window)
{
    SDL_GPUShader* fragShader = LoadShader(device, "cube.frag");
    SDL_GPUShader* vertShader = LoadShader(device, "cube.vert");
    if (!fragShader || !vertShader)
    {
        SDL_Log("Failed to load shader(s)");
        return nullptr;
    }
    SDL_GPUColorTargetDescription targets[1]{};
    SDL_GPUVertexBufferDescription buffers[2]{};
    SDL_GPUVertexAttribute attribs[6]{};
    targets[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);
    buffers[0].slot = 0;
    buffers[0].pitch = sizeof(float) * 6;
    buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    buffers[0].instance_step_rate = 0;
    buffers[1].slot = 1;
    buffers[1].pitch = sizeof(float) * 16;
    buffers[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    buffers[1].instance_step_rate = 0;
    attribs[0].location = 0;
    attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attribs[0].offset = 0;
    attribs[0].buffer_slot = 0;
    attribs[1].location = 1;
    attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attribs[1].offset = sizeof(float) * 3;
    attribs[1].buffer_slot = 0;
    attribs[2].location = 2;
    attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attribs[2].offset = 0;
    attribs[2].buffer_slot = 1;
    attribs[3].location = 3;
    attribs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attribs[3].offset = sizeof(float) * 4;
    attribs[3].buffer_slot = 1;
    attribs[4].location = 4;
    attribs[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attribs[4].offset = sizeof(float) * 8;
    attribs[4].buffer_slot = 1;
    attribs[5].location = 5;
    attribs[5].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attribs[5].offset = sizeof(float) * 12;
    attribs[5].buffer_slot = 1;
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertShader;
    info.fragment_shader = fragShader;
    info.target_info.color_target_descriptions = targets;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = 2;
    info.vertex_input_state.vertex_attributes = attribs;
    info.vertex_input_state.num_vertex_attributes = 6;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!pipeline)
    {
        SDL_Log("Failed to create cube pipeline: %s", SDL_GetError());
        return nullptr;
    }
    SDL_ReleaseGPUShader(device, fragShader);
    SDL_ReleaseGPUShader(device, vertShader);
    return pipeline;
}

SDL_GPUGraphicsPipeline* CreateSolidPolygonPipeline(SDL_GPUDevice* device, SDL_Window* window)
{
    SDL_GPUShader* fragShader = LoadShader(device, "solid_polygon.frag");
    SDL_GPUShader* vertShader = LoadShader(device, "solid_polygon.vert");
    if (!fragShader || !vertShader)
    {
        SDL_Log("Failed to load shader(s)");
        return nullptr;
    }
    SDL_GPUColorTargetDescription targets[1]{};
    SDL_GPUVertexBufferDescription buffers[1]{};
    SDL_GPUVertexAttribute attribs[5]{};
    targets[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);
    buffers[0].slot = 0;
    buffers[0].pitch = sizeof(float) * 3 + sizeof(uint32_t) + sizeof(float) * 2 + sizeof(float) + sizeof(float);
    buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    buffers[0].instance_step_rate = 0;
    attribs[0].location = 0;
    attribs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attribs[0].offset = 0;
    attribs[0].buffer_slot = 0;
    attribs[1].location = 1;
    attribs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    attribs[1].offset = sizeof(float) * 3;
    attribs[1].buffer_slot = 0;
    attribs[2].location = 2;
    attribs[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attribs[2].offset = sizeof(float) * 3 + sizeof(uint32_t);
    attribs[2].buffer_slot = 0;
    attribs[3].location = 3;
    attribs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    attribs[3].offset = sizeof(float) * 3 + sizeof(uint32_t) + sizeof(float) * 2;
    attribs[3].buffer_slot = 0;
    attribs[4].location = 4;
    attribs[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    attribs[4].offset = sizeof(float) * 3 + sizeof(uint32_t) + sizeof(float) * 2 + sizeof(float);
    attribs[4].buffer_slot = 0;
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertShader;
    info.fragment_shader = fragShader;
    info.target_info.color_target_descriptions = targets;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attribs;
    info.vertex_input_state.num_vertex_attributes = 5;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    if (!pipeline)
    {
        SDL_Log("Failed to create solid polygon pipeline: %s", SDL_GetError());
        return nullptr;
    }
    SDL_ReleaseGPUShader(device, fragShader);
    SDL_ReleaseGPUShader(device, vertShader);
    return pipeline;
}