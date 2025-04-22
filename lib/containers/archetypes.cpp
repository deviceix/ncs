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
                if (column.is_constructed(last_row))
                {
                    void* dst = static_cast<char*>(column.get(row));
                    void* src = static_cast<char*>(column.get(last_row));

                    column.destroy_at(row);
                    if (column.has_copier())
                    {
                        /* non-trivial */
                        CopierFn copier = column.get_copier();
                        copier(dst, src);
                        column.mark_constructed(row);
                    }
                    else /* trivially copyable types */
                    {
                        std::memcpy(dst, src, column.size());
                        column.mark_constructed(row);
                    }

                    column.destroy_at(last_row);
                }
            }

            entities[row] = last_entity;
            entity_rows[last_entity] = row;
        }
        else
        {
            /* if we're removing the last entity, just destroy its components */
            for (auto &[comp_id, column]: columns)
                column.destroy_at(row);
        }

        /* clear the last entity */
        entity_count--;
        entity_rows.erase(entity);
        flags |= DirtyFlags::REMOVED; /* mark as removed */
    }

    void Archetype::move(const size_t row, Archetype* dest, const Entity entity)
    {
        const size_t dest_row = dest->append(entity);

        for (Component comp_id: components)
        {
            if (dest->has(comp_id))
            {
                const auto& src_column = columns[comp_id];
                auto& dst_column = dest->columns[comp_id];
                if (src_column.is_constructed(row))
                {
                    const void* src_ptr = src_column.get(row);
                    void* dst_ptr = dst_column.get(dest_row);

                    /* destroy any existing object at destination */
                    dst_column.destroy_at(dest_row);
                    if (src_column.has_copier()) /* non-trivially copyable */
                    {
                        CopierFn copier = src_column.get_copier();
                        copier(dst_ptr, src_ptr);
                        dst_column.mark_constructed(dest_row);
                    }
                    else /* trivially copyable */
                    {
                        std::memcpy(dst_ptr, src_ptr, src_column.size());
                        dst_column.mark_constructed(dest_row);
                    }
                }
            }
        }

        remove(entity);
    }
}
