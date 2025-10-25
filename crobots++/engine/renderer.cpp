#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

#include "buffer.hpp"
#include "camera.hpp"
#include "engine.hpp"
#include "renderer.hpp"

static constexpr glm::vec3 kUp{0.0f, 1.0f, 0.0f};

Renderer::Renderer()
    : Window{nullptr}
    , Device{nullptr}
    , DepthTexture{nullptr}
    , InstancedPipeline{nullptr}
    , LinePipeline{nullptr}
    , SolidPolygonPipeline{nullptr}
    , CubeBuffer{nullptr}
    , InstanceBuffer{}
    , LineBuffer{}
    , SolidPolygonBuffer{}
{
}

bool Renderer::Init(SDL_Window* window)
{
    Window = window;
    SDL_PropertiesID props = SDL_CreateProperties();
#ifndef NDEBUG
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
#else
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
#endif
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, true);
    Device = SDL_CreateGPUDeviceWithProperties(props);
    if (!Device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(Device, Window))
    {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        return false;
    }
    CubeBuffer = CreateCubeBuffer();
    if (!CubeBuffer)
    {
        SDL_Log("Failed to create cube buffer");
        return false;
    }
    InstancedPipeline = CreateInstancedPipeline();
    if (!InstancedPipeline)
    {
        SDL_Log("Failed to create instanced pipeline");
        return false;
    }
    LinePipeline = CreateLinePipeline();
    if (!LinePipeline)
    {
        SDL_Log("Failed to create line pipeline");
        return false;
    }
    SolidPolygonPipeline = CreateSolidPolygonPipeline();
    if (!SolidPolygonPipeline)
    {
        SDL_Log("Failed to create solid polygon pipeline");
        return false;
    }
    SDL_DestroyProperties(props);
    DebugDraw = b2DefaultDebugDraw();
    DebugDraw.context = this;
    DebugDraw.DrawSolidPolygonFcn = DrawSolidPolygon;
    DebugDraw.DrawSegmentFcn = DrawSegment;
    DebugDraw.drawShapes = true;
    return true;
}

void Renderer::Destroy()
{
    SolidPolygonBuffer.Destroy(Device);
    LineBuffer.Destroy(Device);
    InstanceBuffer.Destroy(Device);
    SDL_ReleaseGPUTexture(Device, DepthTexture);
    SDL_ReleaseGPUBuffer(Device, CubeBuffer);
    SDL_ReleaseGPUGraphicsPipeline(Device, SolidPolygonPipeline);
    SDL_ReleaseGPUGraphicsPipeline(Device, LinePipeline);
    SDL_ReleaseGPUGraphicsPipeline(Device, InstancedPipeline);
    SDL_ReleaseWindowFromGPUDevice(Device, Window);
    SDL_DestroyGPUDevice(Device);
    SDL_Quit();
}

