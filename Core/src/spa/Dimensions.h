#pragma once 

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
        // data 
        T width, height;
    };

    using DimensionsF = DimensionsT<float>;
    using DimensionsI = DimensionsT<int>;
}