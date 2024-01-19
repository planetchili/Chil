#pragma once
#include "ITexture.h"
#include <string>
#include <future>

namespace chil::gfx
{
	class IResourceLoader
	{
	public:
		virtual ~IResourceLoader() = default;
		virtual std::future<std::shared_ptr<ITexture>> LoadTexture(std::wstring path) = 0;
	};
}
