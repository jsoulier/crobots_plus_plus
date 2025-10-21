#include <crobots++/robot.hpp>

namespace crobots
{

IRobot::IRobot()
{
}

void IRobot::SetSpeed(MetersPerSecond speed)
{
}

MetersPerSecond IRobot::GetSpeed()
{
    return {};
}

Radians IRobot::GetRotation()
{
    return Context->Rotation;
}

Meters IRobot::GetX()
{
    return Context->X;
}

Meters IRobot::GetY()
{
    return Context->Y;
}

void IRobot::Fire(Radians radians, Meters range)
{
}

std::optional<Meters> IRobot::Scan(Radians angle, Radians width)
{
    return {};
}

Celsius IRobot::GetHeat()
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

Seconds IRobot::GetTime()
{
    return {};
}

}