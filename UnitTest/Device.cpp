#include "ChilCppUnitTest.h"
#include <Core/src/win/Window.h>
#include <Core/src/win/WindowClass.h>
#include <Core/src/gfx/d11/Device.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace std::string_literals;

namespace Gfx
{
	TEST_CLASS(GfxTestDevice)
	{
	public:
		// testing text formatting
		TEST_METHOD(TestConstruction)
		{
			win::Window window{ std::make_shared<win::WindowClass>(), L"Gfx Device Test Window", {100, 100} };
			gfx::d11::Device device{};
			Assert::IsTrue(true);
		}
	};
}
