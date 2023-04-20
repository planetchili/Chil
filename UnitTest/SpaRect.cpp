#include "ChilCppUnitTest.h"
#include <Core/src/spa/Rect.h>
#include "SpaToString.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace spa;
using namespace std::string_literals;

namespace Spa
{
	TEST_CLASS(SpaRectTests)
	{
	public:
		TEST_METHOD(FromPointAndDimensions)
		{
			// Arrange
			Vec2T<int> topLeft{ 0, 0 };
			DimensionsT<int> dims{ 5, 10 };

			// Act
			RectT<int> rect = RectT<int>::FromPointAndDimensions(topLeft, dims);

			// Assert
			Assert::AreEqual(topLeft, rect.GetTopLeft());
			Assert::AreEqual(Vec2T<int>(5, 10), rect.GetBottomRight());
			Assert::AreEqual(0, rect.left);
			Assert::AreEqual(0, rect.top);
			Assert::AreEqual(5, rect.right);
			Assert::AreEqual(10, rect.bottom);
		}

		TEST_METHOD(FromPoints)
		{
			// Arrange
			Vec2T<int> topLeft{ 0, 0 };
			Vec2T<int> bottomRight{ 5, 10 };

			// Act
			RectT<int> rect = RectT<int>::FromPoints(topLeft, bottomRight);

			// Assert
			Assert::AreEqual(topLeft, rect.GetTopLeft());
			Assert::AreEqual(bottomRight, rect.GetBottomRight());
			Assert::AreEqual(0, rect.left);
			Assert::AreEqual(0, rect.top);
			Assert::AreEqual(5, rect.right);
			Assert::AreEqual(10, rect.bottom);
		}

		TEST_METHOD(GetTopLeft)
		{
			// Arrange
			Vec2T<int> topLeft{ 0, 0 };
			DimensionsT<int> dims{ 5, 10 };
			RectT<int> rect = RectT<int>::FromPointAndDimensions(topLeft, dims);

			// Act
			Vec2T<int> result = rect.GetTopLeft();

			// Assert
			Assert::AreEqual(topLeft, result);
		}

		TEST_METHOD(GetBottomRight)
		{
			// Arrange
			Vec2T<int> topLeft{ 0, 0 };
			DimensionsT<int> dims{ 5, 10 };
			RectT<int> rect = RectT<int>::FromPointAndDimensions(topLeft, dims);

			// Act
			Vec2T<int> result = rect.GetBottomRight();

			// Assert
			Assert::AreEqual(Vec2T<int>(5, 10), result);
		}

		TEST_METHOD(GetTopRight)
		{
			// Arrange
			Vec2T<int> topLeft{ 0, 0 };
			DimensionsT<int> dims{ 5, 10 };
			RectT<int> rect = RectT<int>::FromPointAndDimensions(topLeft, dims);

			// Act
			Vec2T<int> result = rect.GetTopRight();

			// Assert
			Assert::AreEqual(Vec2T<int>(5, 0), result);
		}

		TEST_METHOD(GetBottomLeft)
		{
			// Arrange
			Vec2T<int> topLeft{ 0, 0 };
			DimensionsT<int> dims{ 5, 10 };
			RectT<int> rect = RectT<int>::FromPointAndDimensions(topLeft, dims);
			// Act
			Vec2T<int> result = rect.GetBottomLeft();
			// Assert
			Assert::AreEqual(Vec2T<int>(0, 10), result);
		}

		TEST_METHOD(GetDimensions)
		{
			// Arrange
			Vec2T<int> topLeft{ 0, 0 };
			DimensionsT<int> dims{ 5, 10 };
			RectT<int> rect = RectT<int>::FromPointAndDimensions(topLeft, dims);
			// Act
			DimensionsT<int> result = rect.GetDimensions();
			// Assert
			Assert::AreEqual(dims, result);
		}
	};
}