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
		//// container 
		//ioc::Get().Register<IWindow>([](IWindow::IocParams args) {
		//	return std::make_shared<Window>(
		//		args.pClass ? args.pClass : ioc::Sing().Resolve<IWindowClass>(),
		//		args.pKeySink ? args.pKeySink : ioc::Sing().Resolve<IKeyboardSink>(),
		//		args.name.value_or(L"Main Window"),
		//		args.size.value_or(spa::DimensionsI{ 1280, 720 }),
		//		args.position
		//	);
		//});
		//ioc::Get().Register<gfx::IDevice>([] { // passthru here? 
		//	return std::make_shared<Device>();
		//});
		//ioc::Get().Register<gfx::IResourceLoader>([] { // passthru here? 
		//	return std::make_shared<ResourceLoader>(
		//		ioc::Sing().Resolve<gfx::IDevice>(),
		//		ioc::Get().Resolve<gfx::I
		//	);
		//});

		//// Singleton 
		//ioc::Sing().RegisterPassthru<gfx::IDevice>();
	}
}