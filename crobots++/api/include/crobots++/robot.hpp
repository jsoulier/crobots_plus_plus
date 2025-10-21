#pragma once

#include <crobots++/math.hpp>

#include <optional>

#if defined(_WIN32)
#define ROBOT_ENTRYPOINT extern "C" __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define ROBOT_ENTRYPOINT extern "C" __attribute__((visibility("default")))
#else
#define ROBOT_ENTRYPOINT extern "C"
#endif

#define ROBOT(T) \
    ROBOT_ENTRYPOINT ::crobots::IRobot* NewRobot(::crobots::RobotContext* context) \
    { \
        return ::crobots::IRobot::Create<T>(context); \
    } \

namespace crobots
{

struct RobotContext
{
    Meters X;
    Meters Y;
    Radians Rotation;
};

class IRobot
{
protected:
    IRobot();

public:
    template<typename T>
    static IRobot* Create(RobotContext* context)
    {
        IRobot* robot = new T();
        robot->Context = context;
        return robot;
    }

    IRobot(const IRobot& other) = delete;
    IRobot& operator=(const IRobot& other) = delete;
    IRobot(IRobot&& other) = delete;
    IRobot& operator=(IRobot&& other) = delete;
    ~IRobot() = default;
    virtual void Update() = 0;
    void SetSpeed(MetersPerSecond speed);
    MetersPerSecond GetSpeed();
    Radians GetRotation();
    Meters GetX();
    Meters GetY();
    void Fire(Radians radians, Meters range);
    std::optional<Meters> Scan(Radians angle, Radians width);
    Celsius GetHeat();
    void CoolDown();
    float GetDamage();
    Seconds GetTime();

private:
    RobotContext* Context;
};

}