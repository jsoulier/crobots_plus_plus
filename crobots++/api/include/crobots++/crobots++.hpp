#pragma once

#if defined(_WIN32)
#define CROBOTS_ENTRYPOINT extern "C" __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define CROBOTS_ENTRYPOINT extern "C" __attribute__((visibility("default")))
#else
#define CROBOTS_ENTRYPOINT extern "C"
#endif

#define CROBOTS_ROBOT(T) \
    CROBOTS_ENTRYPOINT ::crobots::IRobot* NewRobot() \
    { \
        return new T(); \
    } \

namespace crobots
{

class Degrees;
class Radians;

struct Degrees
{
    Degrees() = default;
    explicit Degrees(float value);
    explicit Degrees(const Radians& radians);
    operator float() const;

    float Value;
};

struct Radians
{
    Radians() = default;
    explicit Radians(float value);
    explicit Radians(const Degrees& degrees);
    operator float() const;

    float Value;
};

enum class Upgrade
{
};

class IRobot
{
public:

};

}