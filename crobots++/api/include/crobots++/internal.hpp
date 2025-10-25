#pragma once

namespace crobots
{

class RobotContext
{
public:
    RobotContext()
        : X{0.0f}
        , Y{0.0f}
        , Speed{0.0f}
        , Acceleration{1.0f}
    {
    }

    float X;
    float Y;
    float Speed;
    float Acceleration;
};

}