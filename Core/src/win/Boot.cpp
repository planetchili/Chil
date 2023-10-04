#include "Boot.h" 
#include <Core/src/ioc/Container.h> 
#include <Core/src/ioc/Singletons.h> 
#include "WindowClass.h" 
#include "Window.h"
#include "Input.h"

namespace chil::win
{
	void Boot()
	{
		// container 
		ioc::Get().Register<IWindow>([](IWindow::IocParams args) {
			return std::make_shared<Window>(
				args.pClass ? args.pClass : ioc::Sing().Resolve<IWindowClass>(),
				args.pKeySink ? args.pKeySink : ioc::Sing().Resolve<IKeyboardSink>(),
				args.name.value_or(L"Main Window"),
				args.size.value_or(spa::DimensionsI{ 1280, 720 }),
				args.position
			);
		});
		ioc::Get().Register<IWindowClass>([] { // passthru here? 
			return std::make_shared<WindowClass>();
		});

		// Singleton 
		ioc::Sing().RegisterPassthru<IWindowClass>();
	}
}
