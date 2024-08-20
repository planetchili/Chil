#pragma once
#include <utility>

namespace chil::spa
{
    template<typename T>
    struct DimensionsT
    {
        // functions
        T GetArea() const
        {
            return width * height;
        }
        bool operator==(const DimensionsT& rhs) const
        {
			return width == rhs.width && height == rhs.height;
		}
        DimensionsT operator/(T divisor) const
        {
            return { width / divisor, height / divisor };
        }
        template<typename S>
        operator DimensionsT<S>() const
        {
            return { (S)width, (S)height };
        }
        DimensionsT operator*(const DimensionsT& rhs) const
        {
            return { width * rhs.width, height * rhs.height };
        }
        static DimensionsT FromPair(const std::pair<T, T>& p)
        {
            return DimensionsT{ p.first, p.second };
        }
        // data 
        T width, height;
    };

    using DimensionsF = DimensionsT<float>;
    using DimensionsI = DimensionsT<int>;
}