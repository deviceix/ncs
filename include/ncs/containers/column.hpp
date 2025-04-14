#pragma once

#include <algorithm>
#include <cstddef>
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

        template<typename T>
        std::size_t construct_at(const std::size_t row)
        {
            if (row >= cap)
                resize(std::max(cap * 2, row + 1));

            void* p = static_cast<char*>(ptr) + (row * sz);
            std::construct_at<T>(static_cast<T*>(p));
            if (row < constructed.size())
                constructed[row] = true;

            return row;
        }

        template<typename T>
        std::size_t construct_at(const std::size_t row, const T& value)
        {
            if (row >= cap)
                resize(std::max(cap * 2, row + 1));

            void* p = static_cast<char*>(ptr) + (row * sz);
            std::construct_at<T>(static_cast<T*>(p), value);
            if (row < constructed.size())
                constructed[row] = true;

            return row;
        }

        template<typename T>
            requires (!std::is_lvalue_reference_v<T>)
        std::size_t construct_at(const std::size_t row, T&& value)
        {
            if (row >= cap)
                resize(std::max(cap * 2, row + 1));

            void* p = static_cast<char*>(ptr) + (row * sz);
            std::construct_at<std::remove_reference_t<T>>(
                static_cast<std::remove_reference_t<T>*>(p),
                std::forward<T>(value)
            );
            if (row < constructed.size())
                constructed[row] = true;

            return row;
        }

        void destroy_at(std::size_t row);

        [[nodiscard]]
        void* get(std::size_t row) const;

        template<typename T>
        T* get_as(const std::size_t row) const
        {
            void* p = get(row);
            return p ? static_cast<T*>(p) : nullptr;
        }

        template<typename T>
        void load()
        {
            sz = sizeof(T);
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                dtor = [](void* ptr)
                {
                    std::destroy_at(static_cast<T*>(ptr));
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
                    std::construct_at(static_cast<T*>(dst), *static_cast<const T*>(src));
                };
            }
            else
            {
                copier = nullptr;
            }

            constructed = std::vector(cap, false);
        }

        [[nodiscard]] std::size_t capacity() const;

        [[nodiscard]] std::size_t size() const;

        [[nodiscard]] bool has_dtor() const;

        [[nodiscard]] bool has_copier() const;

        [[nodiscard]] CopierFn get_copier() const;

        [[nodiscard]] bool is_constructed(std::size_t row) const;

    private:
        void* ptr = nullptr;
        std::size_t sz = 0;
        std::size_t cap = 0;

        CopierFn copier = nullptr;
        DestructorFn dtor = nullptr;

        std::vector<bool> constructed;
    };
}
