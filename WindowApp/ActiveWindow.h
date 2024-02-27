#pragma once
#include <Core/src/gfx/ISpriteCodex.h>
#include <memory>
#include <semaphore>
#include <thread>


class ActiveWindow
{
public:
	ActiveWindow(int index, std::shared_ptr<chil::gfx::ISpriteCodex> pSpriteCodex);
	bool IsLive() const;
private:
	// functions
	void Kernel_(int index, std::shared_ptr<chil::gfx::ISpriteCodex> pSpriteCodex);
	// data
	std::binary_semaphore constructionSemaphore_{ 0 };
	std::atomic<bool> isLive{ true };
	std::jthread thread_;
	std::atomic<bool> hasException_ = false;
	std::exception_ptr exception_;
};