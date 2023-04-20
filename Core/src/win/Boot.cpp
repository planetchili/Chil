#include "Boot.h" 
#include <Core/src/ioc/Container.h> 
#include <Core/src/ioc/Singletons.h> 
#include "WindowClass.h" 


namespace chil::win
{
	void Boot()
	{
		// container 
		ioc::Get().Register<IWindowClass>([] { // passthru here? 
			return std::make_shared<WindowClass>();
		});

		// Singleton 
		ioc::Sing().RegisterPassthru<IWindowClass>();
	}
}
