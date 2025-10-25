#include <crobots++/robot.hpp>

class Robot : public crobots::IRobot
{
public:
    void Update(float deltaTime) override
    {
    }
};

CROBOTS_ROBOT(Robot)