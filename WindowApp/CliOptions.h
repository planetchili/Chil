#pragma once
#include <Core/src/cli/CliFramework.h>

namespace cli
{
	using namespace ::chil::cli;

	struct Options : public OptionsContainer<Options>
	{
		Flag shitTheBed{ this, "--shit-the-bed", "poopy!" };
		Option<int> numWindows{ this, "--num-windows", "Number of windows to spawn" };
	};
}

using opt = ::cli::Options;