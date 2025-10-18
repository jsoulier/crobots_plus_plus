#include <crobots++/crobots++.hpp>

class UpDownRobot : public crobots::IRobot
{
public:
    void Update() override
    {
    }
};

CROBOTS_ROBOT(UpDownRobot)