#include <crobots++/robot.hpp>

class Robot : public crobots::IRobot
{
public:
    void Update(float deltaTime) override
    {
        SetSpeed(10.0f);
    }
};

CROBOTS_ROBOT(Robot)