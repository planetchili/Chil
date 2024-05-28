#pragma once
#include <Core/src/spa/Dimensions.h>

namespace Global
{
#ifdef NDEBUG
	inline constexpr unsigned int nCharacters = 350'000;
#else
	inline constexpr unsigned int nCharacters = 500;
#endif
	inline constexpr size_t nBatches = 4;
	inline constexpr int nSheets = 32;
	inline constexpr auto outputDims = chil::spa::DimensionsI{ 1280, 720 };
	inline constexpr int nWindows = 1;
	inline constexpr unsigned int seed = 42069;
	inline constexpr int framesToRunFor = 0;
	//inline constexpr int framesToRunFor = 0;
}