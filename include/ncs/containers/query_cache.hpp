#pragma once

#include <cstddef>
#include <ncs/containers/archetype.hpp>

namespace ncs
{
    template<typename... Components>
    struct QueryCache
    {
        Archetype *archetype = nullptr; /* strong pointer to the archetype */
        std::size_t entity_count = 0;        /* entity count at the time of caching */
        std::vector<std::tuple<Entity, Components *...> > result;
    };
}
