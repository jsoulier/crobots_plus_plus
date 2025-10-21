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

#include "shader.hpp"

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const std::string_view &name)
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