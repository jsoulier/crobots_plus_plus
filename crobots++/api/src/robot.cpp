#include <crobots++/internal.hpp>
#include <crobots++/robot.hpp>

namespace crobots
{

IRobot::IRobot()
{
}

void IRobot::SetSpeed(float speed)
{
    Context->Speed = speed;
}

float IRobot::GetSpeed()
{
    return {};
}

float IRobot::GetX()
{
    return Context->X;
}

float IRobot::GetY()
{
    return Context->Y;
}

void IRobot::Fire(float angle, float range)
{
}

std::optional<float> IRobot::Scan(float angle, float width)
{
    return {};
}

float IRobot::GetHeat()
{
    return {};
}

void IRobot::CoolDown()
{
}

float IRobot::GetDamage()
{
    return 0.0f;
}

float IRobot::GetTime()
{
    return {};
}

}