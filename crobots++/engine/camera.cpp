#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

#include "camera.hpp"

static constexpr glm::vec3 kUp{0.0f, 1.0f, 0.0f};
static constexpr float kMaxPitch = glm::pi<float>() / 2.0f - 0.01f;

Camera::Camera()
    : Type{CameraType::ArcBall}
    , Center{}
    , Position{}
    , Forward{}
    , Right{}
    , Up{}
    , ViewProj{}
    , Width{1}
    , Height{1}
    , Pitch{0.0f}
    , Yaw{0.0f}
    , MoveSpeed{0.1f}
    , RotateSpeed{0.01f}
    , ZoomSpeed{2.0f}
    , Fov{glm::radians(60.0f)}
    , Near{0.1f}
    , Far{500.0f}
{
    SetRotation(0.0f, 0.0f);
}

void Camera::Update()
{
    glm::mat4 proj;
    glm::mat4 view;
    switch (Type)
    {
    case CameraType::ArcBall:
        proj = glm::perspective(Fov, float(Width) / Height, Near, Far);
        break;
    }
    switch (Type)
    {
    case CameraType::ArcBall:
        view = glm::lookAt(Position, Position + Forward, kUp);
        break;
    }
    ViewProj = proj * view;
}

void Camera::SetType(CameraType type)
{
    Type = type;
    switch (Type)
    {
    case CameraType::ArcBall:
        break;
    }
}

void Camera::SetCenter(float x, float y)
{
    Center = {x, 0.0f, y};
    Position = Center;
}

void Camera::SetRotation(float pitch, float yaw)
{
    Pitch = std::clamp(pitch, -kMaxPitch, kMaxPitch);
    Yaw = yaw;
    Forward.x = std::cos(Pitch) * std::cos(Yaw);
    Forward.y = std::sin(Pitch);
    Forward.z = std::cos(Pitch) * std::sin(Yaw);
    Forward = glm::normalize(Forward);
    Right = glm::cross(kUp, Forward);
    Right = glm::normalize(Right);
    Up = glm::cross(Forward, Right);
    Up = glm::normalize(Up);
    switch (Type)
    {
    case CameraType::ArcBall:
        Position = Center - Forward * (glm::distance(Position, Center));
        break;
    }
}

void Camera::SetSize(int width, int height)
{
    Width = std::max(width, 1);
    Height = std::max(height, 1);
}

void Camera::Scroll(float delta)
{
    switch (Type)
    {
    case CameraType::ArcBall:
        Position += Forward * delta * ZoomSpeed;
        break;
    }
}

void Camera::Drag(float dx, float dy)
{
    switch (Type)
    {
    case CameraType::ArcBall:
        SetRotation(Pitch - dy * RotateSpeed, Yaw + dx * RotateSpeed);
        break;
    }
}

void Camera::Rotate(float dx, float dy)
{
    switch (Type)
    {
    case CameraType::ArcBall:
        break;
    }
}

void Camera::Move(float dx, float dy, float dz, float dt)
{
    switch (Type)
    {
    case CameraType::ArcBall:
        break;
    }
}

CameraType Camera::GetType() const
{
    return Type;
}

const glm::vec3& Camera::GetPosition() const
{
    return Position;
}

const glm::mat4& Camera::GetViewProj() const
{
    return ViewProj;
}

int Camera::GetWidth() const
{
    return Width;
}

int Camera::GetHeight() const
{
    return Height;
}