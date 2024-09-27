#include "StateMachineMode.h"
#include <ranges>
#include <chrono>
#include <format>
#include <Core/src/log/Log.h> 
#include <Core/src/simp/SimpleContext.h>
#include <Core/src/gfx/SpriteFrame.h>
#include "CliOptions.h"
#include "Sprite.h"

using namespace chil;

void RunStateMachineMode()
{
	auto& opts = opt::Get();
	simp::Init({ *opts.width, *opts.height }, L"State Machine Behavior", *opts.logLevel);
	Sprite s;
	s.SetDir({ 1.f, 0.f });
	while (!simp::Win().IsClosing()) {
		s.Update();
		simp::Begin();
		s.Draw();
		simp::End();
	}
}