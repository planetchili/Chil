#pragma once 
#include "Vec2.h" 
#include "Dimensions.h" 

namespace chil::spa
{
    template<typename T>
    struct RectT
    {
        static RectT FromPointAndDimensions(const Vec2T<T>& topLeft, const DimensionsT<T>& dims)
        {
            return {
                .left = topLeft.x,
                .top = topLeft.y,
                .right = topLeft.x + dims.width,
                .bottom = topLeft.y + dims.height,
            };
        }
        static RectT FromPoints(const Vec2T<T>& topLeft, const Vec2T<T>& bottomRight)
        {
            return {
                .left = topLeft.x,
                .top = topLeft.y,
                .right = bottomRight.x,
                .bottom = bottomRight.y,
            };
        }
        Vec2T<T> GetTopLeft() const { return { left, top }; }
        Vec2T<T> GetBottomRight() const { return { right, bottom }; }
        Vec2T<T> GetTopRight() const { return { right, top }; }
        Vec2T<T> GetBottomLeft() const { return { left, bottom }; }
        DimensionsT<T> GetDimensions() const { return { right - left, bottom - top }; }
        bool Contains(const RectT& inside) const
        {
            return inside.left >= left && inside.top >= top && inside.right <= right && inside.bottom <= bottom;
        }
        // data 
        T left, top, right, bottom;
    };

    using RectF = RectT<float>;
    using RectI = RectT<int>;
}