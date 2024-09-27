#pragma once
#include <Core/src/cli/CliFramework.h>
#include <Core/src/log/Level.h>
#include <cstdint>
#include <utility>

enum class RunMode
{
	StateMachine,
	GeneratorCoro,
};

namespace cli
{
	using namespace ::chil::cli;
	using ::chil::log::Level;

	struct Options : public OptionsContainer<Options>
	{
		CHIL_CLI_OPT(width, int, "Width of the output window(s)", 1280);
		CHIL_CLI_OPT(height, int, "Height of the output window(s)", 720);
		CHIL_CLI_OPT(seed, uint32_t, "Value to seed random engine with", 42069);
		CHIL_CLI_OPT(runMode, RunMode, "Mode to execute from main()", RunMode::StateMachine, cust::EnumMap<RunMode>());
		CHIL_CLI_OPT(logLevel, Level, "Severity to log at", Level::Error, cust::EnumMap<Level>());
	private:
		std::string GetDesc() const override { return "Pulling and pulling on my yellow leg"; };
		rule::Dependency widDep_{ width, height };
		rule::Dependency hgtDep_{ height, width };
	};
}

using opt = ::cli::Options;