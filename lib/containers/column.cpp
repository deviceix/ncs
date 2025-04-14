#include <cstdlib>
#include <cstring>
#include <ncs/containers/column.hpp>

namespace ncs
{
    Column::~Column()
    {
        if (ptr && dtor)
        {
            for (std::size_t i = 0; i < cap && i < constructed.size(); ++i)
            {
                if (constructed[i])
                    dtor(static_cast<char*>(ptr) + (i * sz));
            }
        }

        if (ptr)
        {
            std::free(ptr);
            ptr = nullptr;
        }
    }

    Column::Column(const Column &other) :
        sz(other.sz), cap(other.cap), copier(other.copier), dtor(other.dtor)
    {
        if (other.ptr && other.cap > 0)
        {
            ptr = std::malloc(other.cap * sz);
            if (!ptr)
                throw std::bad_alloc();

            constructed = other.constructed;

            if (copier)
            {
                for (size_t i = 0; i < cap && i < constructed.size(); ++i)
                {
                    if (constructed[i])
                    {
                        void* src = static_cast<char*>(other.ptr) + (i * sz);
                        void* dst = static_cast<char*>(ptr) + (i * sz);
                        copier(dst, src);
                    }
                }
            }
            else
            {
                std::memcpy(ptr, other.ptr, other.cap * sz);
            }
        }
    }

    Column::Column(Column &&other) noexcept :
        ptr(other.ptr), sz(other.sz), cap(other.cap),
        copier(other.copier), dtor(other.dtor),
        constructed(std::move(other.constructed))
    {
        other.ptr = nullptr;
        other.sz = 0;
        other.cap = 0;
        other.dtor = nullptr;
        other.copier = nullptr;
    }

    Column& Column::operator=(const Column &other)
    {
        if (this != &other)
        {
            clear();

            sz = other.sz;
            dtor = other.dtor;
            copier = other.copier;
            cap = other.cap;

            if (other.ptr && other.cap > 0)
            {
                ptr = std::malloc(sz * other.cap);
                if (!ptr)
                    throw std::bad_alloc();

                constructed = other.constructed;

                if (copier)
                {
                    for (size_t i = 0; i < cap && i < constructed.size(); ++i)
                    {
                        if (constructed[i])
                        {
                            void* src = static_cast<char*>(other.ptr) + (i * sz);
                            void* dst = static_cast<char*>(ptr) + (i * sz);
                            copier(dst, src);
                        }
                    }
                }
                else
                {
                    std::memcpy(ptr, other.ptr, other.cap * sz);
                }
            }
        }
        return *this;
    }

    Column& Column::operator=(Column &&other) noexcept
    {
        if (this != &other)
        {
            clear();

            ptr = other.ptr;
            sz = other.sz;
            cap = other.cap;
            dtor = other.dtor;
            copier = other.copier;
            constructed = std::move(other.constructed);

            other.ptr = nullptr;
            other.sz = 0;
            other.cap = 0;
            other.dtor = nullptr;
            other.copier = nullptr;
        }
        return *this;
    }

    void Column::resize(const std::size_t new_cap)
    {
        if (new_cap <= cap)
            return;

        void* new_ptr = std::malloc(sz * new_cap);
        if (!new_ptr)
            throw std::bad_alloc();

        /* Copy data first, then update ptr, then clean up old memory */
        if (ptr && cap > 0)
        {
            if (copier)
            {
                for (size_t i = 0; i < cap && i < constructed.size(); ++i)
                {
                    if (constructed[i])
                    {
                        void* src = static_cast<char*>(ptr) + (i * sz);
                        void* dst = static_cast<char*>(new_ptr) + (i * sz);
                        copier(dst, src);
                    }
                }
            }
            else
            {
                std::memcpy(new_ptr, ptr, sz * cap);
            }

            void* old_ptr = ptr;
            ptr = new_ptr;  /* Update ptr before destroying objects */

            if (dtor)
            {
                for (size_t i = 0; i < cap && i < constructed.size(); ++i)
                {
                    if (constructed[i])
                    {
                        void* p = static_cast<char*>(old_ptr) + (i * sz);
                        dtor(p);
                    }
                }
            }

            std::free(old_ptr);
        }
        else
        {
            ptr = new_ptr;
        }

        cap = new_cap;
        if (constructed.size() < new_cap)
            constructed.resize(new_cap, false);
    }

    void Column::clear()
    {
        if (ptr && dtor)
        {
            for (size_t i = 0; i < cap && i < constructed.size(); ++i)
            {
                if (constructed[i])
                {
                    void* p = static_cast<char*>(ptr) + (i * sz);
                    dtor(p);
                }
            }
        }

        if (ptr)
        {
            std::free(ptr);
            ptr = nullptr;
        }

        cap = 0;
        constructed.clear();
    }

    void* Column::get(const std::size_t row) const
    {
        if (row >= cap || !ptr || (row < constructed.size() && !constructed[row]))
            return nullptr;
        return static_cast<char*>(ptr) + (row * sz);
    }

    void Column::destroy_at(const std::size_t row)
    {
        if (dtor && row < cap && row < constructed.size() && constructed[row])
        {
            void* p = static_cast<char*>(ptr) + (row * sz);
            dtor(p);
            constructed[row] = false;
        }
    }

    std::size_t Column::capacity() const
    {
        return cap;
    }

    std::size_t Column::size() const
    {
        return sz;
    }

    bool Column::has_dtor() const
    {
        return dtor != nullptr;
    }

    bool Column::has_copier() const
    {
        return copier != nullptr;
    }

    CopierFn Column::get_copier() const
    {
        return copier;
    }

    bool Column::is_constructed(std::size_t row) const
    {
        return row < constructed.size() && constructed[row];
    }
}
