#include <algorithm>
#include <cstring>
#include <ncs/containers/archetype.hpp>

namespace ncs
{
    size_t Archetype::append(const Entity entity)
    {
        const size_t row = entity_count++;
        if (row >= entities.size())
        {
            const size_t newsz = entities.empty() ? 16 : entities.size() * 2;
            entities.resize(newsz);
            for (auto &[comp_id, column]: columns)
                column.resize(newsz);
        }

        entities[row] = entity;
        entity_rows[entity] = row;
        flags |= DirtyFlags::ADDED;
        return row;
    }

    bool Archetype::has(const Component c) const
    {
        return std::ranges::find(components, c) != components.end();
    }

    void Archetype::remove(const Entity entity)
    {
        /* entity here is expected to be raw id */
        const auto it = entity_rows.find(entity);
        if (it == entity_rows.end())
            return;

        const size_t row = it->second;
        if (const size_t last_row = entity_count - 1;
            row != last_row)
        {
            /* move the last entity to this row; O(1) for appending last */
            const Entity last_entity = entities[last_row];
            for (auto &[comp_id, column]: columns)
            {
                // Check if both source and destination are constructed
                if (column.is_constructed(last_row) && column.is_constructed(row))
                {
                    if (column.has_copier())
                    {
                        void* dst_ptr = column.get(row);
                        const void* src_ptr = column.get(last_row);
                        column.get_copier()(dst_ptr, src_ptr);
                        column.destroy_at(last_row);
                    }
                    else /* trivially copyable */
                    {
                        void* dst_ptr = column.get(row);
                        const void* src_ptr = column.get(last_row);
                        std::memcpy(dst_ptr, src_ptr, column.size());
                    }
                }
            }

            entities[row] = last_entity;
            entity_rows[last_entity] = row;
        }

        /* patch as removed */
        entity_count--;
        entity_rows.erase(entity);
        flags |= DirtyFlags::REMOVED;
    }

    void Archetype::move(const size_t row, Archetype *dest, const Entity entity)
    {
        const size_t dest_row = dest->append(entity);
        for (Component comp_id: components)
        {
            if (dest->has(comp_id))
            {
                Column &src_col = columns[comp_id];
                Column &dst_col = dest->columns[comp_id];
                if (src_col.is_constructed(row))
                {
                    if (src_col.has_copier())
                    {
                        void* dst_ptr = dst_col.get(dest_row);
                        const void* src_ptr = src_col.get(row);
                        if (dst_ptr && src_ptr)
                            src_col.get_copier()(dst_ptr, src_ptr);
                    }
                    else /* trivial */
                    {
                        void* dst_ptr = dst_col.get(dest_row);
                        const void* src_ptr = src_col.get(row);
                        if (dst_ptr && src_ptr)
                            std::memcpy(dst_ptr, src_ptr, src_col.size());
                    }
                }
            }
        }

        remove(entity);
    }
}
