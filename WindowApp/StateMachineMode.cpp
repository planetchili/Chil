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
	gfx::SpriteFrame frame{ {1, 1}, {0, 0}, simp::LoadAtlas(L"frog.png") };
	while (!simp::Win().IsClosing()) {
		simp::Begin();
		for (float x = -200.f; x <= 200.f; x += 60.f) {
			for (float y = -200.f; y <= 200.f; y += 72.f) {
				frame.DrawToBatch(simp::Batch(), { x, y });
			}
		}
		simp::End();
	}
}