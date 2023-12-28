#pragma once
#include <Core/src/gfx/d12/SpriteBatcher.h>
#include <Core/src/spa/Vec2.h>
#include <vector>
#include <memory>
#include <random>

namespace chil
{
	enum class AnimationId
	{
		Null,
		WalkDown,
		WalkLeft,
		WalkRight,
		WalkUp,
	};

	struct AnimationFrame
	{
		std::shared_ptr<gfx::d12::VINTERFACE(SpriteFrame)> pFrame_;
		float holdSeconds_;
	};

	class ISpriteBlueprint
	{
	public:
		struct State
		{
			AnimationId currentAnimation_ = AnimationId::Null;
			int currentAnimationFrame_ = 0;
			float timeInAnimationFrame_ = 0.f;
		};
		virtual void ChangeDirection(State&, const spa::Vec2F&) const = 0;
		virtual void StepAnimation(State&, float dt) const = 0;
		virtual const gfx::d12::VINTERFACE(SpriteFrame)& GetFrame(const State&) const = 0;
		virtual ~ISpriteBlueprint() = default;
	};

	class SpriteBlueprint VSELECT(: public ISpriteBlueprint)
	{
	public:
		SpriteBlueprint(std::shared_ptr<gfx::d12::SpriteCodex> pCodex, size_t atlasIndex, int hCells, int vCells)
		{
			const auto sheetDims = pCodex->GetAtlasDimensions(atlasIndex);
			for (int v = 0; v < vCells; v++) {
				for (int h = 0; h < hCells; h++) {
					auto pFrame = std::make_shared<gfx::d12::SpriteFrame>(
						spa::DimensionsI{ hCells, vCells }, spa::Vec2I{h, v}, atlasIndex, pCodex);
					animations_[v].push_back(AnimationFrame{
						.pFrame_ = std::move(pFrame),
						.holdSeconds_ = 0.016f,
					});
				}
			}
		}
		void ChangeDirection(ISpriteBlueprint::State& state, const spa::Vec2F& vel) const VOVERRIDE
		{
			const auto oldAnimation = state.currentAnimation_;
			if (std::abs(vel.x) >= std::abs(vel.y)) {
				if (vel.x >= 0.f) {
					state.currentAnimation_ = AnimationId::WalkRight;
				}
				else {
					state.currentAnimation_ = AnimationId::WalkLeft;
				}
			}
			else {
				if (vel.y >= 0.f) {
					state.currentAnimation_ = AnimationId::WalkUp;
				}
				else {
					state.currentAnimation_ = AnimationId::WalkDown;
				}
			}
			if (state.currentAnimation_ != oldAnimation) {
				state.timeInAnimationFrame_ = 0.f;
				state.currentAnimationFrame_ = 0;
			}
		}
		void StepAnimation(ISpriteBlueprint::State& state, float dt) const VOVERRIDE
		{
			const auto& curAnimation = animations_[size_t(state.currentAnimation_) - 1];
			state.timeInAnimationFrame_ += dt;
			if (state.timeInAnimationFrame_ >= curAnimation[state.currentAnimationFrame_].holdSeconds_) {
				state.timeInAnimationFrame_ = 0.f;
				if (++state.currentAnimationFrame_ >= curAnimation.size()) {
					state.currentAnimationFrame_ = 0;
				}
			}
		}
		const gfx::d12::VINTERFACE(SpriteFrame)& GetFrame(const ISpriteBlueprint::State& state) const VOVERRIDE
		{
			return *animations_[size_t(state.currentAnimation_) - 1][state.currentAnimationFrame_].pFrame_;
		}
	private:
		std::array<std::vector<AnimationFrame>, 4> animations_;
	};

	class ISpriteInstance
	{
	public:
		virtual void Draw(gfx::d12::VINTERFACE(SpriteBatcher)&) const = 0;
		virtual void Update(float dt, std::minstd_rand0& rng) = 0;
		virtual ~ISpriteInstance() = default;
	};

	class SpriteInstance VSELECT(: public ISpriteInstance)
	{
	public:
		SpriteInstance(std::shared_ptr<VINTERFACE(SpriteBlueprint)> pBlueprint,
			const spa::Vec2F& pos, const spa::Vec2F& vel)
			:
			pBlueprint_{ std::move(pBlueprint) },
			pos_{ pos },
			vel_{ vel }
		{
			pBlueprint_->ChangeDirection(animationState_, vel_);
		}
		void Draw(gfx::d12::VINTERFACE(SpriteBatcher)& batcher) const VOVERRIDE
		{
			pBlueprint_->GetFrame(animationState_).DrawToBatch(batcher, pos_);
		}
		void Update(float dt, std::minstd_rand0& rng) VOVERRIDE
		{
			timeUntilChangeDirection_ -= dt;
			if (timeUntilChangeDirection_ <= 0.f) {
				const auto speed = vel_.GetLength();
				const auto toCenter = (-pos_).GetNormalized();
				const auto theta = std::uniform_real_distribution<float>{ 0.f, 0.8f }(rng);
				vel_ = (toCenter * speed).GetRotated(theta);
				pBlueprint_->ChangeDirection(animationState_, vel_);
				timeUntilChangeDirection_ = directionChangePeriod_;
			}
			else {
				pBlueprint_->StepAnimation(animationState_, dt);
				pos_ += vel_ * dt;
			}
		}
	private:
		constexpr static float directionChangePeriod_ = 1.f;
		std::shared_ptr<VINTERFACE(SpriteBlueprint)> pBlueprint_;
		ISpriteBlueprint::State animationState_{};
		spa::Vec2F pos_;
		spa::Vec2F vel_;
		float timeUntilChangeDirection_ = directionChangePeriod_;
	};
}