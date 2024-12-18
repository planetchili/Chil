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

class Operation
{
public:
	Operation(const net::MoveCommand& cmd)
		:
		sprite_{ cmd.start },
		coro_{ MoveSpriteTo(sprite_, cmd.end) },
		it_{ coro_.begin() }
	{}
	void Update()
	{
		if (it_ != coro_.end()) {
			it_++;
		}
		sprite_.Update();
	}
	void Draw()
	{
		sprite_.Draw();
	}
private:
	Sprite sprite_;
	co::recursive_generator<int> coro_;
	co::recursive_generator<int>::iterator it_;
};

void RunGeneratorCoroMode()
{
	auto pClient = net::IClient::Make();

	auto& opts = opt::Get();
	simp::Init({ *opts.width, *opts.height }, L"Generator Coro Behavior", *opts.logLevel);
	
	std::vector<std::unique_ptr<Operation>> operationPtrs;
	while (!simp::Win().IsClosing()) {
		for (auto& cmd : pClient->ReceiveCommands()) {
			operationPtrs.emplace_back(std::make_unique<Operation>(cmd));
		}
		for (auto& op : operationPtrs) {
			op->Update();
		}
		simp::Begin();
		for (auto& op : operationPtrs) {
			op->Draw();
		}
		simp::End();
	}
}