#pragma once
#include <Core/src/cli/CliFramework.h>

namespace cli
{
	using namespace ::chil::cli;

	struct Options : public OptionsContainer<Options>
	{
		Flag shitTheBed{ this, "--shit-the-bed,-s", "poopy!" };
		Flag funtimeInBed{ this, "--funtime-in-bed,-f", "funtimes!" };
		Option<int> numWindows{ this, "--num-windows", "Number of windows to spawn", 2 };
		Option<int> nonDefault{ this, "--non-default", "Doesn't logically have a default", {}, CLI::Range{ 69, 420 } };
		Option<std::string> path{ this, "--path", "Path in the filesystem", {}, CLI::ExistingPath };
		Flag lick{ this, "--lick,-l", "licking" };
		Flag brick{ this, "--brick,-b", "bricking" };
		Flag dick{ this, "--dick,-d", "dicking" };
		Flag rick{ this, "--rick,-r", "never gonna" };
		Option<int> width{ this, "--width", "wideification", 640 };
		Option<int> height{ this, "--height", "tallification", 480 };
	private:
		rule::MutualExclusion ickMex_{ lick, brick, dick, rick };
		rule::MutualExclusion bedMex_{ shitTheBed, funtimeInBed };
		rule::MutualExclusion winMex_{ shitTheBed, numWindows };
		rule::Dependency widDep_{ width, height };
		rule::Dependency hgtDep_{ height, width };
		rule::AllowExtras ext_{ this };
	};
}

using opt = ::cli::Options;