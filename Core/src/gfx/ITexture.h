#pragma once


namespace chil::gfx
{
	class ITexture
	{
	public:
		virtual ~ITexture() = default;
		virtual spa::DimensionsI GetDimensions() const = 0;
	};
}