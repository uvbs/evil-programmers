#pragma once

#include "py_type.hpp"

namespace py
{
	class list: public object, public proxy_owner<list, size_t>
	{
	public:
		using proxy_owner<list, size_t>::operator[];

		static const type& get_type();

		explicit list(size_t Size);
		list(cast_guard, const object& Object);

		void set_at(size_t Index, const object& Value);
		object get_at(size_t Index) const;

		size_t size() const;
		void push_back(const object& Object);
		void insert(const object& Object, size_t index);
	};
}