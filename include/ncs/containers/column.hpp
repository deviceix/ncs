#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

namespace ncs
{
	using CopierFn = void(*)(void*, const void*);
	using DestructorFn = void(*)(void*);

	class Column
	{
	public:
		Column() = default;

		~Column();

		Column(const Column& other);

		Column(Column&& other) noexcept;

		Column& operator=(const Column& other);

		Column& operator=(Column&& other) noexcept;

		void resize(std::size_t new_cap);

		void clear();

		void shrink_to_fit();

		[[nodiscard]]
		void* get(std::size_t row) const;

		template<typename T>
		void load()
		{
			sz = sizeof(T);
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				dtor = [](void* ptr)
				{
					static_cast<T*>(ptr)->~T();
				};
			}
			else
			{
				dtor = nullptr;
			}

			if constexpr (!std::is_trivially_copyable_v<T>)
			{
				copier = [](void* dst, const void* src)
				{
					new (dst) T(*static_cast<const T*>(src));
				};
			}
			else
			{
				copier = nullptr;
			}

			constructed = std::vector(cap, false);
		}

		template<typename T>
		T* construct_at(const std::size_t row)
		{
			if (row >= cap)
				resize(std::max(cap * 2, row + 1));

			void* p = static_cast<char*>(ptr) + (row * sz);
			T* result = std::construct_at<T>(static_cast<T*>(p));
			if (dtor && row < constructed.size())
				constructed[row] = true;

			return result;
		}

		template<typename T>
		T* construct_at(const std::size_t row, const T& value)
		{
			if (row >= cap)
				resize(std::max(cap * 2, row + 1));

			void* p = static_cast<char*>(ptr) + (row * sz);
			T* result = std::construct_at<T>(static_cast<T*>(p), value);
			if (dtor && row < constructed.size())
				constructed[row] = true;

			return result;
		}

		template<typename T>
		T* construct_at(const std::size_t row, T&& value)
		{
			if (row >= cap)
				resize(std::max(cap * 2, row + 1));

			void* p = static_cast<char*>(ptr) + (row * sz);
			T* result = std::construct_at<T>(static_cast<T*>(p), std::forward<T>(value));
			if (dtor && row < constructed.size())
				constructed[row] = true; /* mark as constructed */

			return result;
		}

		void destroy_at(const std::size_t row)
		{
			if (dtor && row < cap && row < constructed.size() && constructed[row])
			{
				void* p = static_cast<char*>(ptr) + (row * sz);
				dtor(p);
				constructed[row] = false;
			}
		}

		template<typename T>
		T* get_as(const std::size_t row) const
		{
			void* p = get(row);
			return p ? static_cast<T*>(p) : nullptr;
		}

		[[nodiscard]] std::size_t capacity() const
		{
			return cap;
		}

		[[nodiscard]] std::size_t size() const
		{
			return sz;
		}

		[[nodiscard]] bool has_dtor() const
		{
			return dtor != nullptr;
		}

		[[nodiscard]] bool has_copier() const
		{
			return copier != nullptr;
		}

		[[nodiscard]] CopierFn get_copier() const
		{
			return copier;
		}

	private:
		void* ptr = nullptr;
		std::size_t sz = 0;
		std::size_t cap = 0;

		/* for type management */
		CopierFn copier = nullptr;
		DestructorFn dtor = nullptr;

		std::vector<bool> constructed;
	};
}
