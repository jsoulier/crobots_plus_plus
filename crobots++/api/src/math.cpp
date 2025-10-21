#include <crobots++/math.hpp>

namespace crobots
{

Meters::Meters(float value)
    : Value{value}
{
}

Meters::Meters(const Feet& feet)
    : Value{feet.Value * 0.3048f}
{
}

float Meters::GetValue() const
{
    return Value;
}

Feet::Feet(float value)
    : Value{value}
{
}

Feet::Feet(const Meters& meters)
    : Value{meters.Value * 3.28084f}
{
}

float Feet::GetValue() const
{
    return Value;
}

Degrees::Degrees(float value)
    : Value{value}
{
}

Degrees::Degrees(const Radians& radians)
    : Value{radians.Value * 57.2958f}
{
}

float Degrees::GetValue() const
{
    return Value;
}

Radians::Radians(float value)
    : Value{value}
{
}

Radians::Radians(const Degrees& degrees)
    : Value{degrees.Value * 0.0174533f}
{
}

float Radians::GetValue() const
{
    return Value;
}

}