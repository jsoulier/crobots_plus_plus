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
class Feet;
class Meters;
class Radians;

class Meters
{
    friend class Feet;

public:
    Meters() = default;
    explicit Meters(float value);
    explicit Meters(const Feet& feet);
    float GetValue() const;

private:
    float Value;
};

class Feet
{
    friend class Meters;

public:
    Feet() = default;
    explicit Feet(float value);
    explicit Feet(const Meters& meters);
    float GetValue() const;

private:
    float Value;
};

class Degrees
{
    friend class Radians;

public:
    Degrees() = default;
    explicit Degrees(float value);
    explicit Degrees(const Radians& radians);
    float GetValue() const;

private:
    float Value;
};

class Radians
{
    friend class Degrees;

public:
    Radians() = default;
    explicit Radians(float value);
    explicit Radians(const Degrees& degrees);
    float GetValue() const;

private:
    float Value;
};

class IRobot
{
public:
    IRobot() = default;
    virtual ~IRobot() = default;
    virtual void Update() = 0;

protected:
    Meters Scan(Radians angle, Radians width);
};

}