#include <crobots++/robot.hpp>

class UpDownRobot : public crobots::IRobot
{
public:
    void Update() override
    {
    }
};

ROBOT(UpDownRobot)