#pragma once

#include <optional>

#if defined(_WIN32)
#define ROBOT_ENTRYPOINT extern "C" __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define ROBOT_ENTRYPOINT extern "C" __attribute__((visibility("default")))
#else
#define ROBOT_ENTRYPOINT extern "C"
#endif

#define ROBOT(T) \
    ROBOT_ENTRYPOINT IRobot* NewRobot() \
    { \
        return new T(); \
    } \

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

class Seconds
{

};

class Celsius
{

};

template<typename NumT, typename DenT>
struct Ratio
{

};

using MetersPerSecond = Ratio<Meters, Seconds>;
using FeetPerSecond = Ratio<Feet, Seconds>;

class IRobot
{
protected:
    IRobot();

public:
    IRobot(const IRobot& other) = delete;
    IRobot& operator=(const IRobot& other) = delete;
    IRobot(IRobot&& other) = delete;
    IRobot& operator=(IRobot&& other) = delete;
    ~IRobot() = default;
    virtual void Update() = 0;
    void SetSpeed(MetersPerSecond speed);
    MetersPerSecond GetSpeed();
    Radians GetRotation();
    Meters GetX();
    Meters GetY();
    void Fire(Radians radians, Meters range);
    std::optional<Meters> Scan(Radians angle, Radians width);
    Celsius GetHeat();
    void CoolDown();
    float GetDamage();
    Seconds GetTime();

private:
    Seconds TimeSlice;
};