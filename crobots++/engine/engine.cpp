#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <crobots++/internal.hpp>
#include <crobots++/robot.hpp>
#include <glm/glm.hpp>

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>
#include <limits>

#include "engine.hpp"

static constexpr const char* kNewRobot = "NewRobot";
static constexpr float kEpsilon = std::numeric_limits<float>::epsilon();
static constexpr float kWidth = 20.0f;
static constexpr float kP = 5.0f;

static constexpr b2Vec2 kSpawns[8] =
{
    {kWidth / 4 * 1, kWidth / 2 * 1},
    {kWidth / 4 * 3, kWidth / 2 * 1},
    {kWidth / 2, kWidth / 4 * 1},
    {kWidth / 2, kWidth / 4 * 3},
    {kWidth / 4 * 1, kWidth / 4 * 1},
    {kWidth / 4 * 3, kWidth / 4 * 1},
    {kWidth / 4 * 3, kWidth / 4 * 3},
    {kWidth / 4 * 1, kWidth / 4 * 3},
};

EngineParams::EngineParams()
    : Robots{}
    , Timestep{0.016f}
{
}

Engine::Engine()
    : Robots{}
    , Projectiles{}
    , SharedObjects{}
    , WorldID{}
    , ChainBodyID{}
    , Debug{true}
    , Timestep{0.0f}
{
}

bool Engine::Init(const EngineParams& params)
{
    if (params.Robots.size() < 2 || params.Robots.size() > 8)
    {
        SDL_Log("Must have between 2 and 8 (inclusive) robots: %d", params.Robots.size());
        return false;
    }
    if (params.Timestep < kEpsilon)
    {
        SDL_Log("Timestep must be greater than zero");
        return false;
    }
    Timestep = params.Timestep;
    for (const std::string& string : params.Robots)
    {
        Robot robot;
        robot.Context = std::make_shared<crobots::RobotContext>();
        robot.Interface.reset(Load(string, robot.Context));
        if (!robot.Interface)
        {
            SDL_Log("Failed to load robot: %s", string.data());
            return false;
        }
        Robots.push_back(std::move(robot));
    }
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity.x = 0.0f;
        worldDef.gravity.y = 0.0f;
        WorldID = b2CreateWorld(&worldDef);
    }
    int robotID = 0;
    for (Robot& robot : Robots)
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = kSpawns[robotID];
        bodyDef.rotation = b2MakeRot(0.0f);
        robot.BodyID = b2CreateBody(WorldID, &bodyDef);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        b2Polygon polygon = b2MakeBox(0.5f, 0.5f);
        b2CreatePolygonShape(robot.BodyID, &shapeDef, &polygon);
        b2Body_EnableHitEvents(robot.BodyID, true);
        b2Body_EnableContactEvents(robot.BodyID, true);
        robotID++;
    }
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {0.0f, 0.0f};
        ChainBodyID = b2CreateBody(WorldID, &bodyDef);
        b2Vec2 points[4] =
        {
            {0.0f, 0.0f},
            {0.0f, kWidth},
            {kWidth, kWidth},
            {kWidth, 0.0f}
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
        b2CreateChain(ChainBodyID, &chainDef);
        b2Body_EnableHitEvents(ChainBodyID, true);
        b2Body_EnableContactEvents(ChainBodyID, true);
    }
    return true;
}

void Engine::Destroy()
{
    b2DestroyWorld(WorldID);
    Robots.clear();
    Projectiles.clear();
    for (SDL_SharedObject* object : SharedObjects)
    {
        SDL_UnloadObject(object);
    }
}

