#pragma once

#include <cstring>
#include <unordered_map>
#include <vector>
#include <ncs/types.hpp>
#include <ncs/base/utils.hpp>
#include <ncs/containers/archetype.hpp>
#include <ncs/containers/query_cache.hpp>

namespace ncs
{
    class World
    {
    public:
        World();

        ~World();

        [[nodiscard("ncs::Entity should not be discarded")]]
        Entity entity();

        void despawn(Entity e);

        template<typename T>
        World* set(Entity e, const T& data);

        template<typename T>
        bool has(Entity e);

        template<typename T>
        T* get(Entity e);

        template<typename T>
        World* remove(Entity e);

        template<typename... Components>
        std::vector<std::tuple<Entity, Components*...>> query();

        /* utils */
        static Entity encode_entity(std::uint64_t eid, Generation egen);

        static std::uint64_t get_eid(Entity e);

        static Generation get_egen(Entity e);

    private:
    	/*
    	 * this may seem unnerving at first, but it is valid. for every unique `T`
    	 * the compiler will generate a separate instantiation of `type_hash<T>()`.
    	 * each instantiation has its own unique static variable and each `id` has
    	 * unique address, therefore this method will return the same address everytime
    	 * when the template parameter is the same
    	 *
    	 * TLDR: this is not reproducible across runs so this may very well be changed
    	 * when a better zero-cost solution comes around
    	 *
    	 */
    	template<typename /* T */>
		static constexpr uint64_t type_hash()
    	{
    		static char id = 0;
    		return reinterpret_cast<uint64_t>(&id);
    	}

    	template<typename T>
	    Component get_cid()
    	{
			const uint64_t th = type_hash<T>();
    		for (const auto it = component_types.find(th);
    			it != component_types.end();)
			{
				return it->second;
			}

    		const Component id = next_cid++;
    		component_types[th] = id;
    		component_sizes[id] = sizeof(T);
    		if constexpr (!std::is_trivially_destructible_v<T>)
    		{
    			cdtors[id] = [](void *ptr)
    			{
    				static_cast<T *>(ptr)->~T();
    			};
    		}
    		return id;
    	}

    	template<typename T>
		T *get_component_ptr(Archetype *archetype, const size_t row)
        {
        	const Component cid = get_cid<T>();
        	const Column &column = archetype->columns.at(cid);
        	return reinterpret_cast<T *>(static_cast<char *>(column.data) + (row * column.size));
        }

        Archetype *create_archetype(const std::vector<Component> &components);

		Archetype *find_archetype(const std::vector<Component> &components);

		Archetype *find_archetype_with(Archetype *source, Component component);

		Archetype *find_archetype_without(Archetype *source, Component component);

		void move_entity(Entity entity, Record &record, Archetype *destination);

        std::unordered_map<uint64_t, Archetype *> archetypes;
        std::unordered_map<Entity, Record> entity_records;
        std::unordered_map<Component, void(*)(void *)> cdtors;
        std::unordered_map<uint64_t, std::pair<void *, void(*)(void *)>> qcaches; /* type-erased query caches */

        std::unordered_map<Entity, Generation> generations; /* a sparse set to track decoded entity's id */
		/* maps entity ids to their index poses in the entity pools */
		std::unordered_map<uint64_t, size_t> entity_indices;
		std::unordered_map<std::uint64_t, Component> component_types; /* map component type to component id */
		std::unordered_map<Component, size_t> component_sizes;          /* stores size of each component type */

		std::vector<Entity> entity_pool; /* available ids */

        Archetype *root_archetype = {}; /* */
        uint64_t alive_count;         /* the current number of alive & active entity */
		uint64_t next_eid;            /* next entity id */
		uint16_t next_cid;            /* next component id */
    };

