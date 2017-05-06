#pragma once

#include "py_object.hpp"

namespace py
{
	class list: public object
	{
	public:
		TRIVIALLY_MOVABLE(list);

		explicit list(const object& Object);

		class value_proxy
		{
		public:
			TRIVIALLY_MOVABLE(value_proxy);

			value_proxy(list* Owner, size_t Index);
			value_proxy(const value_proxy& rhs);

			value_proxy& operator=(const object& value);
			value_proxy& operator=(const value_proxy& value);

			operator object() const;

		private:
			list* m_Owner;
			size_t m_Index;
		};

		value_proxy operator[](size_t Index);
		void set_at(size_t Index, const object& Value);
		object get_at(size_t Index) const;

		size_t size() const;
		void push_back(const object& Object);
		void insert(const object& Object, size_t index);
	};
}