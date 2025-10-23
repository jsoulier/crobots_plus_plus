#pragma once

#include <memory>
#include <optional>

#if defined(_WIN32)
#define CROBOTS_ENTRYPOINT extern "C" __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define CROBOTS_ENTRYPOINT extern "C" __attribute__((visibility("default")))
#else
#define CROBOTS_ENTRYPOINT extern "C"
#endif

#define CROBOTS_ROBOT(T) \
    CROBOTS_ENTRYPOINT crobots::IRobot* NewRobot(const std::shared_ptr<crobots::RobotContext>& context) \
    { \
        return crobots::IRobot::Create<T>(context); \
    } \

namespace crobots
{

class RobotContext;
class IRobot
{
protected:
    IRobot();

public:
    template<typename T>
    static IRobot* Create(const std::shared_ptr<RobotContext>& context)
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
    
    /**
     * @param speed
     */
    void SetSpeed(float speed);

    // meters/second
    float GetSpeed();

    // radians
    float GetRotation();

    // meters
    float GetX();

    // meters
    float GetY();

    void Fire(float angle, float range);

    std::optional<float> Scan(float angle, float width);

    float GetHeat();

    void CoolDown();

    float GetDamage();

    float GetTime();

private:
    std::shared_ptr<RobotContext> Context;
};

}