    template<typename T>
	World *World::set(const Entity e, const T &data)
	{
		const uint64_t entity_id = get_eid(e);
		const Generation gen = get_egen(e);

		/* if it is valid; TODO: wrap with debug macro */
		if (const auto it = generations.find(entity_id);
			it == generations.end() || it->second != gen)
		{
			return this;
		}

		const Component component_id = get_cid<T>();
		if (const auto record_it = entity_records.find(entity_id); /* check if entity exists in any archetype */
			record_it == entity_records.end())
		{
			/* start checking from the root archetype; entity doesn't exist yet */
			Archetype *dst = find_archetype_with(root_archetype, component_id);
			const size_t row = dst->append(entity_id);

			Column &column = dst->columns[component_id];
			if (column.data == nullptr)
			{
				column.size = sizeof(T);
				if (column.capacity < row + 1)
					column.resize(std::max(size_t { 16 }, row + 1));
			}

			if (row >= column.capacity)
				column.resize(std::max(column.capacity * 2, row + 1));

			/* copy data */
			void *raw_ptr = static_cast<char *>(column.data) + (row * column.size);
			if constexpr (std::is_trivially_copyable_v<T>)
				std::memcpy(raw_ptr, &data, sizeof(T));
			else
				std::construct_at(static_cast<T*>(raw_ptr), data);

			entity_records[entity_id] = { dst, row }; /* make a new record */
		}
		else /* path 2: entity exists in the archetype */
		{
			Record &record = record_it->second;
			if (Archetype *current = record.archetype;
				current->has(component_id)) /* just update the data */
			{
				const Column &column = current->columns[component_id];
				const size_t row = record.row;

				if (row >= column.capacity)
				{
					auto &mutable_column = const_cast<Column &>(column);
					mutable_column.resize(std::max(column.capacity * 2, row + 1));
				}

				void *raw_ptr = static_cast<char *>(column.data) + (row * column.size);
				if constexpr (std::is_trivially_copyable_v<T>)
				{
					std::memcpy(raw_ptr, &data, sizeof(T));
				}
				else
				{
					std::destroy_at(static_cast<T*>(raw_ptr));
					std::construct_at(static_cast<T*>(raw_ptr), data);
				}

				current->flags |= DirtyFlags::UPDATED;
			}
			else
			{
				Archetype *destination = find_archetype_with(current, component_id);
				if (Column &column = destination->columns[component_id];
					column.data == nullptr) /* setup col if not */
				{
					column.size = sizeof(T);
					column.resize(std::max(size_t { 16 }, destination->entities.size()));
				}
				move_entity(entity_id, record, destination);

				const size_t row = record.row;
				const Column &updated_column = destination->columns[component_id];
				if (row >= updated_column.capacity)
				{
					auto &mutable_column = const_cast<Column &>(updated_column);
					mutable_column.resize(std::max(updated_column.capacity * 2, row + 1));
				}

				/* set the transferred data in the new archetype */
				void *raw_ptr = static_cast<char *>(updated_column.data) + (row * updated_column.size);
				if constexpr (std::is_trivially_copyable_v<T>)
				{
					std::memcpy(raw_ptr, &data, sizeof(T));
				}
				else
				{
					new(raw_ptr) T(data);
				}
			}
		}

		return this;
	}

    template<typename T>
	T *World::get(const Entity e)
	{
		const uint64_t entity_id = get_eid(e);
		const Generation gen = get_egen(e);

    	/* if it is valid; TODO: wrap with debug macro */
		if (const auto it = generations.find(entity_id);
			it == generations.end() || it->second != gen)
		{
			return nullptr;
		}

		const Component component_id = get_cid<T>();
		const auto it = entity_records.find(entity_id);
		if (it == entity_records.end())
			return nullptr;

		auto &[archetype, row] = it->second;
		Archetype *arch = archetype;
		if (!arch->has(component_id))
			return nullptr;

		const Column &column = arch->columns[component_id];
		return reinterpret_cast<T *>(static_cast<char *>(column.data) + (row * column.size));
	}

	template<typename T>
	bool World::has(const Entity e)
	{
		const uint64_t entity_id = get_eid(e);
		const Generation gen = get_egen(e);

    	/* if it is valid; TODO: wrap with debug macro */
		if (const auto it = generations.find(entity_id);
			it == generations.end() || it->second != gen)
		{
			return false;
		}

		const Component component_id = get_cid<T>();
		const auto it = entity_records.find(entity_id);
		if (it == entity_records.end())
			return false;

		Archetype *archetype = it->second.archetype;
		return archetype->has(component_id);
	}

	template<typename T>
	World *World::remove(const Entity e)
	{
		const uint64_t entity_id = get_eid(e);
		const Generation gen = get_egen(e);

    	/* if it is valid; TODO: wrap with debug macro */
		if (const auto it = generations.find(entity_id);
			it == generations.end() || it->second != gen)
		{
			return this;
		}

		const Component component_id = get_cid<T>();
		const auto it = entity_records.find(entity_id);
		if (it == entity_records.end())
			return this;

		Record &record = it->second;
		Archetype *current = record.archetype;
		if (!current->has(component_id))
			return this;

		if constexpr (!std::is_trivially_destructible_v<T>) /* destroy if not trivial type */
		{
			if (T *component_ptr = get<T>(e))
				component_ptr->~T();
		}

		Archetype *dst = find_archetype_without(current, component_id);
		move_entity(entity_id, record, dst);
		return this;
	}

