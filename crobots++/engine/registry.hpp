#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crobots
{

class IRobot;
class RobotContext;

}

class Registry
{
public:
    crobots::IRobot* Load(const std::string_view& name, const std::shared_ptr<crobots::RobotContext>& context);
    void Destroy();

private:
    using NewRobotFunction = crobots::IRobot*(*)(const std::shared_ptr<crobots::RobotContext>& context);

    struct Entry
    {
        Entry() = default;

        Entry(const std::string_view& name, SDL_SharedObject* object, NewRobotFunction function)
            : Name{name}
            , Object{object}
            , Function{function}
        {
        }

        Entry(Entry&& other)
            : Name{std::move(other.Name)}
            , Object{other.Object}
            , Function{other.Function}
        {
        }

        std::string Name;
        SDL_SharedObject* Object;
        NewRobotFunction Function;
    };

    std::vector<Entry> Entries;
};
