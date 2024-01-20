#include "Boot.h" 
#include <Core/src/ioc/Container.h> 
#include <Core/src/ioc/Singletons.h>
#include "Device.h"
#include "RenderPane.h"
#include "Texture.h"
#include "ResourceLoader.h"
#include "SpriteCodex.h"
#include "SpriteBatcher.h"
#include "CommandQueue.h"


namespace chil::gfx::d12
{
	void Boot()
	{
		// container
		ioc::Get().Register<gfx::IRenderPane>([](gfx::IRenderPane::IocParams p) {
			return std::make_shared<RenderPane>(p.hWnd, p.dims,
				ioc::Sing().Resolve<IDevice>(),
				ioc::Get().Resolve<ICommandQueue>()
			);
		});
		ioc::Get().Register<gfx::IResourceLoader>([] {
			return std::make_shared<ResourceLoader>(
				ioc::Sing().Resolve<IDevice>(),
				[](ITexture::IocParams p) { return ioc::Get().Resolve<ITexture>(std::move(p)); }
			);
		});
		ioc::Get().Register<gfx::ISpriteCodex>([](gfx::ISpriteCodex::IocParams p) {
			return std::make_shared<SpriteCodex>(ioc::Sing().Resolve<IDevice>(), p.maxAtlases);
		});
		ioc::Get().Register<gfx::ISpriteBatcher>([](gfx::ISpriteBatcher::IocParams p) {
			return std::make_shared<SpriteBatcher>(
				p.targetDimensions,
				ioc::Sing().Resolve<IDevice>(),
				std::move(p.pSpriteCodex),
				p.maxSpriteCount
			);
		});
		ioc::Get().Register<ICommandQueue>([] {
			return std::make_shared<CommandQueue>(ioc::Sing().Resolve<IDevice>());
		});
		ioc::Get().Register<ITexture>([](ITexture::IocParams p) {
			return std::make_shared<Texture>(std::move(p));
		});
		ioc::Get().Register<IDevice>([] { // passthru here? make this gfx::IDevice? (no reason, has no useable functions rn)
			return std::make_shared<Device>();
		});

		// Singleton 
		ioc::Sing().RegisterPassthru<IDevice>(); // make this gfx::IDevice?
	}
}