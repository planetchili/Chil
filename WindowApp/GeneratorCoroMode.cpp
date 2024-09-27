#include "GeneratorCoroMode.h"
#include <ranges>
#include <chrono>
#include <format>
#include <Core/src/log/Log.h> 
#include <Core/src/simp/SimpleContext.h>
#include <Core/src/gfx/SpriteFrame.h>
#include "CliOptions.h"
#include "Sprite.h"

using namespace chil;

void RunGeneratorCoroMode()
{
	auto& opts = opt::Get();
	simp::Init({ *opts.width, *opts.height }, L"Generator Coro Behavior", *opts.logLevel);
	Sprite s;
	while (!simp::Win().IsClosing()) {
		s.Update();
		simp::Begin();
		s.Draw();
		simp::End();
	}
}