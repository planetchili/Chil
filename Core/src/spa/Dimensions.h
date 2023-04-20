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
        // data 
        T width, height;
    };

    using DimensionsF = DimensionsT<float>;
    using DimensionsI = DimensionsT<int>;
}