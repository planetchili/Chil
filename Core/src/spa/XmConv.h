#pragma once
#include <DirectXMath.h>
#include "Rect.h"
#include "Vec2.h"
#include "Dimensions.h"

namespace chil::spa
{
	template<typename T>
	DirectX::XMFLOAT2 toxm(const Vec2T<T>& v)
	{
		return { (float)v.x, (float)v.y };
	}

	template<typename T>
	DirectX::XMFLOAT2 toxm(const DimensionsT<T>& dims)
	{
		return { (float)dims.width, (float)dims.height };
	}

	template<typename T>
	DirectX::XMFLOAT4 toxm(const RectT<T>& rect)
	{
		return { (float)rect.left, (float)rect.right, (float)rect.right, (float)rect.bottom };
	}
}