#pragma once

#include <glm/glm.hpp>

enum class CameraType
{
    ArcBall,
};

class Camera
{
public:
    Camera();
    void Update();
    void SetType(CameraType type);
    void SetCenter(float x, float y);
    void SetRotation(float pitch, float yaw);
    void SetSize(int width, int height);
    void Scroll(float delta);
    void Drag(float dx, float dy);
    void Rotate(float dx, float dy);
    void Move(float dx, float dy, float dz, float dt);
    CameraType GetType() const;
    const glm::vec3& GetPosition() const;
    const glm::mat4& GetViewProj() const;
    int GetWidth() const;
    int GetHeight() const;

private:
    CameraType Type;
    glm::vec3 Center;
    glm::vec3 Position;
    glm::vec3 Forward;
    glm::vec3 Right;
    glm::vec3 Up;
    glm::mat4 ViewProj;
    int Width;
    int Height;
    float Pitch;
    float Yaw;
    float MoveSpeed;
    float RotateSpeed;
    float ZoomSpeed;
    float Fov;
    float Near;
    float Far;
};