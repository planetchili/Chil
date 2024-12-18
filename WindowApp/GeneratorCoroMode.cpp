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
#include <Core/src/net/Net.h>

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

co::recursive_generator<int> Serpentine(Sprite& sprite, float width, float height)
{
	const auto start = sprite.GetPos();
	const auto a = start + spa::Vec2F{ width, 0.f };
	const auto b = a - spa::Vec2F{ 0.f, height };
	const auto c = b - spa::Vec2F{ width, 0.f };
	const auto d = c - spa::Vec2F{ 0.f, height };
	co_yield MoveSpriteTo(sprite, a);
	co_yield MoveSpriteTo(sprite, b);
	co_yield MoveSpriteTo(sprite, c);
	co_yield MoveSpriteTo(sprite, d);
}

co::recursive_generator<int> Behavior(Sprite& sprite, int reps)
{
	for (int i = 0; i < reps; i++) {
		co_yield Triangulate(sprite, { 100.f, 100.f }, 200.f);
		co_yield Triangulate(sprite, { -100.f, 100.f }, 100.f);
		for (int i = 0; i < 4; i++) {
			co_yield Triangulate(sprite, { -200.f, -200.f }, 50.f);
		}
		co_yield MoveSpriteTo(sprite, { -400.f, 300.f });
		for (int j = 0; j < 3; j++) {
			co_yield Serpentine(sprite, 200.f, 50.f);
		}
	}
}

void RunGeneratorCoroMode()
{
	auto pClient = net::IClient::Make();

	auto& opts = opt::Get();
	simp::Init({ *opts.width, *opts.height }, L"Generator Coro Behavior", *opts.logLevel);
	Sprite s;
	for (auto d : Behavior(s, 3)) {
		if (simp::Win().IsClosing()) break;
		s.Update();
		simp::Begin();
		s.Draw();
		simp::End();
	}
}