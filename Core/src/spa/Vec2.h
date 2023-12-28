#pragma once
#include <cmath>

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
        Vec2T operator-() const
        {
            return Vec2T{ -x, -y };
        }
        Vec2T& operator*=(const T& rhs)
        {
            x *= rhs;
            y *= rhs;
            return *this;
        }
        Vec2T operator*(const T& rhs) const
        {
            return Vec2T{ x * rhs, y * rhs };
        }
        Vec2T operator/(const T& rhs) const
        {
            return Vec2T{ x / rhs, y / rhs };
        }
        Vec2T& operator/=(const T& rhs) const
        {
            x /= rhs;
            y /= rhs;
            return *this;
        }
        bool operator==(const Vec2T& rhs) const
        {
            return x == rhs.x && y == rhs.y;
        }
        T GetLength() const
        {
            return (T)std::sqrt(float(x * x + y * y));
        }
        Vec2T GetNormalized() const
        {
            return *this / GetLength();
        }
        Vec2T GetRotated(float theta) const
        {
            return Vec2T{
                T((float)x * std::cos(theta) - (float)y * std::sin(theta)),
                T((float)x * std::sin(theta) + (float)y * std::cos(theta)),
            };
        }
        // data 
        T x, y;
    };

    using Vec2F = Vec2T<float>;
    using Vec2I = Vec2T<int>;
}
