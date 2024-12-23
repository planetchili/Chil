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
			std::regex patternSingle(R"(\s*([+-]?\d*\.?\d+)(.*?))");
			std::smatch match;
			if (std::regex_match(argstring, match, patternSingle)) {
				net::MoveCommand mv;
				mv.speed = std::stof(match[1].str());
				argstring = match[2].str();
				std::regex pattern(R"(\s*([+-]?\d*\.?\d+),([+-]?\d*\.?\d+)(.*?))");
				std::smatch matches;
				while (std::regex_match(argstring, matches, pattern)) {
					const auto x = std::stof(matches[1].str());
					const auto y = std::stof(matches[2].str());
					mv.waypoints.emplace_back(x, y);
					argstring = matches[3].str();
				}
				if (mv.waypoints.size() >= 2) {
					pServer->SendCommand(std::move(mv));
				}
				else {
					std::cout << "Bad format for command [mv]" << std::endl;
				}
			}
			else {
				std::cout << "Bad format for command [mv]" << std::endl;
			}
		}
		else if (command == "tit") {
			std::regex pattern(R"(^\s*(\w+)\s*(.*)$)");
			std::smatch match;
			if (std::regex_match(argstring, match, pattern)) {
				pServer->SendCommand(net::TitleCommand{ .title = match[1].str(), .shmitle = match[2].str() });
			}
			else {
				std::cout << "Bad arguments for command [tit]" << std::endl;
			}
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