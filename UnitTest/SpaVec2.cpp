#include "ChilCppUnitTest.h"
#include <Core/src/spa/Vec2.h>
#include "SpaToString.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace spa;
using namespace std::string_literals;

namespace Spa
{
	TEST_CLASS(SpaVec2Tests)
	{
	public:
		TEST_METHOD(Addition)
		{
			// Arrange
			Vec2T<int> a{ 1, 2 };
			Vec2T<int> b{ 3, 4 };
			// Act
			Vec2T<int> result = a + b;
			// Assert
			Assert::AreEqual(Vec2T<int>(4, 6), result);
		}
		TEST_METHOD(Subtraction)
		{
			// Arrange
			Vec2T<int> a{ 1, 2 };
			Vec2T<int> b{ 3, 4 };
			// Act
			Vec2T<int> result = a - b;
			// Assert
			Assert::AreEqual(Vec2T<int>(-2, -2), result);
		}
		TEST_METHOD(AdditionAssignment)
		{
			// Arrange
			Vec2T<int> a{ 1, 2 };
			Vec2T<int> b{ 3, 4 };
			// Act
			a += b;
			// Assert
			Assert::AreEqual(Vec2T<int>(4, 6), a);
		}
		TEST_METHOD(SubtractionAssignment)
		{
			// Arrange
			Vec2T<int> a{ 1, 2 };
			Vec2T<int> b{ 3, 4 };
			// Act
			a -= b;
			// Assert
			Assert::AreEqual(Vec2T<int>(-2, -2), a);
		}
	};
}