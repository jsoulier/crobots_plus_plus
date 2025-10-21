#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

#include "camera.hpp"

static constexpr glm::vec3 kUp{0.0f, 1.0f, 0.0f};

Camera::Camera()
    : Type{CameraType::TopDown}
    , Position{}
    , Forward{}
    , Right{}
    , Up{0.0f, 1.0f, 0.0f}
    , ViewProj{}
    , Width{1}
    , Height{1}
    , Pitch{0.0f}
    , Yaw{0.0f}
    , MoveSpeed{1.0f}
    , RotateSpeed{1.0f}
    , ZoomSpeed{1.0f}
    , Fov{glm::radians(60.0f)}
    , Near{0.1f}
    , Far{100.0f}
{
    SetRotation(0.0f, 0.0f);
}

void Camera::Update()
{
    glm::mat4 proj;
    switch (Type)
    {
    case CameraType::TopDown:
        float width = Width / 100.0f;
        float height = Height / 100.0f;
        proj = glm::ortho(-width, width, height, -height, 0.0f, Far);
        // proj = glm::perspective(Fov, float(Width) / Height, Near, Far);
        break;
    }
    glm::mat4 view = glm::lookAt(Position, Position + Forward, kUp);
    ViewProj = proj * view;
}

void Camera::SetType(CameraType type)
{
    Type = type;
    switch (Type)
    {
    case CameraType::TopDown:
        break;
    }
}

void Camera::SetRotation(float pitch, float yaw)
{
    Pitch = pitch;
    Yaw = yaw;
    Forward.x = std::cos(Pitch) * std::cos(Yaw);
    Forward.y = std::sin(Pitch);
    Forward.z = std::cos(Pitch) * std::sin(Yaw);
    Forward = glm::normalize(Forward);
    Right = glm::cross(Up, Forward);
    Right = glm::normalize(Right);
    Up = glm::cross(Forward, Right);
    Up = glm::normalize(Up);
}

void Camera::SetCenter(float x, float y)
{
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
    case CameraType::TopDown:
        Position += Forward * delta * ZoomSpeed;
        break;
    }
}

void Camera::Drag(float dx, float dy)
{
    switch (Type)
    {
    case CameraType::TopDown:
        Position += Right * dx * MoveSpeed;
        Position += Up * dy * MoveSpeed;
        break;
    }
}

void Camera::Rotate(float dx, float dy)
{
    switch (Type)
    {
    case CameraType::TopDown:
        break;
    }
}

void Camera::Move(float dx, float dy, float dz, float dt)
{
    switch (Type)
    {
    case CameraType::TopDown:
        break;
    }
}

CameraType Camera::GetType() const
{
    return CameraType::TopDown;
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