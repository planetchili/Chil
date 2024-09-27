#include "GeneratorCoroMode.h"
#include <ranges>
#include <chrono>
#include <format>
#include <Core/src/log/Log.h> 
#include <Core/src/simp/SimpleContext.h>
#include <Core/src/gfx/SpriteFrame.h>
#include "CliOptions.h"
#include "Sprite.h"
#include <cppcoro/recursive_generator.hpp>

using namespace chil;
namespace co = cppcoro;

co::recursive_generator<int> WaitNextFrame()
{
	co_yield 0;
}

co::recursive_generator<int> MoveSpriteTo(Sprite& sprite, const spa::Vec2F& target)
{
	while (true) {
		if (auto offset = target - sprite.GetPos(); offset.GetLength() >= 1.f) {
			SendSpriteTo(sprite, target);
			co_yield WaitNextFrame();
		}
		else {
			break;
		}
	}
}

co::recursive_generator<int> Triangulate(Sprite& sprite, const spa::Vec2F& center, float scale)
{
	const auto top = center + spa::Vec2F{ 0.f, .5f } * scale;
	const auto right = center + spa::Vec2F{ .5f, -.5f } * scale;
	const auto left = center + spa::Vec2F{ -.5f, -.5f } * scale;
	co_yield MoveSpriteTo(sprite, top);
	co_yield MoveSpriteTo(sprite, right);
	co_yield MoveSpriteTo(sprite, left);
	co_yield MoveSpriteTo(sprite, top);
}

void RunGeneratorCoroMode()
{
	auto& opts = opt::Get();
	simp::Init({ *opts.width, *opts.height }, L"Generator Coro Behavior", *opts.logLevel);
	Sprite s;
	for (auto d : Triangulate(s, {200.f, 200.f}, 100.f)) {
		s.Update();
		simp::Begin();
		s.Draw();
		simp::End();
	}
}