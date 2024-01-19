#pragma once
#include "Device.h"
#include "ICommandQueue.h"
#include "../IResourceLoader.h"

namespace chil::gfx::d12
{
	class IResourceLoader : public gfx::IResourceLoader {};

	class ResourceLoader : public IResourceLoader
	{
	public:
		ResourceLoader(std::shared_ptr<gfx::IDevice> pDevice);
		std::future<std::shared_ptr<gfx::ITexture>> LoadTexture(std::wstring path) override;
	private:
		std::shared_ptr<d12::IDevice> pDevice_;
		std::shared_ptr<ICommandQueue> pQueue_;
	};
}