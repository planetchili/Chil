#include "ActiveWindow.h"
#include <Core/src/ioc/Container.h>
#include <Core/src/win/IWindow.h>
#include <Core/src/win/Input.h>
#include <Core/src/gfx/IRenderPane.h>
#include <ranges>
#include <numbers>
#include "Sprite.h"
#include "Global.h"

using namespace chil;
using hrclock = std::chrono::high_resolution_clock;
namespace rn = std::ranges;
namespace vi = rn::views;


ActiveWindow::ActiveWindow(int index, std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex)
	:
	thread_{ &ActiveWindow::Kernel_, this, index, std::move(pSpriteCodex) }
{
	constructionSemaphore_.acquire();
	if (hasException_) {
		std::rethrow_exception(exception_);
	}
}

bool ActiveWindow::IsLive() const
{
	if (hasException_) {
		std::rethrow_exception(exception_);
	}
	return isLive;
}

void ActiveWindow::Kernel_(int index, std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex)
{
	try {
		// ioc container shortcut
		auto& C = ioc::Get();
		//// do construction
		// make sprite batchers
		auto batchers = vi::iota(0ull, Global::nBatches) | vi::transform([&](auto i) {
			return C.Resolve<gfx::ISpriteBatcher>(gfx::ISpriteBatcher::IocParams{
				.targetDimensions = Global::outputDims,
				.pSpriteCodex = pSpriteCodex,
				.maxSpriteCount = UINT(Global::nCharacters / Global::nBatches + 1)
				});
		}) | rn::to<std::vector>();
		// make window
		auto keyboard = std::make_shared<win::Keyboard>();
		auto pWindow = C.Resolve<win::IWindow>(win::IWindow::IocParams{
			.pKeySink = keyboard,
			.name = std::format(L"Window #{}", index),
			.size = Global::outputDims,
			});
		// make graphics pane
		auto pPane = C.Resolve<gfx::IRenderPane>(gfx::IRenderPane::IocParams{
			.hWnd = pWindow->GetHandle(),
			.dims = Global::outputDims,
			});
		// signal completion of construction phase
		constructionSemaphore_.release();

		// random engine
		std::minstd_rand0 rne;
		// sprite blueprints
		std::vector<std::shared_ptr<ISpriteBlueprint>> blueprints;
		for (int i = 0; i < Global::nSheets; i++) {
			blueprints.push_back(std::make_shared<SpriteBlueprint>(pSpriteCodex, i, 8, 4));
		}
		// sprite instances
		const auto characters =
			vi::iota(0u, Global::nCharacters) |
			vi::transform([
				&blueprints,
				&rne,
				posDist = std::uniform_real_distribution<float>{ -360.f, 360.f },
				speedDist = std::uniform_real_distribution<float>{ 240.f, 600.f },
				angleDist = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> }
			] (uint32_t i) mutable -> std::unique_ptr<ISpriteInstance> {
			return std::make_unique<SpriteInstance>(blueprints[i % Global::nSheets],
				spa::Vec2F{ posDist(rne), posDist(rne) },
				spa::Vec2F{ 1.f, 0.f }.GetRotated(angleDist(rne)) * speedDist(rne)
			);
		}) | rn::to<std::vector>();

		// camera state variables
		spa::Vec2F pos{};
		float rot = 0.f;
		float scale = 1.f;
		// pair batchers together with a set of characters to draw to each batch
		auto batches = vi::zip(batchers, characters | vi::chunk(characters.size() / batchers.size() + 1));

		// do render loop while window not closing
		while (!pWindow->IsClosing()) {
			// camera movement controls
			if (keyboard->KeyIsPressed('W')) {
				constexpr auto deg90 = float(std::numbers::pi / 2.);
				auto up = spa::Vec2F{ std::cos(rot + deg90), std::sin(rot + deg90) };
				pos += up * 10.f;
			}
			else if (keyboard->KeyIsPressed('S')) {
				constexpr auto deg90 = float(std::numbers::pi / 2.);
				auto up = spa::Vec2F{ std::cos(rot + deg90), std::sin(rot + deg90) };
				pos -= up * 10.f;
			}
			if (keyboard->KeyIsPressed('D')) {
				auto right = spa::Vec2F{ std::cos(rot), std::sin(rot) };
				pos += right * 10.f;
			}
			else if (keyboard->KeyIsPressed('A')) {
				auto right = spa::Vec2F{ std::cos(rot), std::sin(rot) };
				pos -= right * 10.f;
			}
			if (keyboard->KeyIsPressed('Q')) {
				rot += 0.02f;
			}
			else if (keyboard->KeyIsPressed('E')) {
				rot -= 0.02f;
			}
			if (keyboard->KeyIsPressed('R')) {
				scale *= 1.02f;
			}
			else if (keyboard->KeyIsPressed('F')) {
				scale /= 1.02f;
			}
			for (auto& pBatcher : batchers) {
				pBatcher->SetCamera(pos, rot, scale);
			}

			// update sprites
			const auto markStartSpriteUpdate = hrclock::now();
			{
				auto drawFutures = batches | vi::transform([&](auto&& batch) {
					return std::async([&](auto&& batch) {
						auto&& [pBatcher, spritePtrRange] = batch;
						for (const auto& ps : spritePtrRange) {
							ps->Update(0.001f, rne);
						}
					}, batch);
				}) | rn::to<std::vector>();
			}
			const auto durationSpriteUpdate = hrclock::now() - markStartSpriteUpdate;
			const auto spriteUpdateMs = std::chrono::duration<float, std::milli>(durationSpriteUpdate).count();

			// begin frame
			pPane->BeginFrame();
			// prepare each batcher to draw to
			for (auto& pBatcher : batchers) {
				pBatcher->StartBatch(*pPane);
			}
			// spawn a thread for each batch to draw sprites to batchers
			const auto markStartSpriteDraw = hrclock::now();
			{
				auto drawFutures = batches | vi::transform([&](auto&& batch) {
					return std::async([&](auto&& batch) {
						auto&& [pBatcher, spritePtrRange] = batch;
						for (const auto& ps : spritePtrRange) {
							ps->Draw(*pBatcher);
						}
					}, batch);
				}) | rn::to<std::vector>();
			}
			const auto durationSpriteDraw = hrclock::now() - markStartSpriteDraw;
			const auto spriteDrawMs = std::chrono::duration<float, std::milli>(durationSpriteDraw).count();
			// finish all batcher batches and submit resulting command list to the pane
			for (auto& pBatcher : batchers) {
				pBatcher->EndBatch(*pPane);
			}
			// finish and present the frame
			pPane->EndFrame();

			// output benching information
			OutputDebugStringA(std::format("s-up [{:>8.4f}ms]  s-draw [{:>8.4f}ms]\n", spriteUpdateMs, spriteDrawMs).c_str());
		}
		pPane->FlushQueues();
		isLive = false;
	}
	catch (...) {
		exception_ = std::current_exception();
		hasException_ = true;
		constructionSemaphore_.release();
	}
}