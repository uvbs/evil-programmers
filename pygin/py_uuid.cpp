#include "headers.hpp"

#include "py_uuid.hpp"

#include "py_common.hpp"
#include "py_import.hpp"
#include "py_string.hpp"
#include "py_type.hpp"

#include "error_handling.hpp"
#include "types_cache.hpp"

#include "python.hpp"

using namespace py::literals;

namespace
{
	static std::wstring UuidToString(const UUID& Uuid)
	{
		RPC_WSTR RpcStr;
		const auto Result = UuidToString(&Uuid, &RpcStr);
		if (Result != RPC_S_OK)
			throw MAKE_PYGIN_EXCEPTION("UuidToString returned " + std::to_string(Result));

		std::wstring Str = reinterpret_cast<const wchar_t*>(RpcStr);
		RpcStringFree(&RpcStr);

		return Str;
	}

	static UUID UuidFromString(const std::wstring& Str)
	{
		UUID Uuid;
		const auto Result = UuidFromString(reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(Str.data())), &Uuid);
		if (Result != RPC_S_OK)
			throw MAKE_PYGIN_EXCEPTION("UuidFromString returned " + std::to_string(Result));

		return Uuid;
	}
}

namespace py
{
	const type& uuid::get_type()
	{
		return types_cache::get_type(types::uuid, []()
		{
			return type(cast_guard{}, uuid(UUID{}));
		});
	}

	uuid::uuid(const UUID& Uuid):
		object(import::import("uuid"_py).get_attribute("UUID")(string(UuidToString(Uuid))))
	{
	}

	uuid::uuid(cast_guard, const object& Object):
		object(Object)
	{
	}

	uuid::operator UUID() const
	{
		const auto Bytes = (*this)["bytes_le"];
		UUID Result;
		std::memcpy(&Result, invoke(PyBytes_AsString, Bytes.get()), sizeof(Result));
		return Result;
	}
}
