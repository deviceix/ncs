#include <algorithm>
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
				memcpy(
					static_cast<char *>(column.data) + (row * column.size),
					static_cast<char *>(column.data) + (last_row * column.size),
					column.size
				);
			}

			entities[row] = last_entity;
			entity_rows[last_entity] = row;
		}

		/* clear the last entity*/
		entity_count--;
		entity_rows.erase(entity);
		flags |= DirtyFlags::REMOVED; /* mark as removed */
	}

	void Archetype::move(const size_t row, Archetype *dest, const Entity entity)
	{
		const size_t dest_row = dest->append(entity);
		for (Component comp_id: components)
		{
			if (dest->has(comp_id))
			{
				const auto& c1 = columns[comp_id];
				const auto& c2 = dest->columns[comp_id];
				memcpy(static_cast<char *>(c2.data) + (dest_row * c1.size),
					static_cast<char *>(c1.data) + row * c1.size, c1.size);
			}
		}

		remove(entity);
	}
}
