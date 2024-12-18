#pragma once
#include <memory>

class IChild
{
public:
	static std::unique_ptr<IChild> Spawn();
	virtual ~IChild() = default;
	virtual void Terminate() = 0;
};