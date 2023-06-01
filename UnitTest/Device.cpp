#include "ChilCppUnitTest.h"
#include <Core/src/win/Window.h>
#include <Core/src/win/WindowClass.h>
#include <Core/src/gfx/d11/Device.h>
#include <Core/src/gfx/d11/RenderPane.h>


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
			auto pDevice = std::make_shared<gfx::d11::Device>();
			gfx::d11::RenderPane pane{ window.GetHandle(), { 100, 100 }, pDevice };
			Assert::IsTrue(true);
		}
	};
}
