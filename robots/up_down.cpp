#include <crobots++/robot.hpp>

class UpDownRobot : public crobots::IRobot
{
public:
    void Update() override
    {
        SetSpeed(5.0f);
    }
};

CROBOTS_ROBOT(UpDownRobot)