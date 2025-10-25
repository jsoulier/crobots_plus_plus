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
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "buffer.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "registry.hpp"

static constexpr float kPhysicsY = 0.0f;
static constexpr float kWorldWidth = 20.0f;

struct LineVertex
{
    glm::vec3 Position;
    uint32_t Color;
};

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
static SDL_GPUGraphicsPipeline* linePipeline;
static SDL_GPUGraphicsPipeline* solidPolygonPipeline;
static SDL_GPUBuffer* cubeBuffer;
static DynamicBuffer<Instance> instanceBuffer;
static DynamicBuffer<LineVertex> lineBuffer;
static DynamicBuffer<SolidPolygonVertex> solidPolygonBuffer;
static Camera camera;
static Registry registry;
static std::vector<Robot> robots;
static std::vector<Projectile> projectiles;
static b2WorldId worldId;
static b2DebugDraw debugDraw;
static b2BodyId chainBodyId;
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
    SDL_PropertiesID props = SDL_CreateProperties();
#ifndef NDEBUG
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
#else
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
#endif
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, true);
    device = SDL_CreateGPUDeviceWithProperties(props);
    if (!device)
    {
        SDL_Log("Failed to create device: %s", SDL_GetError());
        return false;
    }
    SDL_DestroyProperties(props);
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
    linePipeline = CreateLinePipeline(device, window);
    if (!linePipeline)
    {
        SDL_Log("Failed to create line pipeline");
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
                    vertex.Position.y = kPhysicsY;
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
        debugDraw.DrawSegmentFcn = [](b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
        {
            LineVertex vertex0;
            vertex0.Position.x = p1.x;
            vertex0.Position.y = kPhysicsY;
            vertex0.Position.z = p1.y;
            vertex0.Color = color;
            LineVertex vertex1;
            vertex1.Position.x = p2.x;
            vertex1.Position.y = kPhysicsY;
            vertex1.Position.z = p2.y;
            vertex1.Color = color;
            lineBuffer.Emplace(device, vertex0);
            lineBuffer.Emplace(device, vertex1);
        };
        debugDraw.drawShapes = true;
    }
    for (Robot& robot : robots)
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {kWorldWidth / 2, kWorldWidth / 2};
        float rotation = 0.6f;
        bodyDef.rotation.c = std::cosf(rotation);
        bodyDef.rotation.s = std::sinf(rotation);
        // bodyDef.angularDamping = 0.5f;
        robot.BodyId = b2CreateBody(worldId, &bodyDef);
        if (B2_IS_NULL(robot.BodyId))
        {
            SDL_Log("Failed to create box2d body");
            return false;
        }
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        // shapeDef.material.friction = 0.0f;
        b2Polygon box = b2MakeBox(0.5f, 0.5f);
        b2CreatePolygonShape(robot.BodyId, &shapeDef, &box);
        b2Body_EnableContactEvents(robot.BodyId, true);
        b2Body_EnableHitEvents(robot.BodyId, true);
    }
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {0.0f, 0.0f};
        chainBodyId = b2CreateBody(worldId, &bodyDef);
        b2Vec2 points[4] =
        {
            {0.0f, 0.0f},
            {0.0f, kWorldWidth},
            {kWorldWidth, kWorldWidth},
            {kWorldWidth, 0.0f}
        };
        b2SurfaceMaterial materials[4]{};
        for (int i = 0; i < 4; i++)
        {
            materials[i].friction = 0.0f;
            materials[i].restitution = 1.0f;
        }
        b2ChainDef chainDef = b2DefaultChainDef();
        chainDef.points = points;
        chainDef.count = 4;
        chainDef.materials = materials;
        chainDef.materialCount = 4;
        chainDef.isLoop = true;
        b2CreateChain(chainBodyId, &chainDef);
        // TODO:
        b2Body_EnableContactEvents(chainBodyId, true);
        b2Body_EnableHitEvents(chainBodyId, true);
    }
    camera.SetCenter(kWorldWidth / 2, kWorldWidth / 2);
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
        b2Rot rotation = b2Body_GetRotation(robot.BodyId);
        glm::vec2 velocity;
        velocity.x = rotation.c;
        velocity.y = rotation.s;
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
    b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
    for (int i = 0; i < contactEvents.hitCount; i++)
    {
        b2ContactHitEvent& event = contactEvents.hitEvents[i];
        if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB))
        {
            continue;
        }
        b2BodyId body1 = b2Shape_GetBody(event.shapeIdA);
        b2BodyId body2 = b2Shape_GetBody(event.shapeIdB);
        auto updateTransform = [](b2BodyId body)
        {
            b2Vec2 position = b2Body_GetPosition(body);
            b2Vec2 velocity = b2Body_GetLinearVelocity(body);
            glm::vec2 glmVelocity;
            glmVelocity.x = velocity.x;
            glmVelocity.y = velocity.y;
            if (glm::length(glmVelocity) < std::numeric_limits<float>::epsilon())
            {
                return;
            }
            glmVelocity = glm::normalize(glmVelocity);
            b2Rot rotation;
            rotation.c = glmVelocity.x;
            rotation.s = glmVelocity.y;
            b2Body_SetTransform(body, position, rotation);
            b2Body_SetAngularVelocity(body, 0.0f);
            // b2Transform transform;
            // transform.p = position;
            // transform.q = rotation;
            // b2Body_SetTargetTransform(body, transform, timestep);
        };
        if (B2_ID_EQUALS(body1, chainBodyId))
        {
            updateTransform(body2);
        }
        else if (B2_ID_EQUALS(body2, chainBodyId))
        {
            updateTransform(body1);
        }
    }
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
    if (true)
    {
        b2World_Draw(worldId, &debugDraw);
    }
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
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!copyPass)
    {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return;
    }
    instanceBuffer.Upload(device, copyPass);
    lineBuffer.Upload(device, copyPass);
    solidPolygonBuffer.Upload(device, copyPass);
    SDL_EndGPUCopyPass(copyPass);
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
    if (lineBuffer.GetSize())
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
        vertexBuffer.buffer = lineBuffer.GetBuffer();
        SDL_BindGPUGraphicsPipeline(renderPass, linePipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBuffer, 1);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &camera.GetViewProj(), 64);
        SDL_DrawGPUPrimitives(renderPass, lineBuffer.GetSize(), 1, 0, 0);
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
    uint64_t time2 = SDL_GetTicks();
    uint64_t time1 = time2;
    while (running)
    {
        time2 = SDL_GetTicks();
        float dt = time2 - time1;
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
                    camera.Rotate(event.motion.xrel, event.motion.yrel);
                }
                else if (event.motion.state & SDL_BUTTON_LMASK)
                {
                    camera.Rotate(event.motion.xrel, event.motion.yrel);
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                camera.Scroll(event.wheel.y);
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
        camera.Move(delta.x, delta.y, delta.z, dt);
        Tick();
        Draw();
    }
    b2DestroyWorld(worldId);
    solidPolygonBuffer.Destroy(device);
    lineBuffer.Destroy(device);
    instanceBuffer.Destroy(device);
    SDL_ReleaseGPUTexture(device, depthTexture);
    SDL_ReleaseGPUBuffer(device, cubeBuffer);
    SDL_ReleaseGPUGraphicsPipeline(device, solidPolygonPipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, linePipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, cubePipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    robots.clear();
    projectiles.clear();
    registry.Destroy();
    return 0;
}