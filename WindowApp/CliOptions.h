#pragma once
#include <Core/src/cli/CliFramework.h>

namespace cli
{
	using namespace ::chil::cli;

	struct Options : public OptionsContainer<Options>
	{
		Option<int> numWindows{ this, "--num-windows", "Number of windows to spawn" };
	};
}

using opt = ::cli::Options;