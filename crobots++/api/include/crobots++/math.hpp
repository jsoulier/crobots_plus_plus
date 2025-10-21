#pragma once

namespace crobots
{

class Degrees;
class Feet;
class Meters;
class Radians;
class Seconds;
class Celsius;
class Kilograms;
class Pounds;

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

class Kilograms
{

};

class Pounds
{

};

template<typename NumT, typename DenT>
struct Ratio
{

};

using MetersPerSecond = Ratio<Meters, Seconds>;
using FeetPerSecond = Ratio<Feet, Seconds>;

}