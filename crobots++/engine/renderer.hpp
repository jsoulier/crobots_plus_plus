#pragma once

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string_view>

#include "buffer.hpp"

class Camera;
class Engine;

class Renderer
{
public:
    Renderer();
    bool Init(SDL_Window* window);
    void Destroy();
    void Draw(const Engine& engine, Camera& camera);

private:
    SDL_GPUShader* LoadShader(const std::string_view &name);
    SDL_GPUGraphicsPipeline* CreateInstancedPipeline();
    SDL_GPUGraphicsPipeline* CreateLinePipeline();
    SDL_GPUGraphicsPipeline* CreateSolidPolygonPipeline();
    SDL_GPUBuffer* CreateCubeBuffer();
    static void DrawSolidPolygon(b2Transform transform, const b2Vec2* vertices, int count, float radius, b2HexColor color, void* context);
    static void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context);

    struct ColorVertex
    {
        glm::vec3 Position;
        uint32_t Color;
    };

    struct NormalVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
    };

    struct VertexTransform
    {
        glm::vec2 Position;
        glm::vec2 Rotation;
    };

    struct TransformedVertex
    {
        ColorVertex Vertex;
        VertexTransform Transform;
    };

    struct Instance
    {
        glm::mat4 Matrix;
        // color
    };

    SDL_Window* Window;
    SDL_GPUDevice* Device;
    SDL_GPUTexture* DepthTexture;
    SDL_GPUGraphicsPipeline* InstancedPipeline;
    SDL_GPUGraphicsPipeline* LinePipeline;
    SDL_GPUGraphicsPipeline* SolidPolygonPipeline;
    SDL_GPUBuffer* CubeBuffer;
    DynamicBuffer<Instance, SDL_GPU_BUFFERUSAGE_VERTEX> InstanceBuffer;
    DynamicBuffer<ColorVertex, SDL_GPU_BUFFERUSAGE_VERTEX> LineBuffer;
    DynamicBuffer<TransformedVertex, SDL_GPU_BUFFERUSAGE_VERTEX> SolidPolygonBuffer;
    b2DebugDraw DebugDraw;
};