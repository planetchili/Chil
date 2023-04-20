#pragma once
#include <string>
#include <Core/src/spa/Vec2.h>
#include <Core/src/spa/Dimensions.h>
#include <sstream>

namespace Microsoft::VisualStudio::CppUnitTestFramework
{
    template<> inline std::wstring __cdecl
    ToString<chil::spa::Vec2I>(const chil::spa::Vec2I& vec)
    {
        std::wstringstream stream;
        stream << L"Vec2(" << vec.x << L", " << vec.y << L")";
        return stream.str();
    }

    template<> inline std::wstring __cdecl
    ToString<chil::spa::DimensionsI>(const chil::spa::DimensionsI& dims)
    {
        std::wstringstream stream;
        stream << L"Dimensions(" << dims.width << L", " << dims.height << L")";
        return stream.str();
    }
}