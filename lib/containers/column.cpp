#include <cstdlib>
#include <cstring>
#include <new>
#include <ncs/containers/column.hpp>

namespace ncs
{
	Column::Column() = default;

	Column::~Column()
	{
		if (data)
		{
			std::free(data);
			data = nullptr;
		}
	}

	Column::Column(const Column &other) : size(other.size)
	{
		if (other.data && other.capacity > 0)
		{
			data = std::malloc(size * other.capacity);
			if (data)
				std::memcpy(data, other.data, size * other.capacity);
			capacity = other.capacity;
		}
	}

	Column::Column(Column &&other) noexcept : data(other.data), size(other.size), capacity(other.capacity)
	{
		other.data = nullptr;
		other.size = 0;
		other.capacity = 0;
	}

	Column &Column::operator=(const Column &other)
	{
		if (this != &other)
		{
			if (data)
			{
				std::free(data);
				data = nullptr;
			}

			size = other.size;
			capacity = other.capacity;

			if (other.data && other.capacity > 0)
			{
				data = std::malloc(size * capacity);
				if (data)
					std::memcpy(data, other.data, size * capacity);
			}
		}
		return *this;
	}

	Column &Column::operator=(Column &&other) noexcept
	{
		if (this != &other)
		{
			if (data)
				std::free(data);

			data = other.data;
			size = other.size;
			capacity = other.capacity;

			/* leave empty */
			other.data = nullptr;
			other.size = 0;
			other.capacity = 0;
		}
		return *this;
	}

	void Column::resize(const size_t nsz)
	{
		if (nsz <= capacity)
			return;

		if (data == nullptr)
		{
			data = std::malloc(size * nsz);
			if (!data)
				throw std::bad_alloc();
		}
		else /* realloc existing */
		{
			void *new_data = std::malloc(size * nsz);
			if (!new_data)
				throw std::bad_alloc();

			if (capacity > 0 && size > 0)
				std::memcpy(new_data, data, size * capacity);

			std::free(data);
			data = new_data;
		}
		capacity = nsz;
	}

	void Column::clear()
	{
		if (data)
		{
			std::free(data);
			data = nullptr;
		}
		capacity = 0;
	}

	void *Column::get(const size_t row) const
	{
		if (row >= capacity || !data)
			return nullptr;

		return static_cast<char *>(data) + (row * size);
	}
}