    template<typename... Components>
	std::vector<std::tuple<Entity, Components *...> > World::query()
	{
		const std::vector<Component> cids = { get_cid<Components>()... };
		const uint64_t qhash = archash(cids);

		auto cache_it = qcaches.find(qhash);
		QueryCache<Components...> *cache = nullptr;

		if (cache_it != qcaches.end())
		{
			cache = static_cast<QueryCache<Components...> *>(cache_it->second.first);
			if (cache->archetype && cache->entity_count == cache->archetype->entity_count &&
			    !has_flag(cache->archetype->flags, DirtyFlags::ADDED | DirtyFlags::REMOVED | DirtyFlags::UPDATED))
			{
				return cache->result;
			}

			if (cache->archetype)
			{
				if (has_flag(cache->archetype->flags, DirtyFlags::ADDED) &&
				    !has_flag(cache->archetype->flags, DirtyFlags::REMOVED | DirtyFlags::UPDATED))
				{
					/* append the new entities */
					for (size_t i = cache->entity_count; i < cache->archetype->entity_count; ++i)
					{
						Entity entity_id = cache->archetype->entities[i];
						const auto gen_it = generations.find(entity_id);
						if (gen_it == generations.end())
							continue;

						Entity encoded_entity = encode_entity(entity_id, gen_it->second);
						cache->result.emplace_back(std::make_tuple(
							encoded_entity,
							get_component_ptr<Components>(cache->archetype, i)...
						));
					}
					cache->entity_count = cache->archetype->entity_count;
					cache->archetype->flags = static_cast<DirtyFlags>(
						static_cast<uint64_t>(cache->archetype->flags) &
						~static_cast<uint64_t>(DirtyFlags::ADDED) /* reset the flag */
					);
					return cache->result;
				}

				if (has_flag(cache->archetype->flags, DirtyFlags::REMOVED) &&
				    !has_flag(cache->archetype->flags, DirtyFlags::ADDED | DirtyFlags::UPDATED))
				{
					/* filter out non-existent entities; entities were removed */
					auto &result = cache->result;
					result.erase(
						std::remove_if(result.begin(), result.end(),
						               [this, cache](const auto &tuple)
						               {
							               Entity encoded_entity = std::get<0>(tuple);
							               uint64_t entity_id = get_eid(encoded_entity);
							               return cache->archetype->entity_rows.find(entity_id) == cache->archetype->
							                      entity_rows.end();
						               }),
						result.end()
					);
					cache->entity_count = cache->archetype->entity_count;
					cache->archetype->flags = static_cast<DirtyFlags>(
						static_cast<uint64_t>(cache->archetype->flags) &
						~static_cast<uint64_t>(DirtyFlags::REMOVED) /* reset the flag */
					);
					return cache->result;
				}
				if (has_flag(cache->archetype->flags, DirtyFlags::UPDATED))
				{
					cache->archetype->flags = static_cast<DirtyFlags>(
						static_cast<uint64_t>(cache->archetype->flags) &
						~static_cast<uint64_t>(DirtyFlags::UPDATED)
					);
				}
			}
		}
		else
		{
			cache = new QueryCache<Components...>();
			qcaches[qhash] = {
				cache,
				[](void *ptr)
				{
					delete static_cast<QueryCache<Components...> *>(ptr);
				}
			};
		}

		/* clear existing results if this is a cache rebuild */
		std::vector<std::tuple<Entity, Components *...>>().swap(cache->result);

		/* populate the query */
		for (const auto &[hash, arch]: archetypes)
		{
			auto valid = true;
			for (const Component cid: cids)
			{
				if (!arch->has(cid))
				{
					valid = false;
					break;
				}
			}

			if (!valid)
				continue;

			/* patch & cache the result */
			cache->archetype = arch;
			cache->entity_count = arch->entity_count;
			for (size_t i = 0; i < arch->entity_count; ++i)
			{
				Entity entity_id = arch->entities[i];
				const auto gen_it = generations.find(entity_id);
				if (gen_it == generations.end())
					continue;

				Entity encoded_entity = encode_entity(entity_id, gen_it->second);
				cache->result.emplace_back(std::make_tuple(
					encoded_entity,
					get_component_ptr<Components>(arch, i)...
				));
			}
		}

		return cache->result;
	}
}
