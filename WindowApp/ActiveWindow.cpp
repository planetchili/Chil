#include "ActiveWindow.h"
#include <Core/src/ioc/Container.h>
#include <Core/src/win/IWindow.h>
#include <Core/src/win/Input.h>
#include <Core/src/gfx/IRenderPane.h>
#include <ranges>
#include <numbers>
#include <numeric>
#include "Sprite.h"
#include "Global.h"

using namespace chil;
using hrclock = std::chrono::high_resolution_clock;
namespace rn = std::ranges;
namespace vi = rn::views;


class BenchData
{
public:
	BenchData(size_t initial)
	{
		datapoints_.reserve(initial);
	}
	void Push(float data) { datapoints_.push_back(data); }
	float GetPercentile(float percentile)
	{
		if (datapoints_.empty()) {
			return std::numeric_limits<float>::quiet_NaN();
		}
		Sort_();
		return datapoints_[(size_t)std::round(float(datapoints_.size()) * (percentile / 100.f))];
	}
	float GetAverage()
	{
		if (datapoints_.empty()) {
			return std::numeric_limits<float>::quiet_NaN();
		}
		return float(std::accumulate(datapoints_.begin(), datapoints_.end(), 0.)
			/ double(datapoints_.size()));
	}
	int GetCount() const
	{
		return (int)datapoints_.size();
	}
	void DitchFirstPercent(float percent)
	{
		const auto n = int(float(datapoints_.size()) * (percent / 100.f));
		datapoints_ = datapoints_ | vi::drop(n) | rn::to<std::vector>();
	}
private:
	void Sort_()
	{
		if (!sorted) {
			rn::sort(datapoints_);
			sorted = true;
		}
	}
	bool sorted = false;
	std::vector<float> datapoints_;
};


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
		// setup benching data
		BenchData updates{ 10'000 };
		BenchData draws{ 10'000 };

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

		//// random engine
		//std::minstd_rand0 rne{ Global::seed };
		//// sprite blueprints
		//std::vector<std::shared_ptr<ISpriteBlueprint>> blueprints;
		//for (int i = 0; i < Global::nSheets; i++) {
		//	blueprints.push_back(std::make_shared<SpriteBlueprint>(pSpriteCodex, i, 8, 4));
		//}
		//// sprite instances
		//const auto characters =
		//	vi::iota(0u, Global::nCharacters) |
		//	vi::transform([
		//		&blueprints,
		//		&rne,
		//		posDist = std::uniform_real_distribution<float>{ -360.f, 360.f },
		//		speedDist = std::uniform_real_distribution<float>{ 240.f, 600.f },
		//		angleDist = std::uniform_real_distribution<float>{ 0.f, 2.f * std::numbers::pi_v<float> }
		//	] (uint32_t i) mutable -> std::unique_ptr<ISpriteInstance> {
		//	return std::make_unique<SpriteInstance>(blueprints[i % Global::nSheets],
		//		spa::Vec2F{ posDist(rne), posDist(rne) },
		//		spa::Vec2F{ 1.f, 0.f }.GetRotated(angleDist(rne)) * speedDist(rne)
		//	);
		//}) | rn::to<std::vector>();

		// camera state variables
		spa::Vec2F pos{};
		float rot = 0.f;
		float scale = 1.f;
		float srot = 0.f;
		// pair batchers together with a set of characters to draw to each batch
		//auto batches = vi::zip(batchers, characters | vi::chunk(characters.size() / batchers.size() + 1));

		gfx::SpriteFrame frame{ { 8, 4}, {0,0}, 0, pSpriteCodex };
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

			//// update sprites
			//const auto markStartSpriteUpdate = hrclock::now();
			//{
			//	auto drawFutures = batches | vi::transform([&](auto&& batch) {
			//		return std::async([&](auto&& batch) {
			//			auto&& [pBatcher, spritePtrRange] = batch;
			//			for (const auto& ps : spritePtrRange) {
			//				ps->Update(0.001f, rne);
			//			}
			//		}, batch);
			//	}) | rn::to<std::vector>();
			//}
			//const auto durationSpriteUpdate = hrclock::now() - markStartSpriteUpdate;
			//const auto spriteUpdateMs = std::chrono::duration<float, std::milli>(durationSpriteUpdate).count();
			// begin frame
			pPane->BeginFrame();
			batchers[0]->StartBatch(*pPane);
			// prepare each batcher to draw to
			//for (auto& pBatcher : batchers) {
			//	pBatcher->StartBatch(*pPane);
			//}
			
			// spawn a thread for each batch to draw sprites to batchers
			//const auto markStartSpriteDraw = hrclock::now();
			//{
			//	auto drawFutures = batches | vi::transform([&](auto&& batch) {
			//		return std::async([&](auto&& batch) {
			//			auto&& [pBatcher, spritePtrRange] = batch;
			//			for (const auto& ps : spritePtrRange) {
			//				ps->Draw(*pBatcher);
			//			}
			//		}, batch);
			//	}) | rn::to<std::vector>();
			//}
			//const auto durationSpriteDraw = hrclock::now() - markStartSpriteDraw;
			//const auto spriteDrawMs = std::chrono::duration<float, std::milli>(durationSpriteDraw).count();

			frame.DrawToBatch(*batchers[0], {0,0}, srot, {10, 10});
			//// finish all batcher batches and submit resulting command list to the pane
			//for (auto& pBatcher : batchers) {
			//	pBatcher->EndBatch(*pPane);
			//}
			batchers[0]->EndBatch(*pPane);
			// finish and present the frame
			pPane->EndFrame();

			// accumulate benching information
			//updates.Push(spriteUpdateMs);
			//draws.Push(spriteDrawMs);

			if constexpr (Global::framesToRunFor) {
				if (updates.GetCount() >= Global::framesToRunFor) {
					break;
				}
			}
			srot += 0.003;
		}
		updates.DitchFirstPercent(10.f);
		draws.DitchFirstPercent(10.f);
		OutputDebugStringA(std::format("spr-upd | avg [{:>8.4f}ms] 99% [{:>8.4f}ms] 90% [{:>8.4f}ms]\n",
			updates.GetAverage(), updates.GetPercentile(99.f), updates.GetPercentile(90.f)).c_str());
		OutputDebugStringA(std::format("spr-drw | avg [{:>8.4f}ms] 99% [{:>8.4f}ms] 90% [{:>8.4f}ms]\n",
			draws.GetAverage(), draws.GetPercentile(99.f), draws.GetPercentile(90.f)).c_str());
		pPane->FlushQueues();
		isLive = false;
	}
	catch (...) {
		exception_ = std::current_exception();
		hasException_ = true;
		constructionSemaphore_.release();
	}
}