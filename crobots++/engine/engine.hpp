#pragma once

#include <box2d/box2d.h>
#include <crobots++/internal.hpp>
#include <crobots++/robot.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct EngineParams
{
    EngineParams();

    std::vector<std::string> Robots;
    float Timestep;
};

struct Robot
{
    std::unique_ptr<crobots::IRobot> Interface;
    std::shared_ptr<crobots::RobotContext> Context;
    b2BodyId BodyID;
};

struct Projectile
{
    b2BodyId BodyID;
};

class Engine
{
public:
    Engine();
    bool Init(const EngineParams& params);
    void Destroy();
    void Tick();
    const std::vector<Robot>& GetRobots() const;
    const std::vector<Projectile> GetProjectiles() const;
    b2WorldId GetWorldID() const;
    float GetWidth() const;
    void SetDebug(bool debug);
    bool GetDebug() const;

private:
    crobots::IRobot* Load(const std::string_view& name, const std::shared_ptr<crobots::RobotContext>& context);

    std::vector<Robot> Robots;
    std::vector<Projectile> Projectiles;
    std::vector<SDL_SharedObject*> SharedObjects;
    b2WorldId WorldID;
    b2BodyId ChainBodyID;
    bool Debug;
    float Timestep;
};