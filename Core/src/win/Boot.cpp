#include "Boot.h" 
#include <Core/src/ioc/Container.h> 
#include <Core/src/ioc/Singletons.h> 
#include "WindowClass.h" 
#include "Window.h"

// how to forward this for rval goodness? 
// how to do this for const bois? 
template<class T>
auto operator|(std::shared_ptr<T> lhs, std::shared_ptr<T> rhs)
{
	if (bool(lhs)) {
		return std::move(lhs);
	}
	else {
		return std::move(rhs);
	}
}

namespace chil::win
{
	void Boot()
	{
		// container 
		ioc::Get().Register<IWindow>([](IWindow::IocParams args) {
			return std::make_shared<Window>(
				(args.pClass | ioc::Sing().Resolve<IWindowClass>()),
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
