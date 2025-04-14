#include <cstdlib>
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
	}

	Column::Column(const Column &other) : sz(other.sz), dtor(other.dtor), copier(other.copier)
	{
		if (other.ptr && other.cap > 0)
		{
			ptr = std::malloc(other.cap * sz);
			if (!ptr)
				throw std::bad_alloc();

			constructed = other.constructed;
			cap = other.cap;
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

	Column::Column(Column &&other) noexcept
		: ptr(other.ptr), sz(other.sz), cap(other.cap), dtor(other.dtor),
		  copier(other.copier), constructed(std::move(other.constructed))
	{
		other.ptr = nullptr;
		other.sz = 0;
		other.cap = 0;
		other.dtor = nullptr;
		other.copier = nullptr;
	}

	Column & Column::operator=(const Column &other)
	{
		if (this != &other)
		{
			this->~Column();

			sz = other.sz;
			dtor = other.dtor;
			copier = other.copier;

			if (other.ptr && other.cap > 0)
			{
				ptr = std::malloc(sz * other.cap);
				if (!ptr)
					throw std::bad_alloc();

				constructed = other.constructed;
				cap = other.cap;

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

	Column & Column::operator=(Column &&other) noexcept
	{
		if (this != &other)
		{
			this->~Column();

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

	void Column::resize(std::size_t new_cap)
	{
		if (new_cap <= cap)
			return;

		void* new_ptr = std::malloc(sz * new_cap);
		if (!new_ptr)
			throw std::bad_alloc();

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

				if (dtor)
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
			}
			else
			{
				std::memcpy(new_ptr, ptr, sz * cap);
			}

			std::free(ptr);
		}

		ptr = new_ptr;
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

	void Column::shrink_to_fit()
	{
		if (!ptr || cap == 0)
			return;

		if (!copier)
		{
			void* new_ptr = std::realloc(ptr, sz * cap);
			if (new_ptr)
				ptr = new_ptr;
		}
		else
		{
			void* new_ptr = std::malloc(sz * cap);
			if (!new_ptr)
				return;

			for (size_t i = 0; i < cap && i < constructed.size(); ++i)
			{
				if (constructed[i])
				{
					void* src = static_cast<char*>(ptr) + (i * sz);
					void* dst = static_cast<char*>(new_ptr) + (i * sz);
					copier(dst, src);
				}
			}

			if (dtor)
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

			std::free(ptr);
			ptr = new_ptr;
		}

		if (constructed.size() > cap)
		{
			constructed.resize(cap);
			constructed.shrink_to_fit();
		}
	}

	void * Column::get(std::size_t row) const
	{
		if (row >= cap || !ptr)
			return nullptr;

		return static_cast<char*>(ptr) + (row * sz);
	}
}
