#pragma once
#include <Core/src/cli/CliFramework.h>

namespace cli
{
	using namespace ::chil::cli;

	struct Options : public OptionsContainer<Options>
	{
		Flag shitTheBed{ this, "--shit-the-bed", "poopy!" };
		Option<int> numWindows{ this, "--num-windows", "Number of windows to spawn", 2, [this](CLI::Option* pOpt) { pOpt->excludes(GetOpt_(shitTheBed)); } };
		Option<int> nonDefault{ this, "--non-default", "Doesn't logically have a default", {}, CLI::Range{ 69, 420 } };
		Option<std::string> path{ this, "--path", "Path in the filesystem", {}, CLI::ExistingPath };
	};
}

using opt = ::cli::Options;