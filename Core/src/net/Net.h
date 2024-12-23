#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <variant>
#include <span>
#include <concepts>
#include <string>
#include <Core/src/spa/Vec2.h>

namespace chil::net
{
	struct MoveCommand
	{
		std::vector<spa::Vec2F> waypoints;
	};

	struct TitleCommand
	{
		std::string title;
		std::string shmitle;
	};

	using Command = std::variant<MoveCommand, TitleCommand>;

	class IServer
	{
	public:
		virtual ~IServer() = default;
		virtual void SendCommand(const Command& cmd) = 0;
		static std::unique_ptr<IServer> Make();
	};

	class IClient
	{
	public:
		virtual ~IClient() = default;
		virtual std::vector<Command> ReceiveCommands() = 0;
		static std::unique_ptr<IClient> Make();
	};
}