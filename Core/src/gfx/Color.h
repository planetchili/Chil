#pragma once
#include <cstdint>

namespace chil::gfx
{
	template<typename T>
	struct Color
	{
		T r = 0, g = 0, b = 0, a = 0;
		static constexpr Color White()
		{
			return { 255, 255, 255, 255 };
		}
	};

	using Color8 = Color<uint8_t>;
}