void Renderer::Draw(const Engine& engine, Camera& camera)
{
    SDL_WaitForGPUSwapchain(Device, Window);
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(Device);
    if (!commandBuffer)
    {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* swapchainTexture;
    uint32_t width;
    uint32_t height;
    if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, Window, &swapchainTexture, &width, &height))
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
    if (camera.GetWidth() != width || camera.GetHeight() != height)
    {
        SDL_ReleaseGPUTexture(Device, DepthTexture);
        SDL_GPUTextureCreateInfo info{};
        info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        DepthTexture = SDL_CreateGPUTexture(Device, &info);
        if (!DepthTexture)
        {
            SDL_Log("Failed to create depth texture: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        camera.SetSize(width, height);
    }
    camera.Update();
    if (engine.GetDebug())
    {
        b2World_Draw(engine.GetWorldID(), &DebugDraw);
    }
    for (const Robot& robot : engine.GetRobots())
    {
        b2Vec2 position = b2Body_GetPosition(robot.BodyID);
        b2Rot rotation = b2Body_GetRotation(robot.BodyID);
        glm::mat4 r = glm::rotate(glm::mat4(1.0f), -b2Rot_GetAngle(rotation), kUp);
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, 0.0f, position.y));
        glm::mat4 transform = t * r;
        InstanceBuffer.Emplace(Device, t * r);
    }
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!copyPass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }
    InstanceBuffer.Upload(Device, copyPass);
    LineBuffer.Upload(Device, copyPass);
    SolidPolygonBuffer.Upload(Device, copyPass);
    SDL_EndGPUCopyPass(copyPass);
    if (InstanceBuffer.GetSize())
    {
        SDL_GPUColorTargetInfo colorInfo{};
        SDL_GPUDepthStencilTargetInfo depthInfo{};
        colorInfo.texture = swapchainTexture;
        colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.texture = DepthTexture;
        depthInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        depthInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        depthInfo.clear_depth = 1.0f;
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorInfo, 1, &depthInfo);
        if (!renderPass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        SDL_GPUBufferBinding vertexBuffers[2]{};
        vertexBuffers[0].buffer = CubeBuffer;
        vertexBuffers[1].buffer = InstanceBuffer.GetBuffer();
        SDL_BindGPUGraphicsPipeline(renderPass, InstancedPipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, vertexBuffers, 2);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &camera.GetViewProj(), 64);
        SDL_DrawGPUPrimitives(renderPass, 36, InstanceBuffer.GetSize(), 0, 0);
        SDL_EndGPURenderPass(renderPass);
    }
    if (SolidPolygonBuffer.GetSize())
    {
        SDL_GPUColorTargetInfo colorInfo{};
        SDL_GPUDepthStencilTargetInfo depthInfo{};
        colorInfo.texture = swapchainTexture;
        colorInfo.load_op = SDL_GPU_LOADOP_LOAD;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.texture = DepthTexture;
        depthInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        depthInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        depthInfo.clear_depth = 1.0f;
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorInfo, 1, &depthInfo);
        if (!renderPass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        SDL_GPUBufferBinding vertexBuffer{};
        vertexBuffer.buffer = SolidPolygonBuffer.GetBuffer();
        SDL_BindGPUGraphicsPipeline(renderPass, SolidPolygonPipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBuffer, 1);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &camera.GetViewProj(), 64);
        SDL_DrawGPUPrimitives(renderPass, SolidPolygonBuffer.GetSize(), 1, 0, 0);
        SDL_EndGPURenderPass(renderPass);
    }
    if (LineBuffer.GetSize())
    {
        SDL_GPUColorTargetInfo colorInfo{};
        SDL_GPUDepthStencilTargetInfo depthInfo{};
        colorInfo.texture = swapchainTexture;
        colorInfo.load_op = SDL_GPU_LOADOP_LOAD;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.texture = DepthTexture;
        depthInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        depthInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        depthInfo.clear_depth = 1.0f;
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorInfo, 1, &depthInfo);
        if (!renderPass)
        {
            SDL_Log("Failed to begin render pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        SDL_GPUBufferBinding vertexBuffer{};
        vertexBuffer.buffer = LineBuffer.GetBuffer();
        SDL_BindGPUGraphicsPipeline(renderPass, LinePipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBuffer, 1);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &camera.GetViewProj(), 64);
        SDL_DrawGPUPrimitives(renderPass, LineBuffer.GetSize(), 1, 0, 0);
        SDL_EndGPURenderPass(renderPass);
    }
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

SDL_GPUShader* Renderer::LoadShader(const std::string_view &name)
{
    SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(Device);
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
    SDL_GPUShader *shader = SDL_CreateGPUShader(Device, &info);
    if (!shader)
    {
        SDL_Log("Failed to create shader: %s, %s", name.data(), SDL_GetError());
        return nullptr;
    }
    return shader;
}

