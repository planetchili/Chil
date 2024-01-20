#pragma once
#include <Core/src/spa/Dimensions.h>

namespace chil::gfx
{
	class ITexture
	{
	public:
		// types
		// This is just here as notes to Chili, not actually putting IoC params into gfx::ITexture see GOOD IDEA below
		//struct IoCParams
		//{
		//	spa::DimensionsI dimensions;
		//	uint32_t mipLevels;
		//	// how to communicate format in the options here?
		//	// options: hardcode only 1 format in the ioc
		//	//			loader creates resource, **somehow inserts into ITexture?? (can downcast inside)
		//	//			IoC entry for d12 texture w/ specific param, somehow connectd to ITexture??
		//	//			create platform-agnostic format codes and platform mappings
		//	//		GOOD IDEA:
		//	//			don't register ITexture at all => not supposed to be resolved: created by factory
		//	//			register d12::ITexture, then ctor can have platform specific
		//	//			factory d12 resolves d12 of course, all good
		//};
		// functions
		virtual ~ITexture() = default;
		virtual spa::DimensionsI GetDimensions() const = 0;
	};
}