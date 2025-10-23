#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <box2d/box2d.h>
#include <crobots++/internal.hpp>
#include <crobots++/robot.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "buffer.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "registry.hpp"
#include "shader.hpp"

struct SolidPolygonVertex
{
    glm::vec3 Position;
    uint32_t Color;
    glm::vec2 WorldPosition;
    float Sin;
    float Cos;
};

struct Instance
{
    glm::mat4 Matrix;
};

struct Robot
{
    std::unique_ptr<crobots::IRobot> Interface;
    std::shared_ptr<crobots::RobotContext> Context;
    b2BodyId BodyId;
};

struct Projectile
{
    b2BodyId BodyId;
};

static SDL_Window* window;
static SDL_GPUDevice* device;
static SDL_GPUTexture* depthTexture;
static SDL_GPUGraphicsPipeline* cubePipeline;
static SDL_GPUGraphicsPipeline* solidPolygonPipeline;
static SDL_GPUBuffer* cubeBuffer;
static DynamicBuffer<Instance> instanceBuffer;
static DynamicBuffer<SolidPolygonVertex> solidPolygonBuffer;
static Camera camera;
static Registry registry;
static std::vector<Robot> robots;
static std::vector<Projectile> projectiles;
static b2WorldId worldId;
static b2DebugDraw debugDraw;
static float timestep = 0.016f;

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
                robot.Context = std::make_shared<crobots::RobotContext>();
                robot.Interface.reset(registry.Load(inner, robot.Context));
                if (!robot.Interface)
                {
                    continue;
                }
                robots.push_back(std::move(robot));
            }
        }
        else if (outer == "--timestep")
        {
            std::string inner = argv[i];
            try
            {
                timestep = std::stof(inner);
            }
            catch (const std::invalid_argument& e)
            {
                SDL_Log("Failed to parse timestep: %s", e.what());
                return false;
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
    solidPolygonPipeline = CreateSolidPolygonPipeline(device, window);
    if (!solidPolygonPipeline)
    {
        SDL_Log("Failed to create solid polygon pipeline");
        return false;
    }
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity.x = 0.0f;
        worldDef.gravity.y = 0.0f;
        worldId = b2CreateWorld(&worldDef);
        if (B2_IS_NULL(worldId))
        {
            SDL_Log("Failed to create box2d world");
            return false;
        }
        debugDraw = b2DefaultDebugDraw();
        debugDraw.DrawSolidPolygonFcn = [](b2Transform transform, const b2Vec2* vertices,
            int count, float radius, b2HexColor color, void* context)
        {
            for (int i = 1; i < count - 1; i++)
            {
                const int kIndices[3] = {0, i, i + 1};
                for (int j = 0; j < 3; j++)
                {
                    SolidPolygonVertex vertex;
                    vertex.Position.x = vertices[kIndices[j]].x;
                    vertex.Position.y = 1.0f;
                    vertex.Position.z = vertices[kIndices[j]].y;
                    vertex.Color = color;
                    vertex.WorldPosition.x = transform.p.x;
                    vertex.WorldPosition.y = transform.p.y;
                    vertex.Sin = transform.q.s;
                    vertex.Cos = transform.q.c;
                    solidPolygonBuffer.Emplace(device, vertex);
                }
            }
        };
        debugDraw.drawShapes = true;
    }
    for (Robot& robot : robots)
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {0.0f, 0.0f};
        robot.BodyId = b2CreateBody(worldId, &bodyDef);
        if (B2_IS_NULL(robot.BodyId))
        {
            SDL_Log("Failed to create box2d body");
            return false;
        }
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.0f;
        b2Polygon box = b2MakeBox(0.5f, 0.5f);
        b2CreatePolygonShape(robot.BodyId, &shapeDef, &box);
        // b2Body_SetAngularVelocity(robot.BodyId, 0.1f);
        // b2Vec2 velocity;
        // velocity.x = 1.0f;
        // velocity.y = 2.0f;
        // b2Body_SetLinearVelocity(robot.BodyId, velocity);
        // b2Body_SetLinearDamping(robot.BodyId, 0.0f);
    }
    return true;
}

