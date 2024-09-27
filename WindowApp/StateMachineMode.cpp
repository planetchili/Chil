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

class TriangleMachine
{
	enum class State
	{
		GoingTopStart,
		GoingRight,
		GoingLeft,
		GoingTopEnd,
		Done,
	};
public:
	TriangleMachine(spa::Vec2F center, float scale)
	{
		top_ = center + spa::Vec2F{ 0.f, .5f } * scale;
		right_ = center + spa::Vec2F{ .5f, -.5f } * scale;
		left_ = center + spa::Vec2F{ -.5f, -.5f } * scale;
	}
	void Step(Sprite& s)
	{
		switch (state_) {
		case State::GoingTopStart:
			if ((top_ - s.GetPos()).GetLength() < 1.f) {
				state_ = State::GoingRight;
			}
			else {
				SendSpriteTo(s, top_);
			}
			break;
		case State::GoingRight:
			if ((right_ - s.GetPos()).GetLength() < 1.f) {
				state_ = State::GoingLeft;
			}
			else {
				SendSpriteTo(s, right_);
			}
			break;
		case State::GoingLeft:
			if ((left_ - s.GetPos()).GetLength() < 1.f) {
				state_ = State::GoingTopEnd;
			}
			else {
				SendSpriteTo(s, left_);
			}
			break;
		case State::GoingTopEnd:
			if ((top_ - s.GetPos()).GetLength() < 1.f) {
				state_ = State::Done;
			}
			else {
				SendSpriteTo(s, top_);
			}
			break;
		case State::Done:
			s.SetDir({ 0,0 });
			break;
		}
	}
	bool IsDone() const
	{
		return state_ == State::Done;
	}
private:
	State state_ = State::GoingTopStart;
	spa::Vec2F top_;
	spa::Vec2F right_;
	spa::Vec2F left_;
};

void RunStateMachineMode()
{
	auto& opts = opt::Get();
	simp::Init({ *opts.width, *opts.height }, L"State Machine Behavior", *opts.logLevel);
	Sprite s;
	TriangleMachine tm{ {200.f, 200.f}, 100.f };
	while (!simp::Win().IsClosing() && !tm.IsDone()) {
		tm.Step(s);
		s.Update();
		simp::Begin();
		s.Draw();
		simp::End();
	}
}