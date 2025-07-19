#pragma once
#include <ranges>
#include <vector>
#include <span>

namespace chil::crn
{
	namespace rn = std::ranges;
	namespace vi = std::views;

	template<class T>
	auto Cast() { return vi::transform([](auto v) { return T(v); }); }

	template<typename T>
	std::span<const T> SpanTemp(const std::vector<T>& v)
	{
		return std::span<const T>{ v };
	}
}