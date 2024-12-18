#include "Child.h"
#include <Core/src/win/ChilWin.h>
#pragma warning(push)
#pragma warning(disable : 26436 26433 26495 26451 6255 6387 6031 26819 6258 6001 26498 26439 26437 6294 6201 26433 6388 26110)
#include <boost/process.hpp>
#pragma warning(pop)

namespace bp = boost::process;

class Child : public IChild
{
public:
	Child()
		:
		child_{ R"(..\x64\Debug\WindowApp.exe)",
			"--run-mode", "GeneratorCoro",
			bp::start_dir(R"(..\x64\Debug)") }
	{}
	void Terminate() override
	{
		child_.terminate();
		child_.wait();
	}
private:
	bp::child child_;
};

std::unique_ptr<IChild> IChild::Spawn()
{
	return std::make_unique<Child>();
}