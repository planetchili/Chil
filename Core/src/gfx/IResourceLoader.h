#pragma once
#include "ITexture.h"
#include <string>
#include <future>

namespace chil::gfx
{
	class IResourceLoader
	{
	public:
		// types
		using FutureTexture = std::future<std::shared_ptr<ITexture>>;
		// functions
		virtual ~IResourceLoader() = default;
		virtual FutureTexture LoadTexture(std::wstring path) = 0;
	};
}
