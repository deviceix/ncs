/* this file is a part of Naught Engine which is under MIT license; see LICENSE for more info */

#pragma once

#include <cstddef>
#include <ncs/types.hpp>

namespace ncs
{
	struct Column
	{
		void *data = nullptr;
		std::size_t size = 0;
		std::size_t capacity = 0;

		Column();

		~Column();

		Column(const Column &other);

		Column(Column &&other) noexcept;

		Column &operator=(const Column &other);

		Column &operator=(Column &&other) noexcept;

		void resize(std::size_t nsz);

		void clear();

		[[nodiscard]] void *get(std::size_t row) const;
	};
}
