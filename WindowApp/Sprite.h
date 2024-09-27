#pragma once
#include <Core/src/spa/Vec2.h>
#include <Core/src/simp/SimpleContext.h>
#include <Core/src/gfx/SpriteFrame.h>

namespace chil
{
	class Sprite
	{
	public:
		float GetSpeed() const
		{
			return speed_;
		}
		const spa::Vec2F& GetPos() const
		{
			return pos_;
		}
		const spa::Vec2F& GetDir() const
		{
			return dir_;
		}
		void SetDir(const spa::Vec2F& dir)
		{
			dir_ = dir;
		}
		void Update()
		{
			pos_ += dir_.GetClamped() * speed_;
		}
		void Draw() const
		{
			frame_.DrawToBatch(simp::Batch(), pos_);
		}
	private:
		gfx::SpriteFrame frame_{ {1, 1}, {0, 0}, simp::LoadAtlas(L"frog.png") };
		spa::Vec2F pos_{ 0.f, 0.f };
		spa::Vec2F dir_{ 0.f, 0.f };
		float speed_ = 1.f;
	};

	void SendSpriteTo(Sprite& sprite, const spa::Vec2F& target)
	{
		sprite.SetDir(target - sprite.GetPos());
	}
}