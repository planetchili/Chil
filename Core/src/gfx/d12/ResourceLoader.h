#pragma once
#include "Device.h"
#include "ICommandQueue.h"
#include "../IResourceLoader.h"
#include "Texture.h"
#include <functional>

namespace chil::gfx::d12
{
	class IResourceLoader : public gfx::IResourceLoader {};

	class ResourceLoader : public IResourceLoader
	{
	public:
		// types
		using TextureFactory = std::function<std::shared_ptr<d12::ITexture>(d12::ITexture::IocParams params)>;
		// functions
		ResourceLoader(std::shared_ptr<gfx::IDevice> pDevice, TextureFactory textureFactory);
		std::future<std::shared_ptr<gfx::ITexture>> LoadTexture(std::wstring path) override;
	private:
		std::shared_ptr<d12::IDevice> pDevice_;
		std::shared_ptr<ICommandQueue> pQueue_;
		TextureFactory textureFactory_;
	};
}