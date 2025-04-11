#include <algorithm>
#include <ncs/world.hpp>

namespace ncs
{
    constexpr std::uint64_t ENTITY_MASK = 0x0000FFFFFFFFFFFF; /* 48 lower bits for entity id */
	constexpr std::uint64_t GENERATION_SHIFT = 48; /* we need to shift 16 bits upper to accommodate the entity bits */
	constexpr Generation MAX_GENERATION = 0xFFFF; /* for 16-bit generation */

    World::World() : root_archetype(create_archetype({})), alive_count(0), next_eid(0), next_cid(0) {}

	World::~World()
	{
		for (auto& [hash, cache_entry] : qcaches)
		{
			auto& [ptr, del] = cache_entry;
			del(ptr);
		}
		qcaches.clear();

		for (auto& [hash, archetype] : archetypes)
		{
			for (auto& [c, edge] : archetype->add_edge)
				delete edge;
			for (auto& [c, edge] : archetype->remove_edge)
				delete edge;

			delete archetype;
		}
		archetypes.clear();
	}

    Entity World::entity()
    {
        Entity entity;
		Generation gen;

		if (alive_count < entity_pool.size())
		{
			/* recycling */
			entity = entity_pool[alive_count]; /* get the entity id to recycle */

			const auto it = generations.find(entity);
			gen = (it != generations.end() ? it->second : 0) + 1;

			/* wrap around if generation is overflow */
			gen = gen > MAX_GENERATION /* 16-bit */ ? 0 : gen;
		}
		else
		{
			/* newborn path */
			entity = next_eid;
			++next_eid;
			gen = 0;

			/* add to the pool */
			entity_pool.emplace_back(entity);
		}

		++alive_count;
		generations[entity] = gen;
		entity_indices[entity] = alive_count - 1; /* store entity's position in the pool */
		return encode_entity(entity, gen);
    }

    void World::despawn(const Entity entity)
	{
		const uint64_t entity_id = get_eid(entity);
		const Generation gen = get_egen(entity);

		/* check if the entity exists with valid generation */
		if (const auto it = generations.find(entity_id);
			it == generations.end() || it->second != gen)
			return;

		/* remove all components */
		if (const auto record_it = entity_records.find(entity_id);
			record_it != entity_records.end())
		{
			Record &record = record_it->second;
			Archetype *archetype = record.archetype;
			const size_t row = record.row;

			/* first call destructors for non-trivial components */
			for (Component comp_id: archetype->components)
			{
				if (auto destructor_it = cdtors.find(comp_id);
					destructor_it != cdtors.end())
				{
					const Column &column = archetype->columns[comp_id];
					void *component_ptr = static_cast<char *>(column.data) + (row * column.size);
					destructor_it->second(component_ptr); /* call ~T() */
				}
			}

			archetype->remove(entity_id) ;/* remove the archetype; note that this cleans up the memory as well */
			entity_records.erase(entity_id); /* clear the record */
		}

		const auto idx_it = entity_indices.find(entity_id);
		if (idx_it == entity_indices.end())
			return;

		if (const size_t index = idx_it->second;
			index < alive_count - 1)
		{
			entity_pool[index] = entity_pool[alive_count - 1];
			entity_indices[entity_pool[index]] = index;
		}

		entity_pool[alive_count - 1] = entity_id;
		--alive_count;

		/* update generation for reuse */
		generations[entity_id] = (generations[entity_id] + 1) > MAX_GENERATION ? 0 : generations[entity_id] + 1;
		entity_indices.erase(entity_id); /* clean up entity index */
	}

	Archetype *World::find_archetype(const std::vector<Component> &components)
	{
		std::vector<Component> sorted_components = components;
		std::ranges::sort(sorted_components);

		const uint64_t hash = archash(sorted_components);
		if (const auto it = archetypes.find(hash);
			it != archetypes.end())
			return it->second;

		return nullptr;
	}

	Archetype *World::find_archetype_without(Archetype *source, const Component component)
	{
		if (const auto edge_it = source->remove_edge.find(component);
			edge_it != source->remove_edge.end() && edge_it->second->to != nullptr)
			return edge_it->second->to;

		if (!source->has(component))
			return source;

		/* create new component list */
		std::vector<Component> new_components;
		new_components.reserve(source->components.size() - 1);
		for (Component c: source->components)
		{
			if (c != component)
				new_components.emplace_back(c);
		}

		Archetype *target = find_archetype(new_components);
		if (!target)
			target = create_archetype(new_components);

		/* cache the edge for future use*/
		auto *edge = new GraphEdge();
		edge->from = source;
		edge->to = target;
		edge->id = component;

		source->remove_edge[component] = edge;
		return target;
	}

	void World::move_entity(const Entity entity, Record &record, Archetype *destination)
	{
		const uint64_t entity_id = get_eid(entity);
		Archetype *source = record.archetype;
		if (source == destination)
			return;

		const size_t src_row = record.row;
		const size_t dest_row = destination->append(entity_id);
		for (Component comp: source->components)
		{
			if (destination->has(comp))
			{
				const Column &src_col = source->columns[comp];
				const Column &dst_col = destination->columns[comp];

				memcpy(
					static_cast<char *>(dst_col.data) + (dest_row * dst_col.size),
					static_cast<char *>(src_col.data) + (src_row * src_col.size),
					src_col.size
				);
			}
		}

		/* patch */
		source->remove(entity_id);
		record.archetype = destination;
		record.row = dest_row;
		entity_records[entity_id] = record;
	}

}
