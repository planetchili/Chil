#pragma once
#include <Core/src/cli/CliFramework.h>
#include <cstdint>
#include <utility>

enum class RunMode
{
	Normal,
	Simple,
};

namespace cli
{
	using namespace ::chil::cli;

	using DimsPair = std::pair<int, int>;
	struct Options : public OptionsContainer<Options>
	{
		CHIL_CLI_OPT(numCharacters, uint32_t, "Number of sprite characters to spawn", DBG_TERN(250'000, 500));
		CHIL_CLI_OPT(numBatches, uint32_t, "Number of batch threads to use", 4);
		CHIL_CLI_OPT(numSheets, uint32_t, "Number of sprite sheet variations to use", 32);
		CHIL_CLI_OPT(width, int, "Width of the output window(s)", 1280);
		CHIL_CLI_OPT(height, int, "Height of the output window(s)", 720);
		CHIL_CLI_OPT(numWindows, uint32_t, "Number of windows to run", 1);
		CHIL_CLI_OPT(seed, uint32_t, "Value to seed random engine with", 42069);
		CHIL_CLI_OPT(framesToRun, uint32_t, "Number of frames to run before automatic stop", 0);
		CHIL_CLI_OPT(runMode, RunMode, "Mode to execute from main()", RunMode::Normal, cust::EnumMap<RunMode>());
	private:
		std::string GetDesc() const override { return "Pulling and pulling on my yellow leg"; };
		rule::Dependency widDep_{ width, height };
		rule::Dependency hgtDep_{ height, width };
	};
}

using opt = ::cli::Options;