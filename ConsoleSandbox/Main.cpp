#include <iostream>
#include <Core/src/log/Log.h>
#include <Core/src/ioc/Container.h>
#include <Core/src/log/SeverityLevelPolicy.h>
#include <Core/src/net/Net.h>
#include "Child.h"
#include <thread>
#include <regex>

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

	auto pChild = IChild::Spawn();
	std::cout << "Child spawned!\n";

	auto pServer = net::IServer::Make();
	std::cout << "Child connected!\n";

	std::cout << "> ";
	for (std::string line; std::getline(std::cin, line);) {
		// split into command and arguments
		std::regex pattern(R"(^\s*(\w+)\s*(.*?)$)");
		std::smatch match;
		if (!std::regex_match(line, match, pattern)) {
			std::cout << "Unknown command format\n> ";
			continue;
		}
		auto command = match[1].str();
		auto argstring = match[2].str();

		if (command == "exit") {
			break;
		}
		else if (command == "mv") {
			std::regex pattern(R"(\s*([+-]?\d*\.?\d+),([+-]?\d*\.?\d+)\s+([+-]?\d*\.?\d+),([+-]?\d*\.?\d+))");
			std::smatch matches;
			if (std::regex_match(argstring, matches, pattern)) {
				// Extract the 4 float values from the match groups 
				const auto x1 = std::stof(matches[1].str());
				const auto y1 = std::stof(matches[2].str());
				const auto x2 = std::stof(matches[3].str());
				const auto y2 = std::stof(matches[4].str());
				// create the command and send that bad boi
				pServer->SendCommand(net::MoveCommand{ {x1, y1}, {x2, y2} });
			}
			else {
				std::cout << "Bad argument for move command!\n";
			}
		}
		else if (command == "tit") {
			pServer->SendCommand(net::TitleCommand{ .title = std::move(argstring) });
		}
		else {
			std::cout << "Unknown command: " << command << std::endl;
		}

		std::cout << "> ";
	}

	pChild->Terminate();
	std::cout << "Child terminated!\n";

	return 0;
}