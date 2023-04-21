#pragma once
#include <string>
#include <format>
#include <Core/src/spa/Vec2.h>
#include <Core/src/spa/Dimensions.h>

namespace chil::spa
{
    template<typename T>
    std::wstring ToString(const Vec2T<T>& vec)
    {
        return std::format(L"Vec2({}, {})", vec.x, vec.y);
    }
    template<typename T>
    std::wstring ToString(const DimensionsT<T>& dims)
    {
        return std::format(L"Dimensions({}, {})", dims.width, dims.height);
    }
}

using chil::spa::ToString;