SDL_GPUGraphicsPipeline* Renderer::CreateInstancedPipeline()
{
    SDL_GPUShader* fragShader = LoadShader("instanced.frag");
    SDL_GPUShader* vertShader = LoadShader("instanced.vert");
    if (!fragShader || !vertShader)
    {
        SDL_Log("Failed to load shader(s)");
        return nullptr;
    }
    SDL_GPUColorTargetDescription targets[1]{};
    SDL_GPUVertexBufferDescription buffers[2]{};
    SDL_GPUVertexAttribute attribs[6]{};
    targets[0].format = SDL_GetGPUSwapchainTextureFormat(Device, Window);
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
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(Device, &info);
    if (!pipeline)
    {
        SDL_Log("Failed to create cube pipeline: %s", SDL_GetError());
        return nullptr;
    }
    SDL_ReleaseGPUShader(Device, fragShader);
    SDL_ReleaseGPUShader(Device, vertShader);
    return pipeline;
}

SDL_GPUGraphicsPipeline* Renderer::CreateLinePipeline()
{
    SDL_GPUShader* fragShader = LoadShader("color.frag");
    SDL_GPUShader* vertShader = LoadShader("color.vert");
    if (!fragShader || !vertShader)
    {
        SDL_Log("Failed to load shader(s)");
        return nullptr;
    }
    SDL_GPUColorTargetDescription targets[1]{};
    SDL_GPUVertexBufferDescription buffers[1]{};
    SDL_GPUVertexAttribute attribs[2]{};
    targets[0].format = SDL_GetGPUSwapchainTextureFormat(Device, Window);
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
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(Device, &info);
    if (!pipeline)
    {
        SDL_Log("Failed to create line pipeline: %s", SDL_GetError());
        return nullptr;
    }
    SDL_ReleaseGPUShader(Device, fragShader);
    SDL_ReleaseGPUShader(Device, vertShader);
    return pipeline;
}

SDL_GPUGraphicsPipeline* Renderer::CreateSolidPolygonPipeline()
{
    SDL_GPUShader* fragShader = LoadShader("transformed_color.frag");
    SDL_GPUShader* vertShader = LoadShader("transformed_color.vert");
    if (!fragShader || !vertShader)
    {
        SDL_Log("Failed to load shader(s)");
        return nullptr;
    }
    SDL_GPUColorTargetDescription targets[1]{};
    SDL_GPUVertexBufferDescription buffers[1]{};
    SDL_GPUVertexAttribute attribs[4]{};
    targets[0].format = SDL_GetGPUSwapchainTextureFormat(Device, Window);
    buffers[0].slot = 0;
    buffers[0].pitch = sizeof(float) * 3 + sizeof(uint32_t) + sizeof(float) * 2 + sizeof(float) * 2;
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
    attribs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attribs[3].offset = sizeof(float) * 3 + sizeof(uint32_t) + sizeof(float) * 2;
    attribs[3].buffer_slot = 0;
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
    info.vertex_input_state.num_vertex_attributes = 4;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(Device, &info);
    if (!pipeline)
    {
        SDL_Log("Failed to create solid polygon pipeline: %s", SDL_GetError());
        return nullptr;
    }
    SDL_ReleaseGPUShader(Device, fragShader);
    SDL_ReleaseGPUShader(Device, vertShader);
    return pipeline;
}

