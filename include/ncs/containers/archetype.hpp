#pragma once

#include <unordered_map>
#include <vector>
#include <ncs/types.hpp>
#include <ncs/containers/column.hpp>

namespace ncs
{
	struct Column;
	struct Archetype;
	struct GraphEdge;
	struct Record;

	struct GraphEdge
	{
		Archetype* from;
		Archetype* to;
		Component id; /* what component causes the transition */
	};

	struct Record
	{
		Archetype* archetype = nullptr;
		size_t row = 0;
	};

	struct Archetype
	{
		/* graph structure */
		std::unordered_map<Component, GraphEdge*> add_edge;
		std::unordered_map<Component, GraphEdge*> remove_edge;

		std::unordered_map<Entity, size_t> entity_rows;
		std::unordered_map<Component, Column> columns;
		std::vector<Component> components;
		std::vector<Entity> entities;
		size_t entity_count = 0;
		uint64_t id = 0;
		DirtyFlags flags = {};

		[[nodiscard]] bool has(Component c) const;

		size_t append(Entity entity);

		void remove(Entity entity);

		void move(size_t row, Archetype* dest, Entity entity);
	};
}
