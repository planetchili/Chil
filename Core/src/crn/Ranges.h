#pragma once
#include <ranges>

namespace chil::crn
{
	namespace rn = std::ranges;
	namespace vi = std::views;

	template<class T>
	auto Cast() { return vi::transform([](auto v) { return T(v); }); }
}