SDL_GPUBuffer* Renderer::CreateCubeBuffer()
{
    static constexpr NormalVertex kVertices[] =
    {
        {{-0.5f,-0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}},
        {{ 0.5f,-0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, 0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}},
        {{-0.5f,-0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, 0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}},
        {{ 0.5f,-0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}},
        {{-0.5f,-0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}},
        {{-0.5f, 0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}},
        {{ 0.5f,-0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}},
        {{-0.5f, 0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}},
        {{ 0.5f, 0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}},
        {{-0.5f,-0.5f,-0.5f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f,-0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f,-0.5f,-0.5f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, 0.5f,-0.5f}, {-1.0f, 0.0f, 0.0f}},
        {{ 0.5f,-0.5f, 0.5f}, { 1.0f, 0.0f, 0.0f}},
        {{ 0.5f,-0.5f,-0.5f}, { 1.0f, 0.0f, 0.0f}},
        {{ 0.5f, 0.5f,-0.5f}, { 1.0f, 0.0f, 0.0f}},
        {{ 0.5f,-0.5f, 0.5f}, { 1.0f, 0.0f, 0.0f}},
        {{ 0.5f, 0.5f,-0.5f}, { 1.0f, 0.0f, 0.0f}},
        {{ 0.5f, 0.5f, 0.5f}, { 1.0f, 0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f, 0.0f}},
        {{ 0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f, 0.0f}},
        {{ 0.5f, 0.5f,-0.5f}, { 0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f, 0.0f}},
        {{ 0.5f, 0.5f,-0.5f}, { 0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f,-0.5f}, { 0.0f, 1.0f, 0.0f}},
        {{-0.5f,-0.5f,-0.5f}, { 0.0f,-1.0f, 0.0f}},
        {{ 0.5f,-0.5f,-0.5f}, { 0.0f,-1.0f, 0.0f}},
        {{ 0.5f,-0.5f, 0.5f}, { 0.0f,-1.0f, 0.0f}},
        {{-0.5f,-0.5f,-0.5f}, { 0.0f,-1.0f, 0.0f}},
        {{ 0.5f,-0.5f, 0.5f}, { 0.0f,-1.0f, 0.0f}},
        {{-0.5f,-0.5f, 0.5f}, { 0.0f,-1.0f, 0.0f}},
    };
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(Device);
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
        transferBuffer = SDL_CreateGPUTransferBuffer(Device, &info);
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
        buffer = SDL_CreateGPUBuffer(Device, &info);
        if (!buffer)
        {
            SDL_Log("Failed to create buffer: %s", SDL_GetError());
            return nullptr;
        }
    }
    float* vertices = static_cast<float*>(SDL_MapGPUTransferBuffer(Device, transferBuffer, false));
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
    SDL_ReleaseGPUTransferBuffer(Device, transferBuffer);
    return buffer;
}

void Renderer::DrawSolidPolygon(b2Transform transform, const b2Vec2* vertices, int count, float radius, b2HexColor color, void* context)
{
    Renderer* renderer = static_cast<Renderer*>(context);
    for (int i = 1; i < count - 1; i++)
    {
        const int kIndices[3] = {0, i, i + 1};
        for (int j = 0; j < 3; j++)
        {
            TransformedVertex vertex;
            vertex.Vertex.Position.x = vertices[kIndices[j]].x;
            vertex.Vertex.Position.y = 0.0f;
            vertex.Vertex.Position.z = vertices[kIndices[j]].y;
            vertex.Vertex.Color = color;
            vertex.Transform.Position.x = transform.p.x;
            vertex.Transform.Position.y = transform.p.y;
            vertex.Transform.Rotation.x = transform.q.s;
            vertex.Transform.Rotation.y = transform.q.c;
            renderer->SolidPolygonBuffer.Emplace(renderer->Device, vertex);
        }
    }
}

void Renderer::DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
{
    Renderer* renderer = static_cast<Renderer*>(context);
    ColorVertex vertex0;
    vertex0.Position.x = p1.x;
    vertex0.Position.y = 0.0f;
    vertex0.Position.z = p1.y;
    vertex0.Color = color;
    ColorVertex vertex1;
    vertex1.Position.x = p2.x;
    vertex1.Position.y = 0.0f;
    vertex1.Position.z = p2.y;
    vertex1.Color = color;
    renderer->LineBuffer.Emplace(renderer->Device, vertex0);
    renderer->LineBuffer.Emplace(renderer->Device, vertex1);
}