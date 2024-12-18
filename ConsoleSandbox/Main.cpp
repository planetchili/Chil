#include <iostream>
#include <Core/src/log/Log.h>
#include <Core/src/ioc/Container.h>
#include <Core/src/log/SeverityLevelPolicy.h>
#include "Child.h"
#include <thread>

using namespace chil;
using namespace std::literals;

void Boot()
{
	log::Boot();

	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Info);
	});
}

int main(int argc, const char** argv)
{
	Boot();

	auto child = IChild::Spawn();

	std::this_thread::sleep_for(2s);

	return 0;
}