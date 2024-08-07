#pragma once
#include <Core/src/cli/CliFramework.h>

namespace cli
{
	using namespace ::chil::cli;

	enum class Dinkum
	{
		chart,
		fart,
		smart,
	};

	using MyPair = std::pair<int, std::string>;
	struct Options : public OptionsContainer<Options>
	{
		CHIL_CLI_FLG(shitTheBed, "s", "poopy!");
		CHIL_CLI_FLG(funtimeInBed, "f", "funtimes!");
		CHIL_CLI_OPT(numWindows, int, "Number of windows to spawn", 2);
		CHIL_CLI_OPT(nonDefault, int, "Doesn't logically have a default", std::nullopt, cust::Range(69, 420));
		CHIL_CLI_OPT(path, std::string, "Path in the filesystem", std::nullopt, cust::ExistingPath());
		CHIL_CLI_OPT(wammy, std::string, "wwwww", std::nullopt, cust::In<false>({"spig", "spog"}));
		CHIL_CLI_FLG(lick, "l", "licking");
		CHIL_CLI_FLG(brick, "b", "bricking");
		CHIL_CLI_FLG(dick, "d", "dicking");
		CHIL_CLI_FLG(rick, "r", "never gonna");
		CHIL_CLI_OPT(width, int, "wideification", 640);
		CHIL_CLI_OPT(height, int, "tallification", 480);
		CHIL_CLI_OPT(list, std::vector<int>, "listed");
		CHIL_CLI_OPT(pair, MyPair, "paired");
		CHIL_CLI_OPT(dinkum, Dinkum, "Dinkum enum option", std::nullopt, cust::In({ Dinkum::chart, Dinkum::smart }) | cust::EnumMap<Dinkum>());
	private:
		std::string GetDesc() const override { return "Pulling and pulling on my yellow leg"; };
		rule::MutualExclusion ickMex_{ lick, brick, dick, rick };
		rule::MutualExclusion bedMex_{ shitTheBed, funtimeInBed };
		rule::MutualExclusion winMex_{ shitTheBed, numWindows };
		rule::Dependency widDep_{ width, height };
		rule::Dependency hgtDep_{ height, width };
		rule::AllowExtras ext_{ this };
	};
}

using opt = ::cli::Options;