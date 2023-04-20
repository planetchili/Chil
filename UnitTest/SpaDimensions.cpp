#include "ChilCppUnitTest.h"
#include <Core/src/spa/Dimensions.h>
#include "SpaToString.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace spa;
using namespace std::string_literals;

namespace Spa
{
	TEST_CLASS(SpaDimensionsTests)
	{
	public:
		TEST_METHOD(GetArea)
		{
			// Arrange
			DimensionsT<int> dims{ 5, 10 };
			// Act
			int result = dims.GetArea();
			// Assert
			Assert::AreEqual(50, result);
		}
	};
}