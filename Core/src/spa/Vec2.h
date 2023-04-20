#pragma once 

namespace chil::spa
{
    template<typename T>
    struct Vec2T
    {
        // functions 
        Vec2T& operator+=(const Vec2T& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }
        Vec2T& operator-=(const Vec2T& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }
        Vec2T operator+(const Vec2T& rhs) const
        {
            return Vec2T{ *this } += rhs;
        }
        Vec2T operator-(const Vec2T& rhs) const
        {
            return Vec2T{ *this } -= rhs;
        }
        Vec2T& operator*=(const T& rhs)
        {
            x *= rhs;
            y *= rhs;
            return *this;
        }
        Vec2T operator*(const T& rhs)
        {
            return Vec2T{ *this } *rhs;
        }
        bool operator==(const Vec2T& rhs) const
        {
            return x == rhs.x && y == rhs.y;
        }
        // data 
        T x, y;
    };

    using Vec2F = Vec2T<float>;
    using Vec2I = Vec2T<int>;
}
