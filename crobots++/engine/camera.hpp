#pragma once

#include <glm/glm.hpp>

enum class CameraMode
{
    TopDown,
    ArcBall,
    FreeCam,
    Pov,
};

class Camera
{
public:

private:
    CameraMode Mode;
    glm::vec3 Position;
    float Pitch;
    float Yaw;
};