void Engine::Tick()
{
    for (Robot& robot : Robots)
    {
        robot.Interface->Update(Timestep);
    }
    for (Robot& robot : Robots)
    {
        b2Vec2 linearVelocity = b2Body_GetLinearVelocity(robot.BodyID);
        b2Rot rotation = b2Body_GetRotation(robot.BodyID);
        float mass = b2Body_GetMass(robot.BodyID);
        glm::vec2 velocity;
        velocity.x = rotation.c;
        velocity.y = rotation.s;
        velocity *= robot.Context->Speed;
        velocity.x -= linearVelocity.x;
        velocity.y -= linearVelocity.y;
        glm::vec2 force = velocity * kP;
        // TODO: timestep
        float maxForce = mass * robot.Context->Acceleration;
        if (glm::length(force) > maxForce)
        {
            force *= maxForce / glm::length(force);
        }
        b2Body_ApplyForceToCenter(robot.BodyID, {force.x, force.y}, true);
    }
    b2World_Step(WorldID, Timestep, 4);
    b2ContactEvents contactEvents = b2World_GetContactEvents(WorldID);
    for (int i = 0; i < contactEvents.hitCount; i++)
    {
        b2ContactHitEvent& event = contactEvents.hitEvents[i];
        b2BodyId body1 = b2Shape_GetBody(event.shapeIdA);
        b2BodyId body2 = b2Shape_GetBody(event.shapeIdB);
        auto update = [](b2BodyId body)
        {
            b2Vec2 position = b2Body_GetPosition(body);
            b2Vec2 linearVelocity = b2Body_GetLinearVelocity(body);
            glm::vec2 velocity;
            velocity.x = linearVelocity.x;
            velocity.y = linearVelocity.y;
            if (glm::length(velocity) < kEpsilon)
            {
                return;
            }
            velocity = glm::normalize(velocity);
            b2Rot rotation;
            rotation.c = velocity.x;
            rotation.s = velocity.y;
            b2Body_SetTransform(body, position, rotation);
        };
        if (B2_ID_EQUALS(body1, ChainBodyID))
        {
            update(body2);
        }
        else if (B2_ID_EQUALS(body2, ChainBodyID))
        {
            update(body1);
        }
        b2Body_SetAngularVelocity(body1, 0.0f);
        b2Body_SetAngularVelocity(body2, 0.0f);
    }
    for (int i = 0; i < contactEvents.endCount; i++)
    {
        b2ContactEndTouchEvent& event = contactEvents.endEvents[i];
        if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB))
        {
            continue;
        }
        b2BodyId body1 = b2Shape_GetBody(event.shapeIdA);
        b2BodyId body2 = b2Shape_GetBody(event.shapeIdB);
        b2Body_SetAngularVelocity(body1, 0.0f);
        b2Body_SetAngularVelocity(body2, 0.0f);
    }
    for (Robot& robot : Robots)
    {
        b2Vec2 position = b2Body_GetPosition(robot.BodyID);
        robot.Context->X = position.x;
        robot.Context->Y = position.y;
    }
}

const std::vector<Robot>& Engine::GetRobots() const
{
    return Robots;
}

const std::vector<Projectile> Engine::GetProjectiles() const
{
    return Projectiles;
}

b2WorldId Engine::GetWorldID() const
{
    return WorldID;
}

float Engine::GetWidth() const
{
    return kWidth;
}

void Engine::SetDebug(bool debug)
{
    Debug = debug;
}

bool Engine::GetDebug() const
{
    return true;
}

crobots::IRobot* Engine::Load(const std::string_view& name, const std::shared_ptr<crobots::RobotContext>& context)
{
    std::filesystem::path path = SDL_GetBasePath();
    path /= name;
#if defined(SDL_PLATFORM_WIN32)
    path.replace_extension(".dll");
#elif defined(SDL_PLATFORM_LINUX)
    path.replace_extension(".so");
#elif defined(SDL_PLATFORM_APPLE)
    path.replace_extension(".dylib");
#endif
    SDL_SharedObject* object = SDL_LoadObject(path.string().data());
    if (!object)
    {
        SDL_Log("Failed to load robot: %s, %s", path.string().data(), SDL_GetError());
        return nullptr;
    }
    using Function = crobots::IRobot*(*)(const std::shared_ptr<crobots::RobotContext>& context);
    Function function = reinterpret_cast<Function>(SDL_LoadFunction(object, kNewRobot));
    if (!function)
    {
        SDL_Log("Failed to load %s: %s, %s", kNewRobot, name.data(), SDL_GetError());
        SDL_UnloadObject(object);
        return nullptr;
    }
    crobots::IRobot* robot = function(context);
    if (!robot)
    {
        SDL_Log("Failed to create robot: %s, %s", name.data(), SDL_GetError());
        SDL_UnloadObject(object);
        return nullptr;
    }
    SharedObjects.push_back(object);
    return robot;
}