#include <SDL3/SDL.h>
#include <jsmn.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "pipeline.hpp"

static SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const std::string_view &name)
{
    SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(device);
    const char *entrypoint;
    const char *suffix;
    if (format & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
        suffix = ".spv";
    }
    else if (format & SDL_GPU_SHADERFORMAT_DXIL)
    {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
        suffix = ".dxil";
    }
    else if (format & SDL_GPU_SHADERFORMAT_MSL)
    {
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
        suffix = ".msl";
    }
    else
    {
        SDL_assert(false);
    }
    std::filesystem::path path = SDL_GetBasePath();
    path /= name;
    path += suffix;
    std::ifstream shaderFile(path, std::ios::binary);
    if (shaderFile.fail())
    {
        SDL_Log("Failed to open shader: %s", path.string().data());
        return nullptr;
    }
    path = SDL_GetBasePath();
    path /= name;
    path += ".json";
    std::ifstream jsonFile(path, std::ios::binary);
    if (jsonFile.fail())
    {
        SDL_Log("Failed to open json: %s", path.string().data());
        return nullptr;
    }
    std::string shaderData(std::istreambuf_iterator<char>(shaderFile), {});
    std::string jsonData(std::istreambuf_iterator<char>(jsonFile), {});
    jsmn_parser parser;
    jsmntok_t tokens[64];
    jsmn_init(&parser);
    if (jsmn_parse(&parser, jsonData.data(), jsonData.size(), tokens, 64) <= 0)
    {
        SDL_Log("Failed to parse json: %s", path.string().data());
        return nullptr;
    }
    SDL_GPUShaderCreateInfo info{};
    for (int i = 1; i < 9; i += 2)
    {
        if (tokens[i].type != JSMN_STRING)
        {
            continue;
        }
        char *keyString = jsonData.data() + tokens[i + 0].start;
        char *valueString = jsonData.data() + tokens[i + 1].start;
        int keySize = tokens[i + 0].end - tokens[i + 0].start;
        uint32_t *value;
        if (!std::memcmp("samplers", keyString, keySize))
        {
            value = &info.num_samplers;
        }
        else if (!std::memcmp("storage_textures", keyString, keySize))
        {
            value = &info.num_storage_textures;
        }
        else if (!std::memcmp("storage_buffers", keyString, keySize))
        {
            value = &info.num_storage_buffers;
        }
        else if (!std::memcmp("uniform_buffers", keyString, keySize))
        {
            value = &info.num_uniform_buffers;
        }
        else
        {
            continue;
        }
        *value = *valueString - '0';
    }
    info.code = reinterpret_cast<Uint8 *>(shaderData.data());
    info.code_size = shaderData.size();
    info.entrypoint = entrypoint;
    info.format = format;
    if (name.contains(".frag"))
    {
        info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else
    {
        info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
    if (!shader)
    {
        SDL_Log("Failed to create shader: %s, %s", name.data(), SDL_GetError());
        return nullptr;
    }
    return shader;
}

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

SDL_GPUGraphicsPipeline* CreateLinePipeline(SDL_GPUDevice* device, SDL_Window* window)
{
    SDL_GPUShader* fragShader = LoadShader(device, "line.frag");
    SDL_GPUShader* vertShader = LoadShader(device, "line.vert");
    if (!fragShader || !vertShader)
    {
        SDL_Log("Failed to load shader(s)");
        return nullptr;
    }
    SDL_GPUColorTargetDescription targets[1]{};
    SDL_GPUVertexBufferDescription buffers[1]{};
    SDL_GPUVertexAttribute attribs[2]{};
    targets[0].format = SDL_GetGPUSwapchainTextureFormat(device, window);
    buffers[0].slot = 0;
    buffers[0].pitch = sizeof(float) * 3 + sizeof(uint32_t);
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
    info.vertex_input_state.num_vertex_attributes = 2;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
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