static void Tick()
{
    for (Robot& robot : robots)
    {
        robot.Interface->Update();
    }
    for (Robot& robot : robots)
    {
        glm::vec2 velocity;
        velocity.x = std::cos(robot.Context->Rotation);
        velocity.y = std::sin(robot.Context->Rotation);
        velocity *= robot.Context->Speed;
        b2Vec2 linearVelocity = b2Body_GetLinearVelocity(robot.BodyId);
        velocity.x -= linearVelocity.x;
        velocity.y -= linearVelocity.y;
        static constexpr float kP = 5.0f;
        glm::vec2 force = velocity * kP;
        float mass = b2Body_GetMass(robot.BodyId);
        // TODO: timestep probably needs to be applied here
        float maxForce = mass * robot.Context->Acceleration;
        float forceMagnitude = glm::length(force);
        if (forceMagnitude > maxForce)
        {
            force *= maxForce / forceMagnitude;
        }
        b2Vec2 linearForce;
        linearForce.x = force.x;
        linearForce.y = force.y;
        b2Body_ApplyForceToCenter(robot.BodyId, linearForce, true);
    }
    b2World_Step(worldId, timestep, 4);
    for (Robot& robot : robots)
    {
        b2Vec2 position = b2Body_GetPosition(robot.BodyId);
        robot.Context->X = position.x;
        robot.Context->Y = position.y;
    }
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
    if (camera.GetWidth() != width || camera.GetHeight() != height)
    {
        SDL_ReleaseGPUTexture(device, depthTexture);
        SDL_GPUTextureCreateInfo info{};
        info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        depthTexture = SDL_CreateGPUTexture(device, &info);
        if (!depthTexture)
        {
            SDL_Log("Failed to create depth texture: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            return;
        }
        camera.SetSize(width, height);
    }
    b2World_Draw(worldId, &debugDraw);
    for (Robot& robot : robots)
    {
        b2Vec2 position = b2Body_GetPosition(robot.BodyId);
        b2Rot rotation = b2Body_GetRotation(robot.BodyId);
        glm::mat4 rot = glm::mat4(1.0f);
        rot = glm::rotate(rot, std::atan2(rotation.c, rotation.s), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, 0.0f, position.y));
        glm::mat4 transform = trans * rot;
        instanceBuffer.Emplace(device, transform);
    }
    instanceBuffer.Upload(device, commandBuffer);
    solidPolygonBuffer.Upload(device, commandBuffer);
    camera.Update();
    if (instanceBuffer.GetSize())
    {
        SDL_GPUColorTargetInfo colorInfo{};
        SDL_GPUDepthStencilTargetInfo depthInfo{};
        colorInfo.texture = swapchainTexture;
        colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.texture = depthTexture;
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
        vertexBuffers[0].buffer = cubeBuffer;
        vertexBuffers[1].buffer = instanceBuffer.GetBuffer();
        SDL_BindGPUGraphicsPipeline(renderPass, cubePipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, vertexBuffers, 2);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &camera.GetViewProj(), 64);
        SDL_DrawGPUPrimitives(renderPass, 36, instanceBuffer.GetSize(), 0, 0);
        SDL_EndGPURenderPass(renderPass);
    }
    if (solidPolygonBuffer.GetSize())
    {
        SDL_GPUColorTargetInfo colorInfo{};
        SDL_GPUDepthStencilTargetInfo depthInfo{};
        colorInfo.texture = swapchainTexture;
        colorInfo.load_op = SDL_GPU_LOADOP_LOAD;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthInfo.texture = depthTexture;
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
        vertexBuffer.buffer = solidPolygonBuffer.GetBuffer();
        SDL_BindGPUGraphicsPipeline(renderPass, solidPolygonPipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBuffer, 1);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &camera.GetViewProj(), 64);
        SDL_DrawGPUPrimitives(renderPass, solidPolygonBuffer.GetSize(), 1, 0, 0);
        SDL_EndGPURenderPass(renderPass);
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
            case SDL_EVENT_MOUSE_MOTION:
                if (event.motion.state & SDL_BUTTON_LMASK)
                {
                    camera.Drag(event.motion.xrel, event.motion.yrel);
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                camera.Scroll(event.wheel.y);
                break;
            }
        }
        Tick();
        Draw();
    }
    b2DestroyWorld(worldId);
    solidPolygonBuffer.Destroy(device);
    instanceBuffer.Destroy(device);
    SDL_ReleaseGPUTexture(device, depthTexture);
    SDL_ReleaseGPUBuffer(device, cubeBuffer);
    SDL_ReleaseGPUGraphicsPipeline(device, solidPolygonPipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, cubePipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    robots.clear();
    projectiles.clear();
    registry.Destroy();
    